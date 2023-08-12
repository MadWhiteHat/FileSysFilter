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
  NTSTATUS __res = STATUS_SUCCESS;
  UNICODE_STRING __cdoName;
  UNICODE_STRING __symLink;

  PAGED_CODE();

  RtlInitUnicodeString(&__cdoName, DRIVER_CDO_NAME);
  RtlInitUnicodeString(&__symLink, DRIVER_USERMODE_NAME);
  
  __res = IoCreateDevice(
    __driverObj,
    0,
    &__cdoName,
    FILE_DEVICE_DISK_FILE_SYSTEM,
    FILE_DEVICE_SECURE_OPEN,
    FALSE,
    &_global._filterControlDeviceObject
  );

  if (!NT_SUCCESS(__res)) {
    PRINT_ERROR("IoCreateDevice", __res);
    return __res;
  } else { PRINT_SUCCESS("IoCreateDevice"); }

  for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i) {
    __driverObj->MajorFunction[i] = FSFltMajorFunction;
  }

  __res = IoCreateSymbolicLink(&__symLink, &__cdoName);
  if (!NT_SUCCESS(__res)) {
    PRINT_ERROR("IoCreateSymbolicLink", __res);
    IoDeleteDevice(_global._filterControlDeviceObject);
    return __res;
  } else { PRINT_SUCCESS("IoCreateSymbolicLink"); }

  return __res;
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

  NTSTATUS __res = STATUS_SUCCESS;
  PIO_STACK_LOCATION __irpStack;

  UNREFERENCED_PARAMETER(__deviceObj);

  PAGED_CODE();

  __irpStack = IoGetCurrentIrpStackLocation(__irp);

  PRINT_STATUS("IRP_MJ_FUNC: 0x%08x\n", __irpStack->MajorFunction);

  switch (__irpStack->MajorFunction) {

    case IRP_MJ_CREATE:
      __res = FSFltHandleCreate(__irp);

      __irp->IoStatus.Status = __res;
      if (!NT_SUCCESS(__res)) {
        __irp->IoStatus.Information = 0;
        PRINT_ERROR("FSFltHandleCreate", __res);
      } else {
        __irp->IoStatus.Information = FILE_OPENED;
        PRINT_SUCCESS("FSFltHandleCreate");
      }

      IoCompleteRequest(__irp, IO_NO_INCREMENT);

      break;

    case IRP_MJ_CLOSE:
      __res = FSFltHandleClose(__irp);

      __irp->IoStatus.Status = __res;
      __irp->IoStatus.Information = 0;
      if (!NT_SUCCESS(__res)) {
        PRINT_ERROR("FSFltHandleClose", __res);
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
      __res = FSFltDeviceControl(__irp);

      __irp->IoStatus.Status = __res;
      __irp->IoStatus.Information = sizeof(DRIVER_IO);
      if (!NT_SUCCESS(__res)) {
        PRINT_ERROR("FSFltDeviceControl", __res);
      } else {
        PRINT_SUCCESS("FSFltDeviceControl");
      }

      IoCompleteRequest(__irp, IO_NO_INCREMENT);

      break;
    
    default:
      __res = STATUS_INVALID_DEVICE_REQUEST;

      __irp->IoStatus.Status = __res;
      __irp->IoStatus.Information = 0;

      IoCompleteRequest(__irp, IO_NO_INCREMENT);
  }

  return __res;
}

NTSTATUS
FSFltHandleCreate(
  _In_ PIRP __irp
) {
  NTSTATUS __res = STATUS_SUCCESS;

  UNREFERENCED_PARAMETER(__irp);

  PAGED_CODE();

  if (FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_HANDLE)
    || FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_REF)) {

    FLT_ASSERT(
      !FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_HANDLE)
      || FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_REF)
    );
    
    __res = STATUS_DEVICE_ALREADY_ATTACHED;
  } else {
    SetFlag(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_REF);
    SetFlag(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_HANDLE);

    __res = STATUS_SUCCESS;
  }

  return __res;
}

