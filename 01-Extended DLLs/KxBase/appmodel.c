#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI LONG WINAPI GetCurrentPackageFamilyName(
	IN OUT	PULONG	NameLength,
	OUT		PWSTR	PackageFamilyName OPTIONAL)
{
	if (!NameLength || *NameLength && !PackageFamilyName) {
		return ERROR_INVALID_PARAMETER;
	}

	*NameLength = 0;

	if (PackageFamilyName) {
		PackageFamilyName[0] = '\0';
	}

	return APPMODEL_ERROR_NO_PACKAGE;
}

KXBASEAPI LONG WINAPI GetCurrentPackageFullName(
	IN OUT	PULONG	PackageFullNameLength,
	OUT		PWSTR	PackageFullName OPTIONAL)
{
	if (!PackageFullNameLength) {
		return ERROR_INVALID_PARAMETER;
	}

	if (*PackageFullNameLength != 0 && !PackageFullName) {
		return ERROR_INVALID_PARAMETER;
	}

	return APPMODEL_ERROR_NO_PACKAGE;
}

KXBASEAPI LONG WINAPI GetCurrentPackageId(
	IN OUT	PULONG	BufferLength,
	OUT		PBYTE	Buffer OPTIONAL)
{
	if (!BufferLength) {
		return ERROR_INVALID_PARAMETER;
	}

	if (*BufferLength != 0 && !Buffer) {
		return ERROR_INVALID_PARAMETER;
	}

	return APPMODEL_ERROR_NO_PACKAGE;
}

KXBASEAPI LONG WINAPI GetPackageFullName(
	IN		HANDLE	ProcessHandle,
	IN OUT	PULONG	NameLength,
	OUT		PWSTR	PackageFullName OPTIONAL)
{
	if (!ProcessHandle || !NameLength || *NameLength && !PackageFullName) {
		return ERROR_INVALID_PARAMETER;
	}

	*NameLength = 0;

	if (PackageFullName) {
		PackageFullName[0] = '\0';
	}

	return APPMODEL_ERROR_NO_PACKAGE;
}

KXBASEAPI LONG WINAPI GetPackageFamilyName(
	IN		HANDLE	ProcessHandle,
	IN OUT	PULONG	NameLength,
	OUT		PWSTR	PackageFamilyName OPTIONAL)
{
	if (!ProcessHandle || !NameLength || *NameLength && !PackageFamilyName) {
		return ERROR_INVALID_PARAMETER;
	}

	*NameLength = 0;

	if (PackageFamilyName) {
		PackageFamilyName[0] = '\0';
	}

	return APPMODEL_ERROR_NO_PACKAGE;
}

KXBASEAPI LONG WINAPI GetPackagePathByFullName(
	IN		PCWSTR	PackageFullName,
	IN	OUT	UINT32	*PathLength,
	OUT		PWSTR	PackagePath	OPTIONAL)
{
	if (!PackageFullName || !PathLength || *PathLength && !PackagePath) {
		return ERROR_INVALID_PARAMETER;
	}

	*PathLength = 0;

	if (PackagePath) {
		PackagePath[0] = '\0';
	}

	return APPMODEL_ERROR_NO_PACKAGE;
}

KXBASEAPI LONG WINAPI GetPackagesByPackageFamily(
	IN		PCWSTR	PackageFamilyName,
	IN OUT	PULONG	Count,
	OUT		PPWSTR	PackageFullNames OPTIONAL,
	IN OUT	PULONG	BufferLength,
	OUT		PPWSTR	Buffer OPTIONAL)
{
	if (!Count || !BufferLength) {
		return ERROR_INVALID_PARAMETER;
	}

	*Count = 0;
	*BufferLength = 0;

	return ERROR_SUCCESS;
}

