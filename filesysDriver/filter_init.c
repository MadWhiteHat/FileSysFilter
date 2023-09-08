#include "filter.h"
#include "rules_list.h"


#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, FSFltUnload)
#pragma alloc_text(PAGE, FSFltInstanceQueryTeardown)

#endif // ALLOC_PRAGMA

CONST FLT_OPERATION_REGISTRATION _callbacks[] = {
  {
    IRP_MJ_CREATE,
    0,
    FSFltPreOperation,
    NULL
  },

  {
    IRP_MJ_READ,
    0,
    FSFltPreOperation,
    NULL
  },

  {
    IRP_MJ_WRITE,
    0,
    FSFltPreOperation,
    NULL
  },

 { IRP_MJ_OPERATION_END }
};

CONST FLT_REGISTRATION _registration = {
  sizeof(FLT_REGISTRATION),               // Size
  FLT_REGISTRATION_VERSION,               // Version
  0,                                      // Flags
  NULL,                                   // Context
  _callbacks,                             // Operation callbacks
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

FILTER_GLOBAL_DATA _global;

NTSTATUS
DriverEntry(
  _In_ PDRIVER_OBJECT  __driverObj,
  _In_ PUNICODE_STRING __registryPath) {

  NTSTATUS __res = STATUS_SUCCESS;

  PRINT_STATUS("DriverEntry starting...\n");

  UNREFERENCED_PARAMETER(__registryPath);

  RtlSecureZeroMemory(&_global, sizeof(_global));

  _global._filterDriverObject = __driverObj;
  _global._rulesList._curTag = START_TAG;
  
  __res = FltRegisterFilter(
    __driverObj,
    &_registration,
    &_global._filter
  );

  if (!NT_SUCCESS(__res)) {
    PRINT_ERROR("FltRegisterFilter", __res);
    return __res;
  } else { PRINT_SUCCESS("FltRegisterFilter"); }

  __res = FSFltCreateCDO(__driverObj);
  if (!NT_SUCCESS(__res)) {
    PRINT_ERROR("FSFltCreateCDO", __res);
    FltUnregisterFilter(_global._filter);
    return __res;
  } else { PRINT_SUCCESS("FSFltCreateCDO"); }

  SetFlag(_global._filterFlags, GLOBAL_DATA_FLAG_ENABLE_FILTERING);

  __res = FltStartFiltering(_global._filter);
  if (!NT_SUCCESS(__res)) {
    PRINT_ERROR("FltStartFiltering", __res);
    ClearFlag(_global._filterFlags, GLOBAL_DATA_FLAG_ENABLE_FILTERING);
    FSFltDeleteCDO();
    FltUnregisterFilter(_global._filter);
    return __res;
  } else { PRINT_SUCCESS("FltStartFiltering"); }


  PRINT_SUCCESS("DriverEntry");
  return __res;
}

NTSTATUS
FSFltUnload(
  _In_ FLT_FILTER_UNLOAD_FLAGS __flags
) {
  NTSTATUS __res = STATUS_SUCCESS;
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
    __res = FSFltRemoveLoadImageNotify(&__result);
    if (!NT_SUCCESS(__res)) {
      PRINT_ERROR("FSFltRemoveLoadImageNotify", __res);
      return STATUS_FLT_DO_NOT_DETACH;
    } else { PRINT_SUCCESS("FSFltRemoveLoadImageNotify"); }
  }

  if (FlagOn(_global._filterFlags, GLOBAL_DATA_FLAG_LOG_FILE_OPENED)) {
    ZwClose(_global._filterLogFile);
    ClearFlag(_global._filterFlags, GLOBAL_DATA_FLAG_LOG_FILE_OPENED);
  }

  FltUnregisterFilter(_global._filter);

  PRINT_SUCCESS("FltUnregisterFilter");

  __res = DelAllRules(&(_global._rulesList), &__result);
  if (!NT_SUCCESS(__res)) {
    PRINT_ERROR("DelAllRules", __res);
    return STATUS_FLT_DO_NOT_DETACH;
  } else { PRINT_SUCCESS("DelAllRules"); }

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
