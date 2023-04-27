#ifndef _DRIVER_H
#define _DRIVER_H

#include <ntifs.h>
#include <ntstrsafe.h>
#include <wdm.h>

// Security parameters for driver full access for SY and full access for
// local group Admintistrators
#define DRIVER_SDDL_STRING L"D:P(A;;GA;;;SY)(A;;GA;;BA)"
#define DRIVER_SCM_NAME L"\\Device\\" DRIVER_NAME
#define USER_MODE_DRIVER_NAME L"\\??\\" DRIVER_NAME

#define DRIVER_ALTITUDE L"380000"
#define LOG_FILE_NAME L"\\SystemRoot\MBKS\\ImageNotify.log"

#define CONSOLE_PROGRAM_NAME L"ConsoleApp.exe"
#define DRIVER_NAME_WITH_EXT DRIVER_NAME L".sys"

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

_Dispatch_type_(IRP_MJ_CREATE)          DRIVER_DISPATCH DriverCreate;
_Dispatch_type_(IRP_MJ_CLOSE)           DRIVER_DISPATCH DriverClose;
_Dispatch_type_(IRP_MJ_CLEANUP)         DRIVER_DISPATCH DriverCleanup;
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)  DRIVER_DISPATCH DriverControl;

NTSTATUS RegistryCallback(
  __in PVOID __callbackContext,
  __in_opt PVOID __arg1,
  __in_opt PVOID __arg2
);

typedef NTSTATUS(*QUERY_INFO_PROCESS) (
    __in HANDLE __procHandle,
    __in PROCESSINFOCLASS __procInfoClass,
    __out_bcount(__procInfoLen) PVOID __procInfo,
    __in ULONG __procInfoLen,
    __out_opt PULONG __retLen
  );
QUERY_INFO_PROCESS ZwQueryInformationProcess;

int SetLoadImageNotify();
int RemoveLoadImageNotify();
PLOAD_IMAGE_NOTIFY_ROUTINE LoadImageNotify;

void _PrintDebugError(const char* __funcName, NTSTATUS __errCode);
void _PrintDebugSuccess(const char* __funcName);

#endif // !_DRIVER_H
