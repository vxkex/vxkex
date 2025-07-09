﻿///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     util.c
//
// Abstract:
//
//     Implements the main GUI of KexCfg.
//
// Author:
//
//     vxiiduu (09-Feb-2024)
//
// Environment:
//
//     Win32 mode. This part of the program is not run as SYSTEM.
//
// Revision History:
//
//     vxiiduu              09-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexcfg.h"
#include "resource.h"
#include <KexGui.h>
#include <ShlObj.h>

STATIC HWND MainWindow = NULL;
STATIC HWND ListViewWindow = NULL;
STATIC BOOLEAN UnsavedChanges;
STATIC BOOLEAN SortOrderIsDescendingForColumn[2] = {0};

STATIC VOID KexCfgGuiPopulateGlobalConfiguration(
	VOID)
{
	BOOLEAN ExtendedContextMenu;
	BOOLEAN LoggingEnabled;
	WCHAR LogDir[MAX_PATH];

	//
	// Populate the Logging control group
	//

	KxCfgQueryLoggingSettings(&LoggingEnabled, LogDir, ARRAYSIZE(LogDir));
	CheckDlgButton(MainWindow, IDC_ENABLELOGGING, LoggingEnabled);
	SetDlgItemText(MainWindow, IDC_LOGDIR, LogDir);

	//
	// Populate the System Integration control group
	//

	CheckDlgButton(MainWindow, IDC_ENABLEFORMSI, KxCfgQueryVxKexEnabledForMsiexec());
	CheckDlgButton(MainWindow, IDC_CPIWBYPA, KxCfgQueryExplorerCpiwBypass());

	if (KxCfgQueryShellContextMenuEntries(&ExtendedContextMenu)) {
		CheckDlgButton(MainWindow, IDC_ADDTOMENU, TRUE);
		ComboBox_SetCurSel(GetDlgItem(MainWindow, IDC_WHICHCONTEXTMENU), ExtendedContextMenu ? 0 : 1);
	} else {
		CheckDlgButton(MainWindow, IDC_ADDTOMENU, FALSE);
	}
}

STATIC VOID KexCfgGuiApplyGlobalConfiguration(
	VOID)
{
	BOOLEAN Success;
	HANDLE TransactionHandle;
	BOOLEAN EnableLogging;
	BOOLEAN EnableForMsiexec;
	BOOLEAN EnableCpiwbypa;
	BOOLEAN EnableContextMenu;
	BOOLEAN ExtendedContextMenu;
	WCHAR LogDir[MAX_PATH];

	//
	// Gather inputs
	//

	EnableForMsiexec = IsDlgButtonChecked(MainWindow, IDC_ENABLEFORMSI);
	EnableCpiwbypa = IsDlgButtonChecked(MainWindow, IDC_CPIWBYPA);
	EnableContextMenu = IsDlgButtonChecked(MainWindow, IDC_ADDTOMENU);
	ExtendedContextMenu = (ComboBox_GetCurSel(GetDlgItem(MainWindow, IDC_WHICHCONTEXTMENU)) == 0);
	EnableLogging = IsDlgButtonChecked(MainWindow, IDC_ENABLELOGGING);

	GetDlgItemText(MainWindow, IDC_LOGDIR, LogDir, ARRAYSIZE(LogDir));

	//
	// Create transaction for the operation
	//

	TransactionHandle = CreateSimpleTransaction(L"VxKex Configuration Tool (GUI) Transaction");

	if (!TransactionHandle) {
		if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
			ErrorBoxF(
				L"无法为此操作创建事务。%s",
				GetLastErrorAsString());
		} else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
			ErrorBoxF(
				L"﻿無法為此操作創建事務。%s",
				GetLastErrorAsString());
		} else {
			ErrorBoxF(
				L"A transaction for this operation could not be created. %s",
				GetLastErrorAsString());
		}

		return;
	}

	//
	// Apply Logging settings
	//

	Success = KxCfgConfigureLoggingSettings(
		EnableLogging,
		LogDir,
		TransactionHandle);

	//
	// Apply system integration settings
	//

	Success = KxCfgEnableVxKexForMsiexec(EnableForMsiexec, TransactionHandle);
	if (!Success) {
		goto Fail;
	}

	Success = KxCfgEnableExplorerCpiwBypass(EnableCpiwbypa, TransactionHandle);
	if (!Success) {
		goto Fail;
	}

	Success = KxCfgConfigureShellContextMenuEntries(EnableContextMenu, ExtendedContextMenu, TransactionHandle);
	if (!Success) {
		goto Fail;
	}

	NtCommitTransaction(TransactionHandle, TRUE);
	SafeClose(TransactionHandle);

	UnsavedChanges = FALSE;
	return;

Fail:
	NtRollbackTransaction(TransactionHandle, TRUE);
	SafeClose(TransactionHandle);

	if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ErrorBoxF(L"无法应用设置。%s", GetLastErrorAsString());
	else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ErrorBoxF(L"無法應用設定。%s", GetLastErrorAsString());
	else ErrorBoxF(L"The settings could not be applied. %s", GetLastErrorAsString());
}

