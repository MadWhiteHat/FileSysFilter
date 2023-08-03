#include "filter.h"


#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, FSFltUnload)
#pragma alloc_text(PAGE, FSFltInstanceQueryTeardown)

#endif // ALLOC_PRAGMA

FILTER_GLOBAL_DATA _global;

NTSTATUS
DriverEntry(
  _In_ PDRIVER_OBJECT  __driverObj,
  _In_ PUNICODE_STRING __registryPath) {

  NTSTATUS __resStatus = STATUS_SUCCESS;

  CONST FLT_REGISTRATION __filterRegistration = {
    sizeof(FLT_REGISTRATION),               // Size
    FLT_REGISTRATION_VERSION,               // Version
    0,                                      // Flags
    NULL,                                   // Context
    NULL,                                   // Operation callbacks
    FSFltUnload,                            // Filter unload callback
    NULL,                                   // Instance setup callback
    FSFltInstanceQueryTeardown,             // Instance query teardown callback
    NULL,                                   // Instance teardown start callback
    NULL,                                   // Instance teardown complete callback
    NULL,                                   // Generate filename callback
    NULL,                                   // Normalize name component callback
    NULL,                                   // Normalize context cleanup callback
    NULL                                    // Transaction notification callback
  };

  PRINT_STATUS("DriverEntry starting...\n");

  UNREFERENCED_PARAMETER(__registryPath);

  RtlSecureZeroMemory(&_global, sizeof(_global));

  _global._filterDriverObject = __driverObj;
  _global._currTag = START_TAG;

  __resStatus = FltRegisterFilter(
    __driverObj,
    &__filterRegistration,
    &_global._filter
  );

  if (!NT_SUCCESS(__resStatus)) {
    PRINT_ERROR("FltRegisterFilter", __resStatus);
    return __resStatus;
  } else { PRINT_SUCCESS("FltRegisterFilter"); }

  __resStatus = FSFltCreateCDO(__driverObj);
  if (!NT_SUCCESS(__resStatus)) {
    PRINT_ERROR("FSFltCreateCDO", __resStatus);
    FltUnregisterFilter(_global._filter);
    return __resStatus;
  } else { PRINT_SUCCESS("FSFltCreateCDO"); }

  __resStatus = FltStartFiltering(
    _global._filter
  );

  if (!NT_SUCCESS(__resStatus)) {
    PRINT_ERROR("FltStartFiltering", __resStatus);
    FSFltDeleteCDO();
    FltUnregisterFilter(_global._filter);
    return __resStatus;
  } else { PRINT_SUCCESS("FltStartFiltering"); }

  PRINT_SUCCESS("DriverEntry");
  return __resStatus;
}

NTSTATUS
FSFltUnload(
  _In_ FLT_FILTER_UNLOAD_FLAGS __flags
) {
  NTSTATUS __resStatus = STATUS_SUCCESS;
  LONG __result = FSFLT_ERROR_SUCCESS;

  UNREFERENCED_PARAMETER(__flags);

  PAGED_CODE();
  PRINT_STATUS("FSFltUnload starting...\n");

  if (FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_CDO_OPEN_REF)
    && FlagOn(__flags, FLTFL_FILTER_UNLOAD_MANDATORY)) {
    PRINT_STATUS("Unloading driver is optional and the CDO is open\n");
    PRINT_ERROR("FSFltUnload", STATUS_UNSUCCESSFUL);
    return STATUS_FLT_DO_NOT_DETACH;
  }

  if (FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_LOAD_IMAGE_SET)) {
    __resStatus = FSFltRemoveLoadImageNotify(&__result);
    if (!NT_SUCCESS(__resStatus)) {
      PRINT_ERROR("FSFltRemoveLoadImageNotify", __resStatus);
      return STATUS_FLT_DO_NOT_DETACH;
    } else { PRINT_SUCCESS("FSFltRemoveLoadImageNotify"); }
  }

  if (FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_LOG_FILE_OPENED)) {
    ZwClose(_global._filterLogFile);
    ClearFlag(_global._filterFlags, GLOBAL_DATA_FLAG_LOG_FILE_OPENED);
  }

  FltUnregisterFilter(_global._filter);

  PRINT_SUCCESS("FltUnregisterFilter");

  FSFltDeleteCDO();

  PRINT_SUCCESS("FSFltUnload");

  return STATUS_SUCCESS;
}

NTSTATUS
FSFltInstanceQueryTeardown(
  _In_ PCFLT_RELATED_OBJECTS __fltObjects,
  _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS __flags
) {
  UNREFERENCED_PARAMETER(__fltObjects);
  UNREFERENCED_PARAMETER(__flags);

  PAGED_CODE();

  PRINT_SUCCESS("FSFltInstanceQueryTeardown");

  return STATUS_SUCCESS;
}