NTSTATUS
FSFltHandleClose(
  _In_ PIRP __irp
) {
  NTSTATUS __res = STATUS_SUCCESS;

  UNREFERENCED_PARAMETER(__irp);

  PAGED_CODE();

  FLT_ASSERT(
    FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_REF)
    && !FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_HANDLE)
  );

  ClearFlag(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_REF);

  return __res;
}

NTSTATUS
FSFltHandleCleanup(
  _In_ PIRP __irp
) {
  NTSTATUS __res = STATUS_SUCCESS;

  UNREFERENCED_PARAMETER(__irp);

  PAGED_CODE();

  FLT_ASSERT(
    FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_REF)
    && FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_HANDLE)
  );

  ClearFlag(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_HANDLE);

  return __res;
}

NTSTATUS
FSFltDeviceControl(
  _In_ PIRP __irp
) {

  NTSTATUS __res = STATUS_SUCCESS;
  LONG __result = FSFLT_ERROR_SUCCESS;
  ULONG __ioctlCode;
  PIO_STACK_LOCATION __irpStack;
  PDRIVER_IO __ioctlInput;
  ULONG __ioctlInputBufferLen;
  ULONG __ioctlOutputBufferLen;

  PAGED_CODE();

  __irpStack = IoGetCurrentIrpStackLocation(__irp);
  __ioctlCode = __irpStack->Parameters.DeviceIoControl.IoControlCode;
  __ioctlInputBufferLen =
    __irpStack->Parameters.DeviceIoControl.InputBufferLength;
  __ioctlOutputBufferLen =
    __irpStack->Parameters.DeviceIoControl.OutputBufferLength;

  if (__ioctlInputBufferLen != sizeof(DRIVER_IO)
    || __ioctlOutputBufferLen != sizeof(DRIVER_IO)) {
    __res = STATUS_INVALID_PARAMETER;
    return __res;
  }

  __ioctlInput = (PDRIVER_IO)__irp->AssociatedIrp.SystemBuffer;

  switch (__ioctlCode) {
    case IOCTL_PRINT_RULES:
      PrintRules(_global._rulesList);
      __result = FSFLT_ERROR_SUCCESS;
      __res = STATUS_SUCCESS;
      break;
    case IOCTL_ADD_RULE:
      __res = AddRule(
        &(_global._rulesList),
        __ioctlInput->_type._ruleAddInfo._fileName,
        __ioctlInput->_type._ruleAddInfo._procName,
        __ioctlInput->_type._ruleAddInfo._accessMask,
        &__result
      );
      break;
    case IOCTL_DEL_RULE:
      __res = DelRule(
        &(_global._rulesList),
        __ioctlInput->_type._ruleAddInfo._fileName,
        __ioctlInput->_type._ruleAddInfo._procName,
        &__result
      );
      break;
    case IOCTL_SET_LOAD_IMAGE:
      __res = FSFltSetLoadImageNotify(&__result);
      break;
    case IOCTL_REMOVE_LOAD_IMAGE:
      __res = FSFltRemoveLoadImageNotify(&__result);
      break;
    default:
      __res = STATUS_INVALID_PARAMETER;
  }

  ((PDRIVER_IO)__irp->UserBuffer)->_result = __result;
  return __res;
}

