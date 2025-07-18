///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllmain.c
//
// Abstract:
//
//     Main file for KXBASE.
//     The DLLs "kernel32" and "kernelbase" are redirected to this DLL.
//
// Author:
//
//     vxiiduu (07-Nov-2022)
//
// Environment:
//
//     Win32 mode.
//     This module can import from NTDLL, KERNEL32, and KERNELBASE.
//
// Revision History:
//
//     vxiiduu              07-Nov-2022  Initial creation.
//     vxiiduu              10-Feb-2024  Rename to KXBASE (from kernel33).
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxbasep.h"
#include <KexW32ML.h>

PKEX_PROCESS_DATA KexData = NULL;
ULONG OriginalMajorVersion = 0, OriginalMinorVersion = 0, OriginalBuildNumber = 0;
DLL_DIRECTORY_DATA DllDirectoryData = {DLL_DIRECTORY_DATA_VERIFICATION_CODE, NULL, NULL, NULL};
ULONG DefaultDllDirectoryFlags = 0;

BOOL WINAPI DllMain(
	IN	PVOID		DllBase,
	IN	ULONG		Reason,
	IN	PCONTEXT	Context)
{
	if (Reason == DLL_PROCESS_ATTACH) {
		LdrDisableThreadCalloutsForDll(DllBase);

		KexDataInitialize(&KexData);
		KexRtlGetNtVersionNumbers(&OriginalMajorVersion, &OriginalMinorVersion, &OriginalBuildNumber);
		KexLogDebugEvent(L"DllMain called with DLL_PROCESS_ATTACH");

		//
		// Get the base address of Kernel32 and store it in KexData.
		//

		BaseGetBaseDllHandle();
		ASSERT (KexData->BaseDllBase != NULL);

		//
		// If we are doing SharedUserData-based version spoofing, we need to
		// hook GetSystemTime and GetSystemTimeAsFileTime.
		//

		if (KexData->IfeoParameters.StrongVersionSpoof & KEX_STRONGSPOOF_SHAREDUSERDATA) {
			KexHkInstallBasicHook(GetSystemTime, KxBasepGetSystemTimeHook, NULL);
			KexHkInstallBasicHook(GetSystemTimeAsFileTime, KxBasepGetSystemTimeAsFileTimeHook, NULL);
		}

		//
		// Patch subsystem version check inside CreateProcessInternalW.
		//

		KexPatchCpiwSubsystemVersionCheck();

		//
		// Get base named object directories and put handles to them in KexData.
		//

		if (OriginalMajorVersion == 6 && OriginalMinorVersion == 1) {
			KexData->BaseNamedObjects = BaseGetNamedObjectDirectory();
			KexData->UntrustedNamedObjects = BaseGetUntrustedNamedObjectDirectory();
		}
	}

	return TRUE;
}