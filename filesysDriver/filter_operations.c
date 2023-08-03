#include "filter.h"

#include <ntstrsafe.h>

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, FSFltCreateCDO)
#pragma alloc_text(PAGE, FSFltDeleteCDO)
#pragma alloc_text(PAGE, FSFltMajorFunction)
#pragma alloc_text(PAGE, FSFltHandleCreate)
#pragma alloc_text(PAGE, FSFltHandleClose)
#pragma alloc_text(PAGE, FSFltDeviceControl)
#pragma alloc_text(PAGE, FSFltSetLoadImageNotify)
#pragma alloc_text(PAGE, FSFltRemoveLoadImageNotify)
#pragma alloc_text(PAGE, FSFltLoadImageNotify)

#endif // ALLOC_PRAGMA

NTSTATUS
_Function_class_(FSFLT_DRIVER_INITIALIZE)
FSFltCreateCDO(
  _Inout_ PDRIVER_OBJECT __driverObj
) {
  NTSTATUS __resStatus = STATUS_SUCCESS;
  UNICODE_STRING __cdoName;
  UNICODE_STRING __symLink;

  PAGED_CODE();

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

  for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i) {
    __driverObj->MajorFunction[i] = FSFltMajorFunction;
  }

  __resStatus = IoCreateSymbolicLink(&__symLink, &__cdoName);
  if (!NT_SUCCESS(__resStatus)) {
    PRINT_ERROR("IoCreateSymbolicLink", __resStatus);
    IoDeleteDevice(_global._filterControlDeviceObject);
    return __resStatus;
  } else { PRINT_SUCCESS("IoCreateSymbolicLink"); }

  return __resStatus;
}

VOID
FSFltDeleteCDO(VOID) {
  UNICODE_STRING __symLink;

  PAGED_CODE();

  RtlInitUnicodeString(&__symLink, DRIVER_USERMODE_NAME);
  IoDeleteSymbolicLink(&__symLink);

  IoDeleteDevice(_global._filterControlDeviceObject);
}

NTSTATUS
FSFltMajorFunction(
  _In_ PDEVICE_OBJECT __deviceObj,
  _Inout_ PIRP __irp
) {

  NTSTATUS __resStatus = STATUS_SUCCESS;
  PIO_STACK_LOCATION __irpStack;

  UNREFERENCED_PARAMETER(__deviceObj);

  PAGED_CODE();

  __irpStack = IoGetCurrentIrpStackLocation(__irp);

  PRINT_STATUS("IRP_MJ_FUNC: 0x%08x\n", __irpStack->MajorFunction);

  switch (__irpStack->MajorFunction) {

    case IRP_MJ_CREATE:
      __resStatus = FSFltHandleCreate(__irp);

      __irp->IoStatus.Status = __resStatus;
      if (!NT_SUCCESS(__resStatus)) {
        __irp->IoStatus.Information = 0;
        PRINT_ERROR("FSFltHandleCreate", __resStatus);
      } else {
        __irp->IoStatus.Information = FILE_OPENED;
        PRINT_SUCCESS("FSFltHandleCreate");
      }

      IoCompleteRequest(__irp, IO_NO_INCREMENT);

      break;

    case IRP_MJ_CLOSE:
      __resStatus = FSFltHandleClose(__irp);

      __irp->IoStatus.Status = __resStatus;
      __irp->IoStatus.Information = 0;
      if (!NT_SUCCESS(__resStatus)) {
        PRINT_ERROR("FSFltHandleClose", __resStatus);
      } else {
        PRINT_SUCCESS("FSFltHandleClose");
      }

      IoCompleteRequest(__irp, IO_NO_INCREMENT);

      break;

    case IRP_MJ_CLEANUP:
      FSFltHandleCleanup(__irp);

      __irp->IoStatus.Status = STATUS_SUCCESS;
      __irp->IoStatus.Information = 0;

      PRINT_SUCCESS("FSFltHandleCleanup");

      IoCompleteRequest(__irp, IO_NO_INCREMENT);

      break;

    case IRP_MJ_DEVICE_CONTROL:
      __resStatus = FSFltDeviceControl(__irp);

      __irp->IoStatus.Status = __resStatus;
      if (!NT_SUCCESS(__resStatus)) {
        __irp->IoStatus.Information = 0;
        PRINT_ERROR("FSFltDeviceControl", __resStatus);
      } else {
        __irp->IoStatus.Information = sizeof(DRIVER_IO);
        PRINT_SUCCESS("FSFltDeviceControl");
      }

      IoCompleteRequest(__irp, IO_NO_INCREMENT);

      break;
    
    default:
      __resStatus = STATUS_INVALID_DEVICE_REQUEST;

      __irp->IoStatus.Status = __resStatus;
      __irp->IoStatus.Information = 0;

      IoCompleteRequest(__irp, IO_NO_INCREMENT);
  }

  return __resStatus;
}