NTSTATUS
FSFltSetLoadImageNotify(
  _Inout_ LONG* __result
) {
  NTSTATUS __res = STATUS_SUCCESS;
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

  __res = ZwCreateFile(
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

  if (!NT_SUCCESS(__res)) {
    PRINT_ERROR("ZwCreateFile", __res);
    *__result = FSFLT_DRIVER_ERROR_LOG_FILE_CREATE_FAILED;
    return __res;
  }

  SetFlag(_global._filterFlags, GLOBAL_DATA_FLAG_LOG_FILE_OPENED);

  __res = PsSetLoadImageNotifyRoutine(FSFltLoadImageNotify);

  if (!NT_SUCCESS(__res)) {
    ZwClose(_global._filterLogFile);
    ClearFlag(_global._filterFlags, GLOBAL_DATA_FLAG_LOG_FILE_OPENED);
    PRINT_ERROR("PsSetLoadImageNotifyRoutine", __res);
    *__result = FSFLT_DRIVER_ERROR_SET_LOAD_IMAGE;
    return __res;
  }

  SetFlag(_global._filterFlags, GLOBAL_DATA_FLAG_LOAD_IMAGE_SET);

  PRINT_SUCCESS("PsSetLoadImageNotifyRoutine");

  *__result = FSFLT_ERROR_SUCCESS;

  return __res;
}

NTSTATUS
FSFltRemoveLoadImageNotify(
  _Inout_ LONG* __result
) {
  NTSTATUS __res = STATUS_SUCCESS;
  if (!FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_LOAD_IMAGE_SET)) {
    *__result = FSFLT_DRIVER_ERROR_LOAD_IMAGE_ALREADY_REMOVED;
    return STATUS_INVALID_PARAMETER;
  }
  if (!FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_LOG_FILE_OPENED)) {
    *__result = FSFLT_DRIVER_ERROR_LOG_FILE_NOT_OPENED;
    return STATUS_INVALID_PARAMETER;
  }

  __res = PsRemoveLoadImageNotifyRoutine(FSFltLoadImageNotify);

  if (!NT_SUCCESS(__res)) {
    PRINT_ERROR("PsRemoveLoadImageNotifyRoutine", __res);
    *__result = FSFLT_DRIVER_ERROR_LOAD_IMAGE_REMOVE;
    return __res;
  }
  
  ClearFlag(_global._filterFlags, GLOBAL_DATA_FLAG_LOAD_IMAGE_SET);

  ZwClose(_global._filterLogFile);
  ClearFlag(_global._filterFlags, GLOBAL_DATA_FLAG_LOG_FILE_OPENED);

  PRINT_SUCCESS("PsRemoveLoadImageNotifyRoutine");

  *__result = FSFLT_ERROR_SUCCESS;
  return __res;
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
  NTSTATUS __res = STATUS_SUCCESS;
  LARGE_INTEGER __systemTime, __localTime;
  TIME_FIELDS __timeFields;
  WCHAR __logStr[DRIVER_LOAD_IMAGE_BUFFER_LENGTH];
  size_t __logStrLen;
  IO_STATUS_BLOCK __ioStatusBlock;

  PAGED_CODE();

  if (__fullImageName == NULL || __imageInfo == NULL) { return; }

  RtlSecureZeroMemory(__logStr, DRIVER_LOAD_IMAGE_BUFFER_LENGTH);

  KeQuerySystemTime(&__systemTime);
  ExSystemTimeToLocalTime(&__systemTime, &__localTime);
  RtlTimeToTimeFields(&__localTime, &__timeFields);

  __res = RtlStringCchPrintfW(
    __logStr,
    DRIVER_LOAD_IMAGE_BUFFER_LENGTH,
    L"[%02hd\\%02hd\\%4hd %02hd:%02hd:%02hd] Proces ID: %d "
    L"Image Name: %wZ Image Base Addr: %p Image Size: %ld bytes\n",
    __timeFields.Day, __timeFields.Month, __timeFields.Year,
    __timeFields.Hour, __timeFields.Minute, __timeFields.Second,
    __processId, __fullImageName,
    __imageInfo->ImageBase, __imageInfo->ImageSize
  );

  if (!NT_SUCCESS(__res)) {
    PRINT_ERROR("RtlStringCchPrintfW", __res);
    return;
  }

  __res = RtlStringCchLengthW(
    __logStr,
    DRIVER_LOAD_IMAGE_BUFFER_LENGTH,
    &__logStrLen
  );

  if (!NT_SUCCESS(__res)) {
    PRINT_ERROR("RtlStringCchLengthW", __res);
    return;
  }

  __logStrLen *= sizeof(__logStr[0]);

  __res = ZwWriteFile(
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

  PRINT_STATUS("Bytes: %llu / Message: %ws", __logStrLen, __logStr);
}
