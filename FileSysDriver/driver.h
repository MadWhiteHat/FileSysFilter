#ifndef _DRIVER_H
#define _DRIVER_H

#include <fltKernel.h>
#include <wdm.h>
#include <dontuse.h>
#include <suppress.h>

#include "../file_sys_filter.h"

#pragma prefast( \
  disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, \
  "Not valid for kernel mode drivers" \
)

#define LOG_FILE_NAME L"\\SystemRoot\\MBKS\\ImageNotify.log"
#define CONSOLE_PROGRAM_NAME L"ConsoleApp.exe"

NTSTATUS
DriverEntry(
  _In_ PDRIVER_OBJECT __driverObj,
  _In_ PUNICODE_STRING __registryPath
);


NTSTATUS
DriverUnload(
  _In_ FLT_FILTER_UNLOAD_FLAGS __flags
);

NTSTATUS
DriverInstanceQueryTeardown(
  _In_ PCFLT_RELATED_OBJECTS __fltObjects,
  _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS __flags
);

FLT_PREOP_CALLBACK_STATUS
FltPreDriverControl(
  _Inout_ PFLT_CALLBACK_DATA __data,
  _In_ PCFLT_RELATED_OBJECTS __fltObjects,
  _Out_ PVOID* __completionContext
);

int
SetLoadImageNotify();
int
RemoveLoadImageNotify();

void
LoadImageNotify(
  _In_opt_ PUNICODE_STRING FullImageName,
  _In_ HANDLE ProcessId,
  _In_ PIMAGE_INFO ImageInfo
);

void
PrintDebugError(const char* __funcName, NTSTATUS __errCode);
void
PrintDebugSuccess(const char* __funcName);
void
_PrintDebugStatus(const char* __fmt, ...);

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, DriverInstanceQueryTeardown)
#pragma alloc_text(PAGE, FltPreDriverControl)
#pragma alloc_text(PAGE, SetLoadImageNotify)
#pragma alloc_text(PAGE, RemoveLoadImageNotify)
#pragma alloc_text(PAGE, LoadImageNotify)
#pragma alloc_text(PAGE, PrintDebugError)
#pragma alloc_text(PAGE, PrintDebugSuccess)
#pragma alloc_text(PAGE, _PrintDebugStatus)

#endif // ALLOC_PRAGMA


#endif // !_DRIVER_H