BOOLEAN CALLBACK ConfigurationCallback(
	IN	PCWSTR							ExeFullPathOrBaseName,
	IN	BOOLEAN							IsLegacyConfiguration,
	IN	PVOID							ExtraParameter)
{
	WCHAR ExeContainingFolder[MAX_PATH];
	PWSTR ExeBaseName;
	LVITEM ListViewItem;
	SHFILEINFO ShellFileInfo;
	ULONG_PTR Success;

	if (IsLegacyConfiguration) {
		// The installer should've removed all this crap
		ASSERT (!IsLegacyConfiguration);
		return TRUE;
	}

	StringCchCopy(
		ExeContainingFolder,
		ARRAYSIZE(ExeContainingFolder),
		ExeFullPathOrBaseName);

	PathCchRemoveFileSpec(ExeContainingFolder, ARRAYSIZE(ExeContainingFolder));
	ExeBaseName = PathFindFileName(ExeFullPathOrBaseName);

	if (StringEqualI(ExeBaseName, L"msiexec.exe")) {
		// Special case. This is represented by a checkbox in the system integration
		// group, so we don't show it here.
		return TRUE;
	}

	//
	// Get icon of file. SHGetFileInfo works for all types of files including
	// MSI files, and gives us the same icon that would be displayed in Windows
	// Explorer.
	//

	Success = SHGetFileInfo(
		ExeFullPathOrBaseName,
		0,
		&ShellFileInfo,
		sizeof(ShellFileInfo),
		SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SYSICONINDEX);

	RtlZeroMemory(&ListViewItem, sizeof(ListViewItem));

	if (Success) {
		ListViewItem.mask |= LVIF_IMAGE;
		ListViewItem.iImage = ShellFileInfo.iIcon;
		DestroyIcon(ShellFileInfo.hIcon);
	}

	ListViewItem.mask	   |= LVIF_TEXT;
	ListViewItem.iItem		= INT_MAX;
	ListViewItem.pszText	= ExeBaseName;

	//
	// Insert the item and set text for all columns.
	//

	ListViewItem.iItem = ListView_InsertItem(ListViewWindow, &ListViewItem);
	ListView_SetItemText(ListViewWindow, ListViewItem.iItem, 1, ExeContainingFolder);

	return TRUE;
}

STATIC VOID KexCfgGuiPopulateApplicationList(
	VOID)
{
	SetWindowRedraw(ListViewWindow, FALSE);

	ListView_DeleteAllItems(ListViewWindow);
	KxCfgEnumerateConfiguration(ConfigurationCallback, NULL);

	if (ListView_GetItemCount(ListViewWindow) != 0) {
		// reflow column widths
		ListView_SetColumnWidth(ListViewWindow, 0, LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(ListViewWindow, 1, LVSCW_AUTOSIZE_USEHEADER);
	}

	SetWindowRedraw(ListViewWindow, TRUE);
}

STATIC PCWSTR GetProgramFullPathFromListViewIndex(
	IN	ULONG	ItemIndex)
{
	STATIC WCHAR FilePath[MAX_PATH];
	WCHAR FileName[MAX_PATH];

	ListView_GetItemText(ListViewWindow, ItemIndex, 0, FileName, ARRAYSIZE(FileName));
	ListView_GetItemText(ListViewWindow, ItemIndex, 1, FilePath, ARRAYSIZE(FilePath));
	PathCchAppend(FilePath, ARRAYSIZE(FilePath), FileName);

	return FilePath;
}

STATIC VOID RemoveSelectedPrograms(
	VOID)
{
	ULONG ItemIndex;
	HANDLE TransactionHandle;

	TransactionHandle = CreateSimpleTransaction(L"VxKex Configuration Tool (GUI) Transaction");

	if (!TransactionHandle) {
		if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
			ErrorBoxF(
				L"无法为此操作创建事务。%s",
				GetLastErrorAsString());
		} else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
			ErrorBoxF(
				L"﻿無法為此操作創建事務。%s",
				GetLastErrorAsString());
		} else {
			ErrorBoxF(
				L"A transaction for this operation could not be created. %s",
				GetLastErrorAsString());
		}

		return;
	}

	ItemIndex = ListView_GetNextItem(ListViewWindow, -1, LVNI_SELECTED);

	while (ItemIndex != -1) {
		PCWSTR ExeFullPath;
		BOOLEAN Success;

		ExeFullPath = GetProgramFullPathFromListViewIndex(ItemIndex);

		Success = KxCfgDeleteConfiguration(ExeFullPath, TransactionHandle);
		if (!Success) {
			if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
				ErrorBoxF(
					L"为“%s”应用设置时出现错误。%s",
					ExeFullPath, GetLastErrorAsString());
			} else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
				ErrorBoxF(
					L"為「%s」應用設定時出現錯誤。%s",
					ExeFullPath, GetLastErrorAsString());
			} else {
				ErrorBoxF(
					L"There was an error applying settings for \"%s\". %s",
					ExeFullPath, GetLastErrorAsString());
			}

			NtRollbackTransaction(TransactionHandle, TRUE);
			SafeClose(TransactionHandle);
			return;
		}

		ItemIndex = ListView_GetNextItem(ListViewWindow, ItemIndex, LVNI_SELECTED);
	}

	NtCommitTransaction(TransactionHandle, TRUE);
	SafeClose(TransactionHandle);

	// refresh the list of enabled applications (since we removed some)
	KexCfgGuiPopulateApplicationList();
}

