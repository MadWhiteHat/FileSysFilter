#include <ntifs.h>
#include <ntstrsafe.h>
#include <wdm.h>

#include "../file_sys_filter.h"
#include "rules_list.h"
#include "driver.h"

const UNICODE_STRING gMyProgName = RTL_CONSTANT_STRING(CONSOLE_PROGRAM_NAME);
const UNICODE_STRING gMyDriverName = RTL_CONSTANT_STRING(DRIVER_NAME_WITH_EXT);

PDEVICE_OBJECT gDeviceObj = NULL;
HANDLE gLoadImageLogFile = NULL;
ULONG gCurrTag = 0x20202020;
MyRuleList* gMyRuleList = NULL;
LARGE_INTEGER gDriverCookie;
ULONG gRegTag = 0x7f7f7f7f;
ULONG gLoadTag = 0x7f7f7f7e;


NTSTATUS
DriverEntry(
  _In_ PDRIVER_OBJECT  __driverObj,
  _In_ PUNICODE_STRING __registryPath) {

  NTSTATUS __resStatus = STATUS_SUCCESS;
  UNICODE_STRING __ntDriverName;
  UNICODE_STRING __dosUserModeDriverName;
  UNICODE_STRING __driverSDDLString;
  UNICODE_STRING __driverAltitude = RTL_CONSTANT_STRING(DRIVER_ALTITUDE);

  UNREFERENCED_PARAMETER(__registryPath);

  DbgPrint("RegistryFilter DriverEntry\n");
  gLoadImageLogFile = NULL;

  ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

  RtlInitUnicodeString(&__ntDriverName, DRIVER_NAME);
  RtlInitUnicodeString(&__driverSDDLString, DRIVER_SDDL_STRING);

  __resStatus = WdmlibIoCreateDeviceSecure(
    __driverObj,
    0,
    &__ntDriverName,
    FILE_DEVICE_UNKNOWN,
    0,
    TRUE,
    &__driverSDDLString,
    NULL,
    &gDeviceObj
  );
  
  if (!NT_SUCCESS(__resStatus)) {
    _PrintDebugError("DriverEntry WdmlibIoCreateDeviceSecure", __resStatus);
    return __resStatus;
  }

  __driverObj->MajorFunction[IRP_MJ_CREATE] = DriverCreate;
  __driverObj->MajorFunction[IRP_MJ_CLOSE] = DriverClose;
  __driverObj->MajorFunction[IRP_MJ_CLEANUP] = DriverCleanup;
  __driverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverControl;
 
  // Create a link in the Win32 namespace
  RtlInitUnicodeString(&__dosUserModeDriverName, USER_MODE_DRIVER_NAME);
  __resStatus = IoCreateSymbolicLink(
    &__dosUserModeDriverName,
    &__ntDriverName
  );

  if (!NT_SUCCESS(__resStatus)) {
    _PrintDebugError("DriverEntry, IoCreateSymbolicLink", __resStatus);
    IoDeviceDelete(__driverObj->DeviceObject);
    return __resStatus;
  }

  // load rules here
  __resStatus = CmRegisterCallbackEx(
    RegistryCallback,
    &__driverAltitude,
    __driverObj,
    NULL,
    &gDriverCookie,
    NULL
  );
  if (!NT_SUCCESS(__resStatus)) {
    _PrintDebugError("DriverEntry CmRegisterCallback", __resStatus);
  } else {
    DbgPrint("Driver was loaded correctly");
  }

  return STATUS_SUCCESS;
}

