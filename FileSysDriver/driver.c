#include <fltKernel.h>
#include <wdm.h>
#include <stdarg.h>
#include <dontuse.h>
#include <suppress.h>

#include "../file_sys_filter.h"
#include "rules_list.h"
#include "driver.h"

const UNICODE_STRING gMyProgName = RTL_CONSTANT_STRING(CONSOLE_PROGRAM_NAME);

PFLT_FILTER gDriverHandle = NULL;
HANDLE gLoadImageLogFile = NULL;
ULONG gCurrTag = 0x20202020;
MyRuleList* gMyRuleList = NULL;
ULONG gRegTag = 0x7f7f7f7f;
ULONG gLoadTag = 0x7f7f7f7e;

CONST FLT_OPERATION_REGISTRATION gCallbacks[] = {
  { IRP_MJ_DEVICE_CONTROL,
    0,
    FltPreDriverControl,
    NULL,
  },
  { IRP_MJ_OPERATION_END }
};

CONST FLT_REGISTRATION gFilterRegistration = {
  sizeof(FLT_REGISTRATION),               // Size
  FLT_REGISTRATION_VERSION,               // Version
  0,                                      // Flags
  NULL,                                   // Context
  gCallbacks,                             // Operation callbacks
  DriverUnload,                           // Filter unload callback
  NULL,                                   // Instance setup callback
  DriverInstanceQueryTeardown,            // Instance query teardown callback
  NULL,                                   // Instance teardown start callback
  NULL,                                   // Instance teardown complete callback
  NULL,                                   // Generate filename callback
  NULL,                                   // Normalize name component callback
  NULL,                                   // Normalize context cleanup callback
  NULL,                                   // Transaction notification callback
};

NTSTATUS
DriverEntry(
  _In_ PDRIVER_OBJECT  __driverObj,
  _In_ PUNICODE_STRING __registryPath) {

  NTSTATUS __resStatus = STATUS_SUCCESS;

  UNREFERENCED_PARAMETER(__registryPath);

  _PrintDebugStatus("DriverEntry start");

  __resStatus = FltRegisterFilter(
    __driverObj,
    &gFilterRegistration,
    &gDriverHandle
  );
  if (!NT_SUCCESS(__resStatus)) {
    PrintDebugError("FltRegisterFilter", __resStatus);
    return __resStatus;
  }
  __resStatus = FltStartFiltering(gDriverHandle);
  if (!NT_SUCCESS(__resStatus)) {
    PrintDebugError("FltStartFiltering", __resStatus);
    FltUnregisterFilter(gDriverHandle);
    return __resStatus;
  }

  return __resStatus;
}


NTSTATUS
DriverUnload(
  _In_ FLT_FILTER_UNLOAD_FLAGS __flags
) {
  UNREFERENCED_PARAMETER(__flags);

  PAGED_CODE();

  FltUnregisterFilter(gDriverHandle);

  return STATUS_SUCCESS;
}

NTSTATUS
DriverInstanceQueryTeardown(
  _In_ PCFLT_RELATED_OBJECTS __fltObjects,
  _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS __flags
) {
  UNREFERENCED_PARAMETER(__fltObjects);
  UNREFERENCED_PARAMETER(__flags);

  PAGED_CODE();

  return STATUS_SUCCESS;
}

FLT_PREOP_CALLBACK_STATUS
FltPreDriverControl(
  _Inout_ PFLT_CALLBACK_DATA __data,
  _In_ PCFLT_RELATED_OBJECTS __fltObjects,
  _Out_ PVOID* __completionContext
) {
  FLT_PREOP_CALLBACK_STATUS __callbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
  PDRIVER_IO __driverIo =
    __data->Iopb->Parameters.DeviceIoControl.Buffered.SystemBuffer;
  ULONG __ioCtlCode =
    __data->Iopb->Parameters.DeviceIoControl.Buffered.IoControlCode;

  UNREFERENCED_PARAMETER(__fltObjects);
  UNREFERENCED_PARAMETER(__completionContext);

  PAGED_CODE(); 

  switch (__ioCtlCode) {
    case IOCTL_DO_UPDATE_RULES:
      __driverIo->_resVal = 1;
      break;
    case IOCTL_SET_LOAD_IMAGE:
      __driverIo->_resVal = 2;
      break;
    case IOCTL_REMOVE_LOAD_IMAGE:
      __driverIo->_resVal = 3;
      break;
    default:
      __driverIo->_resVal = 4;
      break;
  }

  return __callbackStatus;
}