KXBASEAPI LONG WINAPI PackageIdFromFullName(
	IN PCWSTR packageFullName,
	IN const UINT32 flags,
	IN OUT UINT32 *bufferLength,
	OUT OPTIONAL BYTE *buffer)
{
	if(!bufferLength || !buffer) {
		return ERROR_INVALID_PARAMETER;
	}

	bufferLength = 0;
	buffer = 0;
	
	return ERROR_NOT_FOUND;
}

KXBASEAPI LONG WINAPI GetPackagePath(
	IN const PACKAGE_ID *packageID,
	const UINT32 reserved,
	IN OUT UINT32 *pathLength,
	OUT OPTIONAL PWSTR path)
{
	if(!pathLength || !path) {
		return ERROR_INVALID_PARAMETER;
	}

	pathLength = 0;
	path = 0;

	return ERROR_NOT_FOUND;
}

KXBASEAPI LONG WINAPI AppPolicyGetProcessTerminationMethod(
	IN	HANDLE	ProcessToken,
	OUT	PULONG	Policy)
{
	if (!ProcessToken || !Policy) {
		return RtlNtStatusToDosError(STATUS_INVALID_PARAMETER);
	}

	*Policy = 0;
	return ERROR_SUCCESS;
}

KXBASEAPI HRESULT WINAPI AppXGetPackageSid(
	IN	PCWSTR	PackageMoniker,
	OUT	PSID	*PackageSid)
{
	KexLogWarningEvent(L"AppXGetPackageSid called: %s", PackageMoniker);
	KexDebugCheckpoint();

	if (!PackageMoniker || !PackageSid) {
		return E_INVALIDARG;
	}

	return E_NOTIMPL;
}

KXBASEAPI VOID WINAPI AppXFreeMemory(
	IN	PVOID	Pointer)
{
	RtlFreeHeap(RtlProcessHeap(), 0, Pointer);
}

KXBASEAPI ULONG WINAPI PackageFamilyNameFromFullName(
	IN		PCWSTR	PackageFullName,
	IN OUT	PULONG	PackageFamilyNameLength,
	OUT		PWSTR	PackageFamilyName)
{
	if (!PackageFullName || !PackageFamilyNameLength) {
		return ERROR_INVALID_PARAMETER;
	}

	if (*PackageFamilyNameLength != 0 && !PackageFamilyName) {
		return ERROR_INVALID_PARAMETER;
	}

	if (*PackageFamilyNameLength != 0) {
		PackageFamilyName[0] = '\0';
	}

	*PackageFamilyNameLength = 0;
	return ERROR_SUCCESS;
}

KXBASEAPI LONG WINAPI GetApplicationUserModelId(
	IN		HANDLE	ProcessHandle,
	IN OUT	PULONG	ApplicationUserModelIdLength,
	OUT		PWSTR	ApplicationUserModelId)
{
	return APPMODEL_ERROR_NO_APPLICATION;
}

KXBASEAPI LONG WINAPI GetCurrentApplicationUserModelId(
	IN OUT	PULONG	Cch,
	OUT		PWSTR	Buffer)
{
	return APPMODEL_ERROR_NO_APPLICATION;
}

KXBASEAPI LONG WINAPI AppPolicyGetWindowingModel(
	IN	HANDLE						ProcessToken,
	OUT	PAPP_POLICY_WINDOWING_MODEL	WindowingModel)
{
	if (!ProcessToken || !WindowingModel) {
		return ERROR_INVALID_PARAMETER;
	}

	*WindowingModel = AppPolicyWindowingModel_ClassicDesktop;
	return ERROR_SUCCESS;
}

KXBASEAPI LONG WINAPI AppPolicyGetThreadInitializationType(
	IN	HANDLE									ProcessToken,
	OUT	PAPP_POLICY_THREAD_INITIALIZATION_TYPE	InitializationType)
{
	if (!ProcessToken || !InitializationType) {
		return ERROR_INVALID_PARAMETER;
	}

	*InitializationType = AppPolicyThreadInitializationType_None;
	return ERROR_SUCCESS;
}