STATIC VOID AddProgram(
	VOID)
{
	WCHAR FileNames[1024];
	WCHAR FileFullPath[MAX_PATH];
	WCHAR KexDir[MAX_PATH];
	PCWSTR DirectoryName;
	PCWSTR FileName;
	ULONG DirectoryNameCch;
	OPENFILENAME OpenFileInfo;
	KXCFG_PROGRAM_CONFIGURATION Configuration;
	HANDLE TransactionHandle;

	//
	// Get a list of one or more programs from the user.
	//

	FileNames[0] = '\0';

	RtlZeroMemory(&OpenFileInfo, sizeof(OpenFileInfo));
	OpenFileInfo.lStructSize		= sizeof(OpenFileInfo);
	OpenFileInfo.hwndOwner			= MainWindow;
	if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) OpenFileInfo.lpstrFilter			= L"程序（*.exe、*.msi）\0*.exe;*.msi\0";
	else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) OpenFileInfo.lpstrFilter			= L"程式（*.exe、*.msi）\0*.exe;*.msi\0";
	else OpenFileInfo.lpstrFilter		= L"Programs (*.exe, *.msi)\0*.exe;*.msi\0";
	OpenFileInfo.lpstrFile			= FileNames;
	OpenFileInfo.nMaxFile			= ARRAYSIZE(FileNames);
	if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) OpenFileInfo.lpstrTitle			= L"选择程序";
	else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) OpenFileInfo.lpstrTitle			= L"選擇程式";
	else OpenFileInfo.lpstrTitle			= L"Select Program(s)";
	OpenFileInfo.Flags				= OFN_EXPLORER | OFN_ALLOWMULTISELECT |
									  OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	OpenFileInfo.lpstrDefExt		= L"exe";

	if (GetOpenFileName(&OpenFileInfo) == FALSE) {
		ASSERT (CommDlgExtendedError() == 0);
		return;
	}

	ASSERT (FileNames[0] != '\0');

	DirectoryName = FileNames;
	DirectoryNameCch = (ULONG) wcslen(DirectoryName);
	ASSERT (DirectoryNameCch != 0);
	ASSERT (DirectoryNameCch < MAX_PATH);

	if (StringBeginsWithI(DirectoryName, SharedUserData->NtSystemRoot)) {
		// program(s) are in the Windows directory - do not allow
		if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ErrorBoxF(L"无法为 Windows 目录中的程序启用 VxKex。");
		else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ErrorBoxF(L"無法為 Windows 目錄中的程式啟用 VxKex。");
		else ErrorBoxF(L"You cannot enable VxKex for programs in the Windows directory.");
		return;
	}

	KxCfgGetKexDir(KexDir, ARRAYSIZE(KexDir));

	if (StringBeginsWithI(DirectoryName, KexDir)) {
		if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ErrorBoxF(L"无法为 VxKex 安装目录中的程序启用 VxKex。");
		else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ErrorBoxF(L"﻿無法為 VxKex 安裝目錄中的程式啟用 VxKex。");
		else ErrorBoxF(L"You cannot enable VxKex for programs in the VxKex installation directory.");
		return;
	}

	if (OpenFileInfo.nFileOffset >= DirectoryNameCch) {
		// User selected more than one file.
		RtlCopyMemory(FileFullPath, DirectoryName, DirectoryNameCch * sizeof(WCHAR));
		FileFullPath[DirectoryNameCch] = '\\';
		FileFullPath[DirectoryNameCch + 1] = '\0';
		++DirectoryNameCch;

		FileName = FileNames + DirectoryNameCch;
	} else {
		// User selected only one file.
		FileName = FileNames;
		FileNames[DirectoryNameCch + 1] = '\0';
		DirectoryNameCch = 0;
	}

	ASSERT (FileName[0] != '\0');

	RtlZeroMemory(&Configuration, sizeof(Configuration));
	Configuration.Enabled = 1;

	//
	// Create transaction for the operation.
	//

	TransactionHandle = CreateSimpleTransaction(L"VxKex Configuration Tool (GUI) Transaction");

	if (!TransactionHandle) {
		if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
			ErrorBoxF(
				L"无法为此操作创建事务。%s",
				GetLastErrorAsString());
		} else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
			ErrorBoxF(
				L"﻿無法為此操作創建事務。%s",
				GetLastErrorAsString());
		} else {
			ErrorBoxF(
				L"A transaction for this operation could not be created. %s",
				GetLastErrorAsString());
		}

		return;
	}

	//
	// Enable VxKex for each of the selected programs.
	//

	until (FileName[0] == '\0') {
		BOOLEAN Success;

		StringCchCopy(
			FileFullPath + DirectoryNameCch,
			ARRAYSIZE(FileFullPath) - DirectoryNameCch,
			FileName);

		Success = KxCfgSetConfiguration(
			FileFullPath,
			&Configuration,
			TransactionHandle);

		if (!Success) {
			if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
				ErrorBoxF(
					L"为“%s”应用设置时出现错误。%s",
					FileFullPath, GetLastErrorAsString());
			} else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
				ErrorBoxF(
					L"﻿為「%s」應用設定時出現錯誤。%s",
					FileFullPath, GetLastErrorAsString());
			} else {
				ErrorBoxF(
					L"There was an error applying settings for \"%s\". %s",
					FileFullPath, GetLastErrorAsString());
			}

			NtRollbackTransaction(TransactionHandle, TRUE);
			SafeClose(TransactionHandle);
			return;
		}

		FileName = FileName + wcslen(FileName) + 1;
	}

	NtCommitTransaction(TransactionHandle, TRUE);
	SafeClose(TransactionHandle);

	KexCfgGuiPopulateApplicationList();
}

