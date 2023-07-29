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
  DbgPrint("[FileSysFilter]: " FMT, __VA_ARGS__);

#else

#define PRINT_STATUS(FMT, ...) { NOTHING; }

#endif //DBG

#define PRINT_ERROR(FUNC, ERR_CODE) \
  PRINT_STATUS("%s failed with 0x%08x\n", FUNC, ERR_CODE);
#define PRINT_SUCCESS(FUNC) \
  PRINT_STATUS("%s SUCCESS\n", FUNC);

#define LOG_FILE_NAME L"\\SystemRoot\\MBKS\\ImageNotify.log"
#define CONSOLE_PROGRAM_NAME L"ConsoleApp.exe"
#define MY_PROG_NAME RTL_CONSTANT_STRING(CONSOLE_PROGRAM_NAME);

#define START_TAG 0x20202020UL
#define LOAD_TAG 0x7f7f7f7fUL

#define DRIVER_LOAD_IMAGE_BUFFER_LENGTH 1024

#define GLOBAL_DATA_FLAG_CDO_OPEN_REF (1UL << 0)
#define GLOBAL_DATA_FLAG_CDO_OPEN_HANDLE (1UL << 1)
#define GLOBAL_DATA_FLAG_LOG_FILE_OPENED (1UL << 2)
#define GLOBAL_DATA_FLAG_LOAD_IMAGE_SET (1UL << 3)

typedef struct _FILTER_GLOBAL_DATA {
  PFLT_FILTER _filter;
  PDRIVER_OBJECT _filterDriverObject;
  PDEVICE_OBJECT _filterControlDeviceObject;
  ULONG _filterFlags;
  HANDLE _filterLogFile;
  ULONG _currTag;
  MyRuleList* _myRuleList;
} FILTER_GLOBAL_DATA, *PFILTER_GLOBAL_DATA;

extern FILTER_GLOBAL_DATA _global;

NTSTATUS
DriverEntry(
  _In_ PDRIVER_OBJECT __driverObj,
  _In_ PUNICODE_STRING __registryPat
);

NTSTATUS
FSFltUnload(
  _In_ FLT_FILTER_UNLOAD_FLAGS __flags
);


NTSTATUS
FSFltInstanceQueryTeardown(
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

NTSTATUS
FSFltMajorFunction(
  _In_ PDEVICE_OBJECT __deviceObj,
  _Inout_ PIRP __irp
);

NTSTATUS
FSFltHandleCreate(
  _In_ PIRP __irp
);

NTSTATUS
FSFltHandleClose(
  _In_ PIRP __irp
);

NTSTATUS
FSFltHandleCleanup(
  _In_ PIRP __irp
);

NTSTATUS
FSFltDeviceControl(
  _In_ PIRP __irp
);

NTSTATUS
FSFltSetLoadImageNotify(
  _Inout_ ULONG* __result
);

NTSTATUS
FSFltRemoveLoadImageNotify(
  _Inout_ ULONG* __result
);

VOID
FSFltLoadImageNotify(
  _In_opt_ PUNICODE_STRING __fullImageName,
  _In_ HANDLE __processId,
  _In_ PIMAGE_INFO __imageInfo
);

#endif // !_DRIVER_H