NTSTATUS
FSFltHandleCreate(
  _In_ PIRP __irp
) {
  NTSTATUS __resStatus = STATUS_SUCCESS;

  UNREFERENCED_PARAMETER(__irp);

  PAGED_CODE();

  if (FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_HANDLE)
    || FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_REF)) {

    FLT_ASSERT(
      !FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_HANDLE)
      || FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_REF)
    );
    
    __resStatus = STATUS_DEVICE_ALREADY_ATTACHED;
  } else {
    SetFlag(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_REF);
    SetFlag(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_HANDLE);

    __resStatus = STATUS_SUCCESS;
  }

  return __resStatus;
}

NTSTATUS
FSFltHandleClose(
  _In_ PIRP __irp
) {
  NTSTATUS __resStatus = STATUS_SUCCESS;

  UNREFERENCED_PARAMETER(__irp);

  PAGED_CODE();

  FLT_ASSERT(
    FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_REF)
    && !FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_HANDLE)
  );

  ClearFlag(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_REF);

  return __resStatus;
}

NTSTATUS
FSFltHandleCleanup(
  _In_ PIRP __irp
) {
  NTSTATUS __resStatus = STATUS_SUCCESS;

  UNREFERENCED_PARAMETER(__irp);

  PAGED_CODE();

  FLT_ASSERT(
    FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_REF)
    && FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_HANDLE)
  );

  ClearFlag(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_HANDLE);

  return __resStatus;
}

NTSTATUS
FSFltDeviceControl(
  _In_ PIRP __irp
) {

  NTSTATUS __resStatus = STATUS_SUCCESS;
  LONG __result = FSFLT_ERROR_SUCCESS;
  ULONG __ioctlCode;
  PIO_STACK_LOCATION __irpStack;
  PDRIVER_IO __ioctlOutput;
  ULONG __ioctlOutputBufferLen;

  PAGED_CODE();

  __irpStack = IoGetCurrentIrpStackLocation(__irp);
  __ioctlCode = __irpStack->Parameters.DeviceIoControl.IoControlCode;
  __ioctlOutputBufferLen =
    __irpStack->Parameters.DeviceIoControl.OutputBufferLength;

  if (__ioctlOutputBufferLen != sizeof(DRIVER_IO)) {
    __resStatus = STATUS_INVALID_PARAMETER;
    return __resStatus;
  }

  __ioctlOutput = (PDRIVER_IO)__irp->AssociatedIrp.SystemBuffer;

  switch (__ioctlCode) {
    case IOCTL_SET_LOAD_IMAGE:
      __resStatus = FSFltSetLoadImageNotify(&__result);
      break;
    case IOCTL_REMOVE_LOAD_IMAGE:
      __resStatus = FSFltRemoveLoadImageNotify(&__result);
      break;
    case IOCTL_ADD_RULE:
      PRINT_STATUS(
        "Process: %ws; File: %ws; Mask: 0x%08x\n",
        __ioctlOutput->_type._ruleAddInfo._procName,
        __ioctlOutput->_type._ruleAddInfo._fileName,
        __ioctlOutput->_type._ruleAddInfo._accessMask
      );
      break;
    case IOCTL_DEL_RULE:
      PRINT_STATUS("Rule #%d", __ioctlOutput->_type._ruleDelNum);
      break;
    default:
      __resStatus = STATUS_INVALID_PARAMETER;
  }

  __ioctlOutput->_result = __result;

  return __resStatus;
}

NTSTATUS
FSFltSetLoadImageNotify(
  _Inout_ LONG* __result
) {
  NTSTATUS __resStatus = STATUS_SUCCESS;
  UNICODE_STRING __logFileName;
  OBJECT_ATTRIBUTES __objAttrs;
  IO_STATUS_BLOCK __ioStatusBlock;
  if (FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_LOAD_IMAGE_SET)) {
    *__result = FSFLT_DRIVER_ERROR_LOAD_IMAGE_ALREADY_SET;
    return STATUS_INVALID_PARAMETER;
  }

  if (FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_LOG_FILE_OPENED)) {
    *__result = FSFLT_DRIVER_ERROR_LOG_FILE_ALREADY_OPENED;
    return STATUS_INVALID_PARAMETER;
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
    &_global._filterLogFile,
    FILE_APPEND_DATA | SYNCHRONIZE,
    &__objAttrs,
    &__ioStatusBlock,
    NULL,
    FILE_ATTRIBUTE_NORMAL,
    0,
    FILE_OPEN_IF,
    FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT,
    NULL,
    0
  );

  if (!NT_SUCCESS(__resStatus)) {
    PRINT_ERROR("ZwCreateFile", __resStatus);
    *__result = FSFLT_DRIVER_ERROR_LOG_FILE_CREATE_FAILED;
    return __resStatus;
  }

  SetFlag(_global._filterFlags, GLOBAL_DATA_FLAG_LOG_FILE_OPENED);

  __resStatus = PsSetLoadImageNotifyRoutine(FSFltLoadImageNotify);

  if (!NT_SUCCESS(__resStatus)) {
    ZwClose(_global._filterLogFile);
    ClearFlag(_global._filterFlags, GLOBAL_DATA_FLAG_LOG_FILE_OPENED);
    PRINT_ERROR("PsSetLoadImageNotifyRoutine", __resStatus);
    *__result = FSFLT_DRIVER_ERROR_SET_LOAD_IMAGE;
    return __resStatus;
  }

  SetFlag(_global._filterFlags, GLOBAL_DATA_FLAG_LOAD_IMAGE_SET);

  PRINT_SUCCESS("PsSetLoadImageNotifyRoutine");

  *__result = FSFLT_ERROR_SUCCESS;

  return __resStatus;
}

