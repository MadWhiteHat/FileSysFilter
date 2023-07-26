#ifndef _DRIVER_H
#define _DRIVER_H

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

#include "../file_sys_filter.h"
#include "rules_list.h"
  
#pragma prefast( \
  disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, \
  "Not valid for kernel mode drivers" \
)
#pragma warning(error:4100) // Enable-Unreferenced formal parameter
#pragma warning(error:4101) // Enable-Unreferenced local variable
#pragma warning(error:4061) // Enable-missing enumeration in switch statement
#pragma warning(error:4505) // Enable-identify dead functions

#if DBG

#define PRINT_STATUS(FMT, ...) \
  DbgPrint("[FileSysFilter]: " FMT "\n", __VA_ARGS__);

#else

#define PRINT_STATUS(FMT, ...) { NOTHING; }

#endif //DBG

#define PRINT_ERROR(FUNC, ERR_CODE) \
  PRINT_STATUS("%s failed with 0x%08x", FUNC, ERR_CODE);
#define PRINT_SUCCESS(FUNC) \
  PRINT_STATUS("%s SUCCESS", FUNC);

#define LOG_FILE_NAME L"\\SystemRoot\\MBKS\\ImageNotify.log"
#define CONSOLE_PROGRAM_NAME L"ConsoleApp.exe"
#define MY_PROG_NAME RTL_CONSTANT_STRING(CONSOLE_PROGRAM_NAME);

#define START_TAG 0x20202020UL
#define LOAD_TAG 0x7f7f7f7fUL

#define GLOBAL_DATA_FLAG_CDO_OPEN_REF 0x00000001
#define GLOBAL_DATA_FLAG_CDO_OPEN_HANDLE 0x00000002

typedef struct _FILTER_GLOBAL_DATA {
  PFLT_FILTER _filter;
  PDRIVER_OBJECT _filterDriverObject;
  PDEVICE_OBJECT _filterControlDeviceObject;
  ULONG _filterFlags;
  HANDLE _loadImageLogFile;
  ULONG _currTag;
  MyRuleList* _myRuleList;
} FILTER_GLOBAL_DATA, *PFILTER_GLOBAL_DATA;

extern FILTER_GLOBAL_DATA _global;

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

NTSTATUS
_Function_class_(DRIVER_INITIALIZE)
FSFltCreateCDO(
  _Inout_ PDRIVER_OBJECT __driverObj
);

VOID
FSFltDeleteCDO(VOID);

FLT_PREOP_CALLBACK_STATUS
FSFltPreDriverControl(
  _Inout_ PFLT_CALLBACK_DATA __data,
  _In_ PCFLT_RELATED_OBJECTS __fltObjects,
  _Out_ PVOID* __completionContext
);
/*
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
*/
#endif // !_DRIVER_H
