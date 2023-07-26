#include "filter.h"

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, FSFltCreateCDO)
#pragma alloc_text(PAGE, FSFltDeleteCDO)
#pragma alloc_text(PAGE, FSFltPreDriverControl)
//#pragma alloc_text(PAGE, SetLoadImageNotify)
//#pragma alloc_text(PAGE, RemoveLoadImageNotify)
//#pragma alloc_text(PAGE, LoadImageNotify)

#endif // ALLOC_PRAGMA

NTSTATUS
_Function_class_(DRIVER_INITIALIZE)
FSFltCreateCDO(
  _Inout_ PDRIVER_OBJECT __driverObj
) {
  NTSTATUS __resStatus = STATUS_SUCCESS;
  UNICODE_STRING __cdoName;
  UNICODE_STRING __symLink;

  PAGED_CODE();

  PRINT_STATUS("Creating CDO...");

  RtlInitUnicodeString(&__cdoName, DRIVER_CDO_NAME);
  RtlInitUnicodeString(&__symLink, DRIVER_USERMODE_NAME);
  
  __resStatus = IoCreateDevice(
    __driverObj,
    0,
    &__cdoName,
    FILE_DEVICE_DISK_FILE_SYSTEM,
    FILE_DEVICE_SECURE_OPEN,
    FALSE,
    &_global._filterControlDeviceObject
  );

  if (!NT_SUCCESS(__resStatus)) {
    PRINT_ERROR("IoCreateDevice", __resStatus);
    return __resStatus;
  } else { PRINT_SUCCESS("IoCreateDevice"); }

  __resStatus = IoCreateSymbolicLink(&__symLink, &__cdoName);
  if (!NT_SUCCESS(__resStatus)) {
    PRINT_ERROR("IoCreateSymbolicLink", __resStatus);
    return __resStatus;
  } else { PRINT_SUCCESS("IoCreateSymbolicLink"); }

  PRINT_SUCCESS("Creating CDO");
  return STATUS_SUCCESS;
}

VOID
FSFltDeleteCDO(VOID) {
  UNICODE_STRING __symLink;

  PAGED_CODE();

  PRINT_STATUS("Deleting CDO...");

  RtlInitUnicodeString(&__symLink, DRIVER_USERMODE_NAME);
  IoDeleteSymbolicLink(&__symLink);

  IoDeleteDevice(_global._filterControlDeviceObject);

  PRINT_SUCCESS("Deleting CDO");
}

FLT_PREOP_CALLBACK_STATUS
FSFltPreDriverControl(
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
    case IOCTL_UPDATE_RULES:
      __driverIo->_resVal = 1;
      PRINT_STATUS("IOCTL_UPDATE_RULES");
      break;
    case IOCTL_SET_LOAD_IMAGE:
      __driverIo->_resVal = 2;
      PRINT_STATUS("IOCTL_SET_LOAD_IMAGE");
      break;
    case IOCTL_REMOVE_LOAD_IMAGE:
      __driverIo->_resVal = 3;
      PRINT_STATUS("IOCTL_REMOVE_LOAD_IMAGE");
      break;
    default:
      __driverIo->_resVal = 4;
      PRINT_STATUS("IOCTL_DEFAULT");
      break;
  }

  return __callbackStatus;
}
/*
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
  PRINT_SUCCESS("PsSetLoadImageNotifyRoutine");
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
  PRINT_SUCCESS("PsRemoveLoadImageNotifyRoutine");
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

  PRINT_SUCCESS("LoadImageNotify");
}
*/
