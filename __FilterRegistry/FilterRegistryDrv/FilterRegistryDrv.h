#pragma once

#include <ntifs.h>
#include <ntstrsafe.h>
#include <wdmsec.h>

#include "common.h"


// Pool tags
#define REGFLTR_CONTEXT_POOL_TAG          '0tfR'
#define REGFLTR_CAPTURE_POOL_TAG          '1tfR'


#define InfoPrint(str, ...)                 \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID,         \
               DPFLTR_INFO_LEVEL,           \
               "%S: "##str"\n",             \
               DRIVER_NAME,                 \
               __VA_ARGS__)

#define ErrorPrint(str, ...)                \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID,         \
               DPFLTR_ERROR_LEVEL,          \
               "%S: %d: "##str"\n",         \
               DRIVER_NAME,                 \
               __LINE__,                    \
               __VA_ARGS__)

LPCWSTR
GetNotifyClassString(
    _In_ REG_NOTIFY_CLASS NotifyClass
);

// The root key used in the samples
extern HANDLE g_RootKey;


// Pointer to the device object used to register registry callbacks
extern PDEVICE_OBJECT g_DeviceObj;


// Registry callback version
extern ULONG g_MajorVersion;
extern ULONG g_MinorVersion;


// Set to TRUE if TM and RM were successfully created and the transaction
// callback was successfully enabled. 
extern BOOLEAN g_RMCreated;


// Flag that indicates if the system is win8 or higher. This is set on
// driver entry by calling RtlVerifyVersionInfo.
extern BOOLEAN g_IsWin8OrGreater;


// The following are variables used to manage callback contexts handed
// out to user mode.

#define MAX_CALLBACK_CTX_ENTRIES            10

// The fast mutex guarding the callback context list
extern FAST_MUTEX g_CallbackCtxListLock;

//
// The list head
//
extern LIST_ENTRY g_CallbackCtxListHead;

//
// Count of entries in list
//
extern USHORT g_NumCallbackCtxListEntries;

//
// Context data structure for the transaction callback RMCallback
//

typedef struct _RMCALLBACK_CONTEXT {

    //
    // A bit mask of all transaction notifications types that the RM Callback is 
    // notified of.
    //
    ULONG Notification;

    //
    // The handle to an enlistment
    //
    HANDLE Enlistment;
    
} RMCALLBACK_CONTEXT, *PRMCALLBACK_CONTEXT;


//
// The context data structure for the registry callback. It will be passed 
// to the callback function every time it is called. 
//

typedef struct _CALLBACK_CONTEXT {

    //
    // List of callback contexts currently active
    //
    LIST_ENTRY CallbackCtxList;

    //
    // Specifies which callback helper method to use
    //
    CALLBACK_MODE CallbackMode;

    //
    // Records the current ProcessId to filter out registry operation from
    // other processes.
    //
    HANDLE ProcessId;

    //
    // Records the altitude that the callback was registered at
    //
    UNICODE_STRING Altitude; 
    WCHAR AltitudeBuffer[MAX_ALTITUDE_BUFFER_LENGTH];
        
    //
    // Records the cookie returned by the registry when the callback was 
    // registered
    //
    LARGE_INTEGER Cookie;

    //
    // A pointer to the context for the transaction callback. 
    // Used to enlist on a transaction. Only used in the transaction samples.
    //
    PRMCALLBACK_CONTEXT RMCallbackCtx;

    //
    // These fields record information for verifying the behavior of the
    // certain samples. They are not used in all samples
    //
    
    //
    // Number of times the RegNtCallbackObjectContextCleanup 
    // notification was received
    //
    LONG ContextCleanupCount;

    //
    // Number of times the callback saw a notification with the call or
    // object context set correctly.
    //
    LONG NotificationWithContextCount;

    //
    // Number of times callback saw a notirication without call or without
    // object context set correctly
    //
    LONG NotificationWithNoContextCount;

    //
    // Number of pre-notifications received
    //
    LONG PreNotificationCount;

    //
    // Number of post-notifications received
    //
    LONG PostNotificationCount;
    
} CALLBACK_CONTEXT, *PCALLBACK_CONTEXT;