NTSTATUS
DriverCreate(
  _In_ PDEVICE_OBJECT __driverObject,
  _Inout_ PIRP __irpReq
) {
  UNREFERENCED_PARAMETER(__driverObject);

  __irpReq->IoStatus.Status = STATUS_SUCCESS;
  __irpReq->IoStatus.Information = 0;

  IoCompleteRequest(__irpReq, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

NTSTATUS
DriverClose(
  _In_ PDEVICE_OBJECT __driverObject,
  _Inout_ PIRP __irpReq
) {
  UNREFERENCED_PARAMETER(__driverObject);

  __irpReq->IoStatus.Status = STATUS_SUCCESS;
  __irpReq->IoStatus.Information = 0;

  IoCompleteRequest(__irpReq, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

NTSTATUS
DriverCleanup(
  _In_ PDEVICE_OBJECT __driverObject,
  _Inout_ PIRP __irpReq
) {
  UNREFERENCED_PARAMETER(__driverObject);

  __irpReq->IoStatus.Status = STATUS_SUCCESS;
  __irpReq->IoStatus.Information = 0;

  IoCompleteRequest(__irpReq, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

NTSTATUS
DriverControl(
  _In_ PDEVICE_OBJECT __driverObject,
  _Inout_ PIRP __irpReq
) {
  ULONG __ioCtlCode;
  NTSTATUS __resStatus = STATUS_SUCCESS;
  PIO_STACK_LOCATION __irpStack = NULL;
  PDRIVER_IO __driverIoCtl = NULL;
  ULONG __ioBufferLen;

  UNREFERENCED_PARAMETER(__driverObject);

  __irpStack = IoGetCurrentIrpStackLocation(__irpReq);
  __ioCtlCode = __irpStack->Parameters.DeviceIoControl.IoControlCode;
  __ioBufferLen = __irpStack->Parameters.DeviceIoControl.OutputBufferLength;

  if (__ioBufferLen < sizeof(DRIVER_IO)) {
    __resStatus = STATUS_INVALID_PARAMETER;
    _PrintDebugError("DeviceControl", __resStatus);
    return __resStatus;
  }

  __driverIoCtl = (PDRIVER_IO)(__irpReq->AssociatedIrp.SystemBuffer);
  switch (__ioCtlCode) {
    case IOCTL_DO_UPDATE_RULES:
      DbgPrint("IOCTL_DO_UPDATE_RULES");
      break;
    case IOCTL_SET_LOAD_IMAGE:
      DbgPrint("IOCTL_SET_LOAD_IMAGE");
      __driverIoCtl->_resVal = SetLoadImageNotify();
      break;
    case IOCTL_REMOVE_LOAD_IMAGE:
      DbgPrint("IOCTL_REMOVE_LOAD_IMAGE");
      __driverIoCtl->_resVal = RemoveLoadImageNotify();
      break;
    default:
      _PrintDebugError("IOCTL recognition", __ioCtlCode);
      break;
  }

  __irpReq->IoStatus.Status = STATUS_SUCCESS;
  __irpReq->IoStatus.Information = sizeof(DRIVER_IO);

  IoCompleteRequest(__irpReq, IO_NO_INCREMENT);

  return __resStatus;
}

void
DriverUnload(_In_ PDRIVER_OBJECT __driverObj) {
  NTSTATUS __resStatus = STATUS_SUCCESS;
  UNICODE_STRING __dosUserModeDriverName;
  
}

NTSTATUS
RegistryCallback(
  __in PVOID __callbackContext,
  __in_opt PVOID __arg1,
  __in_opt PVOID __arg2
) {
  UNREFERENCED_PARAMETER(__callbackContext);
  UNREFERENCED_PARAMETER(__arg1);
  UNREFERENCED_PARAMETER(__arg2);

  return STATUS_SUCCESS;
}

int
SetLoadImageNotify() {
  NTSTATUS __resStatus = STATUS_SUCCESS;
  UNICODE_STRING __logFileName;
  OBJECT_ATTRIBUTES __objAttrs;
  IO_STATUS_BLOCK __ioStatusBlock;
  if (gLoadImageLogFile != NULL) { return ERROR_LOG_FILE_ALREADY_OPENED; }
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
    return ERROR_LOG_FILE_CREATE_FAILED;
  }

  __resStatus = PsSetLoadImageNotifyRoutine(LoadImageNotify);
  if (!NT_SUCCESS(__resStatus)) {
    ZwClose(gLoadImageLogFile);
    gLoadImageLogFile = NULL;
    return ERROR_SET_NOTIFY_ROUTINE;
  }
  _PrintDebugSuccess("PsSetLoadImageNotifyRoutine");
  return ERROR_SUCCESS;
}

int
RemoveLoadImageNotify() {
  NTSTATUS __resStatus = STATUS_SUCCESS;
  if (gLoadImageLogFile == NULL) {
    return ERROR_LOG_FILE_NOT_OPENED;
  }

  __resStatus = PsRemoveLoadImageNotifyRoutine(LoadImageNotify);
  if (__resStatus == STATUS_PROCEDURE_NOT_FOUND) {
    ZwClose(gLoadImageLogFile);
    gLoadImageLogFile = NULL;
    return ERROR_REMOVE_NOTIFY_ROUTINE;
  }
  ZwClose(gLoadImageLogFile);
  gLoadImageLogFile = NULL;
  _PrintDebugSuccess("PsRemoveLoadImageNotifyRoutine");
  return ERROR_SUCCESS;
}

void
LoadImageNotify(
  PUNICODE_STRING __fullImageName,
  HANDLE __processId,
  PIMAGE_INFO __imageInfo) {
  NTSTATUS __resStatus = ERROR_SUCCESS;
  LARGE_INTEGER __utc0, __utc3;
  WCHAR __logStr[2048] = { 0x00 };
  size_t __logStrLen;
  PUNICODE_STRING __procName;
  IO_STATUS_BLOCK __ioStatusBlock;

}

inline
void
_PrintDebugError(const char* __funcName, NTSTATUS __errCode) {
  DbgPrint("RegistryFilter: %s failed with 0x%08x\n", __funcName, __errCode);
}

inline
void
_PrintDebugSuccess(const char* __funcName) {
  DbgPrint("RegistryFilter: %s SUCCESS\n", __funcName);
}