int
SetLoadImageNotify() {
  NTSTATUS __resStatus = STATUS_SUCCESS;
  UNICODE_STRING __logFileName;
  OBJECT_ATTRIBUTES __objAttrs;
  IO_STATUS_BLOCK __ioStatusBlock;
  if (gLoadImageLogFile != NULL) {
    return DRIVER_ERROR_LOG_FILE_ALREADY_OPENED;
  }
  RtlInitUnicodeString(&__logFileName, LOG_FILE_NAME);
  InitializeObjectAttributes(
    &__objAttrs,
    &__logFileName,
    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
    NULL,
    NULL
  );

  __resStatus = ZwCreateFile(
    &gLoadImageLogFile,
    FILE_APPEND_DATA | SYNCHRONIZE,
    &__objAttrs,
    &__ioStatusBlock,
    NULL,
    FILE_ATTRIBUTE_NORMAL,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    FILE_OPEN_IF,
    FILE_SYNCHRONOUS_IO_NONALERT,
    NULL,
    0
  );

  if (!NT_SUCCESS(__resStatus)) {
    gLoadImageLogFile = NULL;
    return DRIVER_ERROR_LOG_FILE_CREATE_FAILED;
  }

  __resStatus = PsSetLoadImageNotifyRoutine(LoadImageNotify);
  if (!NT_SUCCESS(__resStatus)) {
    ZwClose(gLoadImageLogFile);
    gLoadImageLogFile = NULL;
    return DRIVER_ERROR_SET_NOTIFY_ROUTINE;
  }
  PrintDebugSuccess("PsSetLoadImageNotifyRoutine");
  return DRIVER_ERROR_SUCCESS;
}

int
RemoveLoadImageNotify() {
  NTSTATUS __resStatus = STATUS_SUCCESS;
  if (gLoadImageLogFile == NULL) {
    return DRIVER_ERROR_LOG_FILE_NOT_OPENED;
  }

  __resStatus = PsRemoveLoadImageNotifyRoutine(LoadImageNotify);
  if (__resStatus == STATUS_PROCEDURE_NOT_FOUND) {
    ZwClose(gLoadImageLogFile);
    gLoadImageLogFile = NULL;
    return DRIVER_ERROR_REMOVE_NOTIFY_ROUTINE;
  }
  ZwClose(gLoadImageLogFile);
  gLoadImageLogFile = NULL;
  PrintDebugSuccess("PsRemoveLoadImageNotifyRoutine");
  return DRIVER_ERROR_SUCCESS;
}

void
LoadImageNotify(
  PUNICODE_STRING __fullImageName,
  HANDLE __processId,
  PIMAGE_INFO __imageInfo) {
  NTSTATUS __resStatus = DRIVER_ERROR_SUCCESS;
  LARGE_INTEGER __utc0, __utc3;
  WCHAR __logStr[2048] = { 0x00 };
  size_t __logStrLen;
  PUNICODE_STRING __procName;
  IO_STATUS_BLOCK __ioStatusBlock;

  UNREFERENCED_PARAMETER(__fullImageName);
  UNREFERENCED_PARAMETER(__processId);
  UNREFERENCED_PARAMETER(__imageInfo);
  UNREFERENCED_PARAMETER(__resStatus);
  UNREFERENCED_PARAMETER(__utc0);
  UNREFERENCED_PARAMETER(__utc3);
  UNREFERENCED_PARAMETER(__logStr);
  UNREFERENCED_PARAMETER(__logStrLen);
  UNREFERENCED_PARAMETER(__procName);
  UNREFERENCED_PARAMETER(__ioStatusBlock);

  PrintDebugSuccess("LoadImage");
}

inline
void
PrintDebugError(const char* __funcName, NTSTATUS __errCode) {
  _PrintDebugStatus("%s failed with 0x%08x\n", __funcName, __errCode);
}

inline
void
PrintDebugSuccess(const char* __funcName) {
  _PrintDebugStatus("%s SUCCESS", __funcName);
}

inline
void _PrintDebugStatus(const char* __fmt, ...) {
  va_list __args;
  va_start(__args, __fmt);
  DbgPrint("RegistryFilter: ", __args);
  va_end(__args);
}
