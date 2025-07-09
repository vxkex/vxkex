///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ntthread.c
//
// Abstract:
//
//     Extended functions for dealing with threads.
//
// Author:
//
//     vxiiduu (07-Nov-2022)
//
// Revision History:
//
//     vxiiduu               07-Nov-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"
#include <KexComm.h>
#include <KexDll.h>
#include <KexTypes.h>
#include <NtDll.h>

typedef enum _TIMER_TYPE {
	NotificationTimer,
	SynchronizationTimer
} TIMER_TYPE;

typedef VOID (NTAPI *PTIMER_APC_ROUTINE)(
	IN PVOID TimerContext,
	IN ULONG TimerLowValue,
	IN LONG TimerHighValue);

// Forward declarations
NTSTATUS NTAPI TpSetTimerEx(
	IN PTP_TIMER     pti,
	IN PFILETIME     pftDueTime OPTIONAL,
	IN DWORD         msPeriod,
	IN DWORD         msWindowLength OPTIONAL);

NTSTATUS NTAPI NtCreateTimer(
	OUT PHANDLE             TimerHandle,
	IN ACCESS_MASK          DesiredAccess,
	IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
	IN TIMER_TYPE          TimerType);

NTSTATUS NTAPI NtSetTimer(
	IN HANDLE           TimerHandle,
	IN PLARGE_INTEGER   DueTime,
	IN PTIMER_APC_ROUTINE TimerApcRoutine OPTIONAL,
	IN PVOID            TimerContext OPTIONAL,
	IN BOOLEAN          ResumeTimer,
	IN LONG             Period OPTIONAL,
	OUT PBOOLEAN        PreviousState OPTIONAL);

NTSTATUS NTAPI Ext_NtQueryInformationThread(
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	OUT	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL) PROTECTED_FUNCTION
{
	NTSTATUS Status;

	if (ThreadInformationClass <= ThreadIdealProcessorEx) {

		//
		// fall through and call original NtQueryInformationThread
		//

		NOTHING;

	} else if (ThreadInformationClass == ThreadNameInformation) {
		OBJECT_BASIC_INFORMATION ThreadHandleInformation;

		//
		// Check for THREAD_QUERY_LIMITED_INFORMATION access.
		//

		Status = NtQueryObject(
			ThreadHandle,
			ObjectBasicInformation,
			&ThreadHandleInformation,
			sizeof(ThreadHandleInformation),
			NULL);

		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		unless (ThreadHandleInformation.GrantedAccess & THREAD_QUERY_LIMITED_INFORMATION) {
			return STATUS_ACCESS_DENIED;
		}

		//
		// TODO: Implement ThreadNameInformation here.
		//

		return STATUS_INVALID_INFO_CLASS;
	} else {
		KexLogWarningEvent(
			L"NtQueryInformationThread called with an unsupported extended information class %d",
			ThreadInformationClass);
	}

	return NtQueryInformationThread(
		ThreadHandle,
		ThreadInformationClass,
		ThreadInformation,
		ThreadInformationLength,
		ReturnLength);
} PROTECTED_FUNCTION_END

NTSTATUS NTAPI Ext_NtSetInformationThread(
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	IN	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength) PROTECTED_FUNCTION
{
	//
	// TODO: Implement ThreadNameInformation
	//

	return NtSetInformationThread(
		ThreadHandle,
		ThreadInformationClass,
		ThreadInformation,
		ThreadInformationLength);
} PROTECTED_FUNCTION_END

NTSTATUS NTAPI NtAlertThreadByThreadId(
	IN	HANDLE	UniqueThread) PROTECTED_FUNCTION
{
	// TODO
	return STATUS_NOT_IMPLEMENTED;
} PROTECTED_FUNCTION_END

NTSTATUS NTAPI NtWaitForAlertByThreadId(
	IN	PVOID		Hint,
	IN	PLONGLONG	Timeout) PROTECTED_FUNCTION
{
	// TODO
	return STATUS_NOT_IMPLEMENTED;
} PROTECTED_FUNCTION_END

// NTSTATUS NTAPI NtSetThreadpoolTimer(
// 	IN PTP_TIMER     pti,
// 	IN PFILETIME     pftDueTime OPTIONAL,
// 	IN DWORD         msPeriod,
// 	IN DWORD         msWindowLength OPTIONAL) PROTECTED_FUNCTION
// {
// 	// Forward to TpSetTimerEx
// 	return TpSetTimerEx(
// 		pti,
// 		pftDueTime,
// 		msPeriod,
// 		msWindowLength);
// } PROTECTED_FUNCTION_END

NTSTATUS NTAPI TpSetTimerEx(
	IN PTP_TIMER     pti,
	IN PFILETIME     pftDueTime OPTIONAL,
	IN DWORD         msPeriod,
	IN DWORD         msWindowLength OPTIONAL) PROTECTED_FUNCTION
{
	NTSTATUS Status = STATUS_SUCCESS;
	HANDLE TimerHandle = NULL;
	LARGE_INTEGER DueTime = {0};
	OBJECT_ATTRIBUTES ObjectAttributes = {0};

	// Basic parameter validation
	if (!pti) {
		return STATUS_INVALID_PARAMETER;
	}

	// Convert FILETIME to LARGE_INTEGER if provided
	if (pftDueTime) {
		DueTime.LowPart = pftDueTime->dwLowDateTime;
		DueTime.HighPart = pftDueTime->dwHighDateTime;
	}

	// Initialize object attributes
	InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

	// Create the timer
	Status = NtCreateTimer(
		&TimerHandle,
		TIMER_ALL_ACCESS,
		&ObjectAttributes,
		NotificationTimer);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	// Set the timer
	Status = NtSetTimer(
		TimerHandle,
		pftDueTime ? &DueTime : NULL,
		NULL,  // No APC routine
		NULL,  // No APC context
		FALSE, // Not resume
		msPeriod,
		NULL); // No previous state

	if (!NT_SUCCESS(Status)) {
		NtClose(TimerHandle);
		return Status;
	}

	return Status;
} PROTECTED_FUNCTION_END