STATIC VOID ProgramNotFoundUserPrompt(
	IN	PCWSTR	ProgramFullPath)
{
	INT UserResponse;
	if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
		UserResponse = MessageBoxF(
			TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
			NULL,
			FRIENDLYAPPNAME,
			L"无法找到所选程序",
			L"程序“%s”已被移动或删除。"
			L"您想从启用 VxKex 的应用程序列表中删除此条目吗？",
			ProgramFullPath);
	} else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
		UserResponse = MessageBoxF(
			TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
			NULL,
			FRIENDLYAPPNAME,
			L"無法找到所選程式",
			L"程式「%s」已被移動或刪除。"
			L"您想從啟用 VxKex 的應用程式列表中刪除此條目嗎？",
			ProgramFullPath);
	} else {
		UserResponse = MessageBoxF(
			TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
			NULL,
			FRIENDLYAPPNAME,
			L"The selected program cannot be found",
			L"The program \"%s\" was moved or deleted. "
			L"Would you like to remove this entry from the list of VxKex-NEXT-enabled applications?",
			ProgramFullPath);
	}

	if (UserResponse == IDYES) {
		RemoveSelectedPrograms();
	}
}

STATIC VOID OpenSelectedItemLocation(
	VOID)
{
	HRESULT Result;
	ULONG ItemIndex;
	PCWSTR ProgramFullPath;

	ASSERT (IsWindow(ListViewWindow));
	ASSERT (ListView_GetSelectedCount(ListViewWindow) == 1);

	ItemIndex = ListView_GetNextItem(ListViewWindow, -1, LVIS_SELECTED);
	ProgramFullPath = GetProgramFullPathFromListViewIndex(ItemIndex);

	Result = OpenFileLocation(ProgramFullPath, 0);

	if (FAILED(Result)) {
		if (Result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			//
			// Special case for file not found. We will ask the user whether he wants
			// to remove this entry since the EXE doesn't exist anymore.
			//

			ProgramNotFoundUserPrompt(ProgramFullPath);
		} else {
			// display generic error box
			if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ErrorBoxF(L"无法打开文件位置。%s", Win32ErrorAsString(Result));
			else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ErrorBoxF(L"﻿無法開啟檔案位置。%s", Win32ErrorAsString(Result));
			else ErrorBoxF(L"Couldn't open file location. %s", Win32ErrorAsString(Result));
		}
	}
}

STATIC VOID OpenSelectedItemProperties(
	VOID)
{
	BOOLEAN Success;
	PCWSTR ProgramFullPath;
	ULONG ItemIndex;

	ASSERT (ListView_GetSelectedCount(ListViewWindow) == 1);

	ItemIndex = ListView_GetNextItem(ListViewWindow, -1, LVIS_SELECTED);
	ProgramFullPath = GetProgramFullPathFromListViewIndex(ItemIndex);

	Success = ShowPropertiesDialog(ProgramFullPath, SW_SHOW, FALSE);
	if (!Success) {
		ULONG ErrorCode;

		ErrorCode = GetLastError();

		if (ErrorCode == ERROR_FILE_NOT_FOUND) {
			ProgramNotFoundUserPrompt(ProgramFullPath);
		} else {
			if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) ErrorBoxF(L"无法打开文件属性。%s", Win32ErrorAsString(ErrorCode));
			else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) ErrorBoxF(L"無法開啟檔案屬性。%s", Win32ErrorAsString(ErrorCode));
			else ErrorBoxF(L"Couldn't open file properties. %s", Win32ErrorAsString(ErrorCode));
		}
	}
}