//
// The registry and transaction callback routines
//

EX_CALLBACK_FUNCTION Callback;

NTSTATUS  
RMCallback(
    _In_ PKENLISTMENT EnlistmentObject,
    _In_ PVOID RMContext,    
    _In_ PVOID TransactionContext,    
    _In_ ULONG TransactionNotification,    
    _Inout_ PLARGE_INTEGER TMVirtualClock,
    _In_ ULONG ArgumentLength,
    _In_ PVOID Argument
    );

//
// The samples and their corresponding callback helper methods
//

NTSTATUS 
CallbackPreNotificationBlock(
    _In_ PCALLBACK_CONTEXT CallbackCtx,
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
    );

NTSTATUS
CallbackMonitor(
    _In_ PCALLBACK_CONTEXT CallbackCtx,
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
    );

NTSTATUS
CallbackCapture(
    _In_ PCALLBACK_CONTEXT CallbackCtx,
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
    );

// Driver dispatch functions
//

NTSTATUS
RegisterCallback(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
    );

NTSTATUS
UnRegisterCallback(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
    );

NTSTATUS
GetCallbackVersion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
    );


//
// Config methods
//

VOID RegctrlReadCfg();
VOID RegctrlParseCfg(CHAR* cfg_buff);
VOID RegctrlPushProtectedFileCfg(UNICODE_STRING newstr);
VOID RegctrlDumpProtectedFileCfg();
VOID RegctrlDestroyProtectedFileCfg();
VOID RegctrlUpdateCfg();

typedef struct _PROTECTED_REGKEYS {
    UNICODE_STRING k_name;
    struct _PROTECTED_REGKEYS* k_next;
    struct _PROTECTED_REGKEYS* k_prev;
} PROTECTED_REGKEYS, * PPROTECTED_REGKEYS;

extern PPROTECTED_REGKEYS RegctrlProtectedKeys;


//
// Transaction related routines
//

NTSTATUS
CreateKTMResourceManager(
    _In_ PTM_RM_NOTIFICATION CallbackRoutine,
    _In_opt_ PVOID RMKey
    );

VOID
DeleteKTMResourceManager(
    );


//
// Capture methods
//

NTSTATUS
CaptureBuffer(
    _Outptr_result_maybenull_ PVOID *CapturedBuffer,
    _In_reads_bytes_(Length)PVOID Buffer, 
    _In_ SIZE_T Length, 
    _In_ ULONG PoolTag
    );

VOID
FreeCapturedBuffer(
    _In_ PVOID Buffer, 
    _In_ ULONG PoolTag
    );

NTSTATUS
CaptureUnicodeString(
    _Inout_ UNICODE_STRING * DestString, 
    _In_ PCUNICODE_STRING SourceString, 
    _In_ ULONG PoolTag
    );

VOID
FreeCapturedUnicodeString(
    _In_ UNICODE_STRING * String, 
    _In_ ULONG PoolTag
    );


//
// Utility methods
//

PVOID
CreateCallbackContext(
    _In_ CALLBACK_MODE CallbackMode, 
    _In_ PCWSTR AltitudeString
    );

BOOLEAN
InsertCallbackContext(
    _In_ PCALLBACK_CONTEXT CallbackCtx
    );

PCALLBACK_CONTEXT
FindCallbackContext(
    _In_ LARGE_INTEGER Cookie
    );

PCALLBACK_CONTEXT
FindAndRemoveCallbackContext(
    _In_ LARGE_INTEGER Cookie
    );

VOID
DeleteCallbackContext(
    _In_ PCALLBACK_CONTEXT CallbackCtx
    );


ULONG 
ExceptionFilter (
    _In_ PEXCEPTION_POINTERS ExceptionPointers
    );
    