NTSTATUS
FSFltRemoveLoadImageNotify(
  _Inout_ LONG* __result
) {
  NTSTATUS __resStatus = STATUS_SUCCESS;
  if (!FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_LOAD_IMAGE_SET)) {
    *__result = FSFLT_DRIVER_ERROR_LOAD_IMAGE_ALREADY_REMOVED;
    return STATUS_INVALID_PARAMETER;
  }
  if (!FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_LOG_FILE_OPENED)) {
    *__result = FSFLT_DRIVER_ERROR_LOG_FILE_NOT_OPENED;
    return STATUS_INVALID_PARAMETER;
  }

  __resStatus = PsRemoveLoadImageNotifyRoutine(FSFltLoadImageNotify);

  if (!NT_SUCCESS(__resStatus)) {
    PRINT_ERROR("PsRemoveLoadImageNotifyRoutine", __resStatus);
    *__result = FSFLT_DRIVER_ERROR_LOAD_IMAGE_REMOVE;
    return __resStatus;
  }
  
  ClearFlag(_global._filterFlags, GLOBAL_DATA_FLAG_LOAD_IMAGE_SET);

  ZwClose(_global._filterLogFile);
  ClearFlag(_global._filterFlags, GLOBAL_DATA_FLAG_LOG_FILE_OPENED);

  PRINT_SUCCESS("PsRemoveLoadImageNotifyRoutine");

  *__result = FSFLT_ERROR_SUCCESS;
  return __resStatus;
}

VOID
FSFltLoadImageNotify(
  PUNICODE_STRING __fullImageName,
  HANDLE __processId,
  PIMAGE_INFO __imageInfo) {

  static_assert(
    DRIVER_LOAD_IMAGE_BUFFER_LENGTH < NTSTRSAFE_MAX_CCH,
    "Buffer size is bigger than allowed by NTSTRSAFE_MAX_CCH constant"
  );
  NTSTATUS __resStatus = STATUS_SUCCESS;
  LARGE_INTEGER __systemTime, __localTime;
  TIME_FIELDS __timeFields;
  WCHAR __logStr[DRIVER_LOAD_IMAGE_BUFFER_LENGTH];
  size_t __logStrLen;
  IO_STATUS_BLOCK __ioStatusBlock;

  PAGED_CODE();

  if (__fullImageName == NULL || __imageInfo == NULL) { return; }
  PRINT_SUCCESS("1");

  RtlSecureZeroMemory(__logStr, DRIVER_LOAD_IMAGE_BUFFER_LENGTH);
  PRINT_SUCCESS("2");

  KeQuerySystemTime(&__systemTime);
  PRINT_SUCCESS("3");
  ExSystemTimeToLocalTime(&__systemTime, &__localTime);
  PRINT_SUCCESS("4");
  RtlTimeToTimeFields(&__localTime, &__timeFields);
  PRINT_SUCCESS("5");

  __resStatus = RtlStringCchPrintfW(
    __logStr,
    DRIVER_LOAD_IMAGE_BUFFER_LENGTH,
    L"[%02hd\\%02hd\\%4hd %02hd:%02hd:%02hd] Proces ID: %d "
    L"Image Name: %wZ Image Base Addr: %p Image Size: %ld bytes\n",
    __timeFields.Day, __timeFields.Month, __timeFields.Year,
    __timeFields.Hour, __timeFields.Minute, __timeFields.Second,
    __processId, __fullImageName,
    __imageInfo->ImageBase, __imageInfo->ImageSize
  );
  PRINT_SUCCESS("6");

  if (!NT_SUCCESS(__resStatus)) {
    PRINT_ERROR("RtlStringCchPrintfW", __resStatus);
    return;
  }
  PRINT_SUCCESS("7");

  __resStatus = RtlStringCchLengthW(
    __logStr,
    DRIVER_LOAD_IMAGE_BUFFER_LENGTH,
    &__logStrLen
  );
  PRINT_SUCCESS("8");

  if (!NT_SUCCESS(__resStatus)) {
    PRINT_ERROR("RtlStringCchLengthW", __resStatus);
    return;
  }
  PRINT_SUCCESS("9");

  __logStrLen *= sizeof(__logStr[0]);

  __resStatus = ZwWriteFile(
    _global._filterLogFile,
    NULL,
    NULL,
    NULL,
    &__ioStatusBlock,
    __logStr,
    (ULONG)__logStrLen,
    NULL,
    NULL
  );
  PRINT_STATUS("10 NTSTATUS: 0x%08x", __resStatus);

  PRINT_STATUS("Bytes: %llu / Message: %ws", __logStrLen, __logStr);
  PRINT_SUCCESS("11");
}