STATIC VOID RunSelectedProgram(
	VOID)
{
	PCWSTR ProgramFullPath;
	PCWSTR ErrorMessage;
	ULONG_PTR ErrorCode;
	ULONG ItemIndex;

	ItemIndex = ListView_GetNextItem(ListViewWindow, -1, LVIS_SELECTED);
	ProgramFullPath = GetProgramFullPathFromListViewIndex(ItemIndex);

	//
	// Check if CPIW bypass has been applied from a previous call to this function.
	// If not, load cpiwbypa so that we have the ability to run programs with a
	// subsystem version higher than 6.1.
	//

	if (!(NtCurrentPeb()->SpareBits0 & 1)) {
		LoadLibrary(L"cpiwbypa.dll");
		ASSERT (NtCurrentPeb()->SpareBits0 & 1);
	}

	// why ShellExecute and not CreateProcess? because we can have .msi files too.
	ErrorCode = (ULONG_PTR) ShellExecute(
		MainWindow,
		L"open",
		ProgramFullPath,
		NULL,
		NULL,
		SW_SHOWDEFAULT);

	ErrorMessage = NULL;

	if (ErrorCode <= 32) {
		if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
			switch (ErrorCode) {
				case SE_ERR_FNF:
				case SE_ERR_PNF:
					ProgramNotFoundUserPrompt(ProgramFullPath);
					break;
				case SE_ERR_ACCESSDENIED:
					ErrorMessage = L"访问被拒绝或可执行文件格式无效。";
					break;
				case SE_ERR_OOM:
					ErrorMessage = L"内存不足，无法完成操作。";
					break;
				case SE_ERR_SHARE:
					ErrorMessage = L"发生违规共享。";
					break;
				case SE_ERR_ASSOCINCOMPLETE:
					ErrorMessage = L"SE_ERR_ASSOCINCOMPLETE";
					break;
				case SE_ERR_DDETIMEOUT:
					ErrorMessage = L"SE_ERR_DDETIMEOUT";
					break;
				case SE_ERR_DDEFAIL:
					ErrorMessage = L"SE_ERR_DDEFAIL";
					break;
				case SE_ERR_DDEBUSY:
					ErrorMessage = L"SE_ERR_DDEBUSY";
					break;
				case SE_ERR_NOASSOC:
					ErrorMessage = L"SE_ERR_NOASSOC";
					break;
				case SE_ERR_DLLNOTFOUND:
					ErrorMessage = L"未找到指定的 DLL。";
					break;
				default:
					ErrorMessage = L"发生未知错误。";
					break;
			}
		} else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
			switch (ErrorCode) {
				case SE_ERR_FNF:
				case SE_ERR_PNF:
					ProgramNotFoundUserPrompt(ProgramFullPath);
					break;
				case SE_ERR_ACCESSDENIED:
					ErrorMessage = L"存取被拒絕或可執行檔案格式無效。";
					break;
				case SE_ERR_OOM:
					ErrorMessage = L"內存不足，無法完成操作。";
					break;
				case SE_ERR_SHARE:
					ErrorMessage = L"發生違規共享。";
					break;
				case SE_ERR_ASSOCINCOMPLETE:
					ErrorMessage = L"SE_ERR_ASSOCINCOMPLETE";
					break;
				case SE_ERR_DDETIMEOUT:
					ErrorMessage = L"SE_ERR_DDETIMEOUT";
					break;
				case SE_ERR_DDEFAIL:
					ErrorMessage = L"SE_ERR_DDEFAIL";
					break;
				case SE_ERR_DDEBUSY:
					ErrorMessage = L"SE_ERR_DDEBUSY";
					break;
				case SE_ERR_NOASSOC:
					ErrorMessage = L"SE_ERR_NOASSOC";
					break;
				case SE_ERR_DLLNOTFOUND:
					ErrorMessage = L"未找到指定的 DLL。";
					break;
				default:
					ErrorMessage = L"發生未知錯誤。";
					break;
			}
		} else {
			switch (ErrorCode) {
				case SE_ERR_FNF:
				case SE_ERR_PNF:
					ProgramNotFoundUserPrompt(ProgramFullPath);
					break;
				case SE_ERR_ACCESSDENIED:
					ErrorMessage = L"Access was denied or the executable file format is invalid.";
					break;
				case SE_ERR_OOM:
					ErrorMessage = L"There was not enough memory to complete the operation.";
					break;
				case SE_ERR_SHARE:
					ErrorMessage = L"A sharing violation occurred.";
					break;
				case SE_ERR_ASSOCINCOMPLETE:
					ErrorMessage = L"SE_ERR_ASSOCINCOMPLETE";
					break;
				case SE_ERR_DDETIMEOUT:
					ErrorMessage = L"SE_ERR_DDETIMEOUT";
					break;
				case SE_ERR_DDEFAIL:
					ErrorMessage = L"SE_ERR_DDEFAIL";
					break;
				case SE_ERR_DDEBUSY:
					ErrorMessage = L"SE_ERR_DDEBUSY";
					break;
				case SE_ERR_NOASSOC:
					ErrorMessage = L"SE_ERR_NOASSOC";
					break;
				case SE_ERR_DLLNOTFOUND:
					ErrorMessage = L"The specified DLL was not found.";
					break;
				default:
					ErrorMessage = L"An unknown error has occurred.";
					break;
			}
		}
	}

	if (ErrorMessage) {
		ErrorBoxF(L"\"%s\": %s", ProgramFullPath, ErrorMessage);
	}
}

STATIC VOID HandleListViewContextMenu(
	IN	PPOINT	ClickPoint)
{
	HWND HeaderWindow;
	RECT HeaderWindowRect;
	LVHITTESTINFO HitTestInfo;
	ULONG ItemIndex;
	USHORT MenuId;
	INT DefaultMenuItem;
	ULONG MenuSelection;
	ULONG ListViewSelectedCount;

	HeaderWindow = ListView_GetHeader(ListViewWindow);
	GetWindowRect(HeaderWindow, &HeaderWindowRect);
	
	if (PtInRect(&HeaderWindowRect, *ClickPoint)) {
		// The user has right clicked on the list view header.
		return;
	}

	HitTestInfo.pt = *ClickPoint;
	ScreenToClient(ListViewWindow, &HitTestInfo.pt);
	ItemIndex = ListView_HitTest(ListViewWindow, &HitTestInfo);
	ListViewSelectedCount = ListView_GetSelectedCount(ListViewWindow);
	DefaultMenuItem = -1;

	if (ItemIndex == -1) {
		// The user has right clicked on a blank space in the list view.
		MenuId = IDM_LISTVIEWBLANKSPACEMENU;
	} else {
		// The user has right clicked on one or more items.
		if (ListViewSelectedCount == 1) {
			MenuId = IDM_LISTVIEWITEMMENU;
			DefaultMenuItem = M_OPENFILELOCATION;
		} else if (ListViewSelectedCount > 1) {
			// more than 1 item
			MenuId = IDM_LISTVIEWMULTIITEMMENU;
		} else {
			// can happen if Ctrl key is held down.
			// in this situation, programs like Windows Explorer will display
			// the same menu as for right clicking on blank space.
			MenuId = IDM_LISTVIEWBLANKSPACEMENU;
		}
	}

	MenuSelection = ContextMenuEx(ListViewWindow, MenuId, ClickPoint, DefaultMenuItem);
	if (!MenuSelection) {
		return;
	}
	
	if (MenuSelection == M_OPENFILELOCATION) {
		ASSERT (ListViewSelectedCount == 1);
		OpenSelectedItemLocation();
	} else if (MenuSelection == M_RUNPROGRAM) {
		ASSERT (ListViewSelectedCount == 1);
		RunSelectedProgram();
	} else if (MenuSelection == M_PROPERTIES) {
		ASSERT (ListViewSelectedCount == 1);
		OpenSelectedItemProperties();
	} else if (MenuSelection == M_REMOVE) {
		RemoveSelectedPrograms();
	} else if (MenuSelection == M_ADDPROGRAM) {
		AddProgram();
	}
}

STATIC INT CALLBACK KexCfgGuiSortListViewItems(
	IN	LPARAM	LParam1,
	IN	LPARAM	LParam2,
	IN	LPARAM	ColumnIndex)
{
	WCHAR Text1[MAX_PATH];
	WCHAR Text2[MAX_PATH];
	ULONG Length1;
	ULONG Length2;
	INT ComparisonResult;
	
	ListView_GetItemTextEx(ListViewWindow, LParam1, (INT) ColumnIndex, Text1, ARRAYSIZE(Text1), &Length1);
	ListView_GetItemTextEx(ListViewWindow, LParam2, (INT) ColumnIndex, Text2, ARRAYSIZE(Text2), &Length2);

	ComparisonResult = CompareString(
		LOCALE_USER_DEFAULT,
		0,
		Text1,
		Length1,
		Text2,
		Length2);

	ASSERT (ComparisonResult != 0);
	ComparisonResult -= 2;

	ASSERT (ColumnIndex < ARRAYSIZE(SortOrderIsDescendingForColumn));

	if (SortOrderIsDescendingForColumn[ColumnIndex]) {
		ComparisonResult = -ComparisonResult;
	}

	return ComparisonResult;
}

STATIC INT_PTR CALLBACK DialogProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	if (Message == WM_INITDIALOG) {
		HMODULE Uxtheme;
		HRESULT (WINAPI *pEnableThemeDialogTexture) (HWND, DWORD);
		BOOLEAN Success;
		HWND ComboBoxWindow;
		HWND LogDirWindow;
		LVCOLUMN Column;
		HIMAGELIST SystemSmallImageList = NULL;
		HIMAGELIST SystemLargeImageList = NULL;

		MainWindow = Window;
		KexgApplicationMainWindow = Window;
		SetWindowIcon(Window, IDI_APPICON);

		Uxtheme = LoadLibrary(L"uxtheme.dll");
		pEnableThemeDialogTexture = (HRESULT (WINAPI *) (HWND, DWORD)) GetProcAddress(Uxtheme, "EnableThemeDialogTexture");
		if (pEnableThemeDialogTexture) pEnableThemeDialogTexture(Window, ETDT_ENABLE | ETDT_USEAEROWIZARDTABTEXTURE);
		FreeLibrary(Uxtheme);

		//
		// Limit the max length of the log directory path to 200 characters.
		// Make the path edit box autocomplete paths.
		//

		LogDirWindow = GetDlgItem(Window, IDC_LOGDIR);
		Edit_LimitText(LogDirWindow, 200);
		SHAutoComplete(LogDirWindow, SHACF_FILESYS_DIRS | SHACF_USETAB);

		//
		// Initialize the context menu selection combo box.
		//

		ComboBoxWindow = GetDlgItem(Window, IDC_WHICHCONTEXTMENU);
		if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
			ComboBox_AddString(ComboBoxWindow, L"扩展上下文菜单");
			ComboBox_AddString(ComboBoxWindow, L"常规上下文菜单");
		} else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
			ComboBox_AddString(ComboBoxWindow, L"擴展上下文功能表");
			ComboBox_AddString(ComboBoxWindow, L"常規上下文功能表");
		} else {
			ComboBox_AddString(ComboBoxWindow, L"extended context menu");
			ComboBox_AddString(ComboBoxWindow, L"normal context menu");
		}
		ComboBox_SetCurSel(ComboBoxWindow, 0);

		//
		// Initialize the application list view.
		//

		ListViewWindow = GetDlgItem(Window, IDC_LISTVIEW);
		SetWindowTheme(ListViewWindow, L"Explorer", NULL);

		Success = Shell_GetImageLists(&SystemLargeImageList, &SystemSmallImageList);
		ASSERT (Success);

		if (Success) {
			ListView_SetImageList(ListViewWindow, SystemSmallImageList, LVSIL_SMALL);
			ListView_SetImageList(ListViewWindow, SystemLargeImageList, LVSIL_NORMAL);
		}

		ListView_SetExtendedListViewStyle(
			ListViewWindow,
			LVS_EX_AUTOCHECKSELECT |
			LVS_EX_CHECKBOXES |
			LVS_EX_FULLROWSELECT |
			LVS_EX_LABELTIP |
			LVS_EX_DOUBLEBUFFER);

		RtlZeroMemory(&Column, sizeof(Column));
		Column.mask = LVCF_TEXT | LVCF_WIDTH;
		
		if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) Column.pszText = L"应用程序";
		else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) Column.pszText = L"應用程式";
		else Column.pszText = L"Application";
		Column.cx = DpiScaleX(100);
		ListView_InsertColumn(ListViewWindow, 0, &Column);
		
		if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) Column.pszText = L"包含文件夹";
		else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) Column.pszText = L"﻿﻿包含資料夾";
		else Column.pszText = L"Containing folder";
		ListView_InsertColumn(ListViewWindow, 1, &Column);
		ListView_SetColumnWidth(ListViewWindow, 1, LVSCW_AUTOSIZE_USEHEADER);

		//
		// Add tool tips.
		//
		if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
			ToolTip(Window, IDC_ENABLELOGGING,
				L"如果启用了日志记录功能，每当您运行启用了 VxKex 的应用程序时，VxKex 都会在指定文件夹中创建日志文件。");
			ToolTip(Window, IDC_ENABLEFORMSI,
				L"此选项允许 VxKex 与 MSI 安装程序一起工作。\r\n"
				L"如果在使用 MSI 安装程序时遇到意外问题，请尝试禁用此选项。");
			ToolTip(Window, IDC_CPIWBYPA,
				L"此选项会在启动时将一个 DLL 加载到 Windows 资源管理器中，以移除某些程序的版本检查。\r\n"
				L"如果在使用 Windows 资源管理器时遇到意外问题，请尝试禁用此选项。");
			ToolTip(Window, IDC_ADDTOMENU,
				L"在 .exe 和 .msi 文件的上下文菜单中添加“使用 VxKex 运行”选项。");
			ToolTip(Window, IDC_WHICHCONTEXTMENU,
				L"按住 Shift 键并右键单击 .exe 或 .msi 文件可打开扩展上下文菜单。\r\n"
				L"在不按住 Shift 键的情况下右键单击可打开常规上下文菜单。");
		} else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
			ToolTip(Window, IDC_ENABLELOGGING,
				L"如果啟用了日誌記錄功能，每當您執行啟用了 VxKex 的應用程式時，VxKex 都會在指定﻿資料夾中創建日誌檔案。");
			ToolTip(Window, IDC_ENABLEFORMSI,
				L"此選項允許 VxKex 與 MSI 安裝程式一起工作。\r\n"
				L"如果在使用 MSI 安裝程式時遇到意外問題，請嘗試停用此選項。");
			ToolTip(Window, IDC_CPIWBYPA,
				L"此選項會在啟動時將一個 DLL 載入到 Windows 資源管理器中，以移除某些程式的版本檢查。\r\n"
				L"如果在使用 Windows 資源管理器時遇到意外問題，請嘗試停用此選項。");
			ToolTip(Window, IDC_ADDTOMENU,
				L"在 .exe 和 .msi 檔案的上下文功能表中添加「使用 VxKex 執行」選項。");
			ToolTip(Window, IDC_WHICHCONTEXTMENU,
				L"按住 Shift 鍵並右鍵單擊 .exe 或 .msi 檔案可開啟擴展上下文功能表。\r\n"
				L"在不按住 Shift 鍵的情況下右鍵單擊可開啟常規上下文功能表。");
		} else {
			ToolTip(Window, IDC_ENABLELOGGING,
				L"If you enable logging, VxKex will create log files in the specified "
				L"folder every time you run an application which has VxKex enabled.");
			ToolTip(Window, IDC_ENABLEFORMSI,
				L"This option allows VxKex to work with MSI installers.\r\n"
				L"If you encounter unexpected problems with MSI installers, try disabling this option.");
			ToolTip(Window, IDC_CPIWBYPA,
				L"This option causes a DLL to be loaded into Windows Explorer at startup in order "
				L"to remove the version check for certain programs.\r\n"
				L"If you encounter unexpected problems with Windows Explorer, try disabling this "
				L"option.");
			ToolTip(Window, IDC_ADDTOMENU,
				L"Add \"Run with VxKex\" options to the context menu for .exe and .msi files.");
			ToolTip(Window, IDC_WHICHCONTEXTMENU,
				L"Shift + Right Click on a .exe or .msi file opens the extended context menu.\r\n"
				L"Right clicking without holding the Shift key opens the normal context menu.");
		}

		//
		// Populate the top section (global configuration) and the apps list.
		//

		KexCfgGuiPopulateGlobalConfiguration();
		KexCfgGuiPopulateApplicationList();

		//
		// Update the state of certain interdependent controls (i.e. controls
		// that are supposed to be enabled/disabled based on the state of a
		// checkbox)
		//

		DialogProc(Window, WM_COMMAND, IDC_ENABLELOGGING, 0);
		DialogProc(Window, WM_COMMAND, IDC_ADDTOMENU, 0);

		UnsavedChanges = FALSE;
		EnableWindow(GetDlgItem(Window, IDC_APPLY), FALSE);
	} else if (Message == WM_COMMAND) {
		ULONG ControlId;
		ULONG NotificationCode;

		ControlId = LOWORD(WParam);
		NotificationCode = HIWORD(WParam);

		if (ControlId == IDC_ENABLELOGGING ||
			ControlId == IDC_ENABLEFORMSI ||
			ControlId == IDC_CPIWBYPA ||
			ControlId == IDC_ADDTOMENU ||
			(ControlId == IDC_WHICHCONTEXTMENU && NotificationCode == CBN_SELCHANGE) ||
			(ControlId == IDC_LOGDIR && NotificationCode == EN_CHANGE)) {

			if (UnsavedChanges == FALSE) {
				UnsavedChanges = TRUE;
				EnableWindow(GetDlgItem(Window, IDC_APPLY), TRUE);
			}
		}

		if (ControlId == IDC_CANCEL) {
			if (UnsavedChanges) {
				INT UserResponse;

				if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
					UserResponse = MessageBoxF(
						TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
						0,
						FRIENDLYAPPNAME,
						L"您确定要退出吗？",
						L"未应用的更改将被丢弃。");
				} else if (CURRENTLANG == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
					UserResponse = MessageBoxF(
						TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
						0,
						FRIENDLYAPPNAME,
						L"您確定要退出嗎？",
						L"未應用的更改將被丟棄。");
				} else {
					UserResponse = MessageBoxF(
						TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
						0,
						FRIENDLYAPPNAME,
						L"Are you sure you want to exit?",
						L"Changes that are not applied will be discarded.");
				}

				if (UserResponse == IDYES) {
					EndDialog(Window, 0);
				}
			} else {
				EndDialog(Window, 0);
			}
		} else if (ControlId == IDC_OK) {
			if (UnsavedChanges) {
				KexCfgGuiApplyGlobalConfiguration();
			}

			unless (UnsavedChanges) {
				EndDialog(Window, 0);
			}
		} else if (ControlId == IDC_APPLY) {
			if (UnsavedChanges) {
				KexCfgGuiApplyGlobalConfiguration();
			}

			unless (UnsavedChanges) {
				EnableWindow(GetDlgItem(Window, ControlId), FALSE);
			}
		} else if (ControlId == IDC_BROWSELOGDIR) {
			BOOLEAN Success;
			WCHAR NewLogDir[MAX_PATH];

			GetDlgItemText(Window, IDC_LOGDIR, NewLogDir, ARRAYSIZE(NewLogDir));
			Success = PickFolder(Window, NULL, 0, NewLogDir, ARRAYSIZE(NewLogDir));

			if (Success) {
				SetDlgItemText(Window, IDC_LOGDIR, NewLogDir);
			}
		} else if (ControlId == IDC_ENABLELOGGING) {
			BOOLEAN LoggingEnabled;

			LoggingEnabled = IsDlgButtonChecked(Window, ControlId);

			EnableWindow(GetDlgItem(Window, IDC_LOGDIR), LoggingEnabled);
			EnableWindow(GetDlgItem(Window, IDC_BROWSELOGDIR), LoggingEnabled);
		} else if (ControlId == IDC_ADDTOMENU) {
			EnableWindow(
				GetDlgItem(Window, IDC_WHICHCONTEXTMENU),
				IsDlgButtonChecked(Window, ControlId));
		} else if (ControlId == IDC_NEWAPP) {
			AddProgram();
		} else if (ControlId == IDC_REMOVEAPPS) {
			RemoveSelectedPrograms();
		} else if (ControlId == IDC_PROPERTIES) {
			ULONG ItemIndex;

			ItemIndex = ListView_GetNextItem(ListViewWindow, -1, LVIS_SELECTED);

			ShowPropertiesDialog(
				GetProgramFullPathFromListViewIndex(ItemIndex),
				SW_SHOWDEFAULT,
				FALSE);
		} else {
			return FALSE;
		}
	} else if (Message == WM_NOTIFY) {
		LPNMHDR Notification;

		Notification = (LPNMHDR) LParam;

		if (Notification->idFrom == IDC_LISTVIEW) {
			if (Notification->code == LVN_ITEMCHANGED) {
				ULONG ListViewSelectedCount;

				ListViewSelectedCount = ListView_GetSelectedCount(ListViewWindow);

				if (ListViewSelectedCount == 0) {
					// no items selected
					EnableWindow(GetDlgItem(Window, IDC_REMOVEAPPS), FALSE);
					EnableWindow(GetDlgItem(Window, IDC_PROPERTIES), FALSE);
				} else {
					// one or more items selected
					EnableWindow(GetDlgItem(Window, IDC_REMOVEAPPS), TRUE);

					if (ListViewSelectedCount == 1) {
						EnableWindow(GetDlgItem(Window, IDC_PROPERTIES), TRUE);
					} else {
						// no properties button for >1 item selected
						EnableWindow(GetDlgItem(Window, IDC_PROPERTIES), FALSE);
					}
				}
			} else if (Notification->code == LVN_DELETEITEM) {
				// after items are deleted, no items will be selected anymore
				EnableWindow(GetDlgItem(Window, IDC_REMOVEAPPS), FALSE);
				EnableWindow(GetDlgItem(Window, IDC_PROPERTIES), FALSE);
			} else if (Notification->code == NM_DBLCLK) {
				//
				// User double clicked on a list-view item.
				// We will open the location of that file.
				//

				OpenSelectedItemLocation();
			} else if (Notification->code == LVN_COLUMNCLICK) {
				ULONG ColumnIndex;

				ColumnIndex = ((LPNMLISTVIEW) LParam)->iSubItem;

				// toggle sort order
				ASSERT (ColumnIndex < ARRAYSIZE(SortOrderIsDescendingForColumn));
				SortOrderIsDescendingForColumn[ColumnIndex] ^= 1;

				// User has clicked on a column header.
				// We are now going to sort that column.
				ListView_SortItemsEx(
					ListViewWindow,
					KexCfgGuiSortListViewItems,
					ColumnIndex);
			}
		} else {
			return FALSE;
		}
	} else if (Message == WM_CONTEXTMENU) {
		HWND ContextMenuWindow;

		ContextMenuWindow = (HWND) WParam;

		if (ContextMenuWindow == ListViewWindow) {
			POINT ClickPoint;
			ClickPoint.x = GET_X_LPARAM(LParam);
			ClickPoint.y = GET_Y_LPARAM(LParam);
			HandleListViewContextMenu(&ClickPoint);
		}
	} else if (Message == WM_CLOSE) {
		DialogProc(Window, WM_COMMAND, IDC_CANCEL, 0);
	} else {
		return FALSE;
	}

	return TRUE;
}

VOID KexCfgOpenGUI(
	VOID)
{
	if (!IsUserAnAdmin()) {
		//
		// Elevate the application.
		// Many of the settings that can be changed here are located under the HKLM
		// registry key so we'll require admin to run this program.
		//

		ShellExecute(
			NULL,
			L"runas",
			NtCurrentPeb()->ProcessParameters->ImagePathName.Buffer,
			NULL,
			NULL,
			SW_SHOWDEFAULT);

		return;
	}

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	KexgApplicationFriendlyName = FRIENDLYAPPNAME;
	DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DialogProc);
}