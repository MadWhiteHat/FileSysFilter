#ifndef _REGISTRY_FILTER_H
#define _REGISTRY_FILTER_H

#define DRIVER_NAME L"FileSysDriver"

#define DRIVER_CDO_NAME L"\\FileSystem\\Filters\\" DRIVER_NAME
#define DRIVER_USERMODE_NAME L"\\??\\" DRIVER_NAME

#define IOCTL_UPDATE_RULES \
CTL_CODE ( \
  FILE_DEVICE_UNKNOWN, \
  (0x800 + 0), \
  METHOD_BUFFERED, \
  FILE_SPECIAL_ACCESS \
)
#define IOCTL_SET_LOAD_IMAGE \
CTL_CODE ( \
  FILE_DEVICE_UNKNOWN, \
  (0x800 + 1), \
  METHOD_BUFFERED, \
  FILE_SPECIAL_ACCESS \
)
#define IOCTL_REMOVE_LOAD_IMAGE \
CTL_CODE ( \
  FILE_DEVICE_UNKNOWN, \
  (0x800 + 2), \
  METHOD_BUFFERED, \
  FILE_SPECIAL_ACCESS \
)

// Error codes
#define DRIVER_ERROR_SUCCESS 0
#define DRIVER_ERROR_STATE 0x80000000
#define DRIVER_ERROR_LOG_FILE_ALREADY_OPENED 1 | DRIVER_ERROR_STATE
#define DRIVER_ERROR_LOG_FILE_NOT_OPENED 2 | DRIVER_ERROR_STATE
#define DRIVER_ERROR_LOG_FILE_CREATE_FAILED 3 | DRIVER_ERROR_STATE
#define DRIVER_ERROR_SET_NOTIFY_ROUTINE 4 | DRIVER_ERROR_STATE
#define DRIVER_ERROR_REMOVE_NOTIFY_ROUTINE 5 | DRIVER_ERROR_STATE


// Availble operations
#define ALLOW_READ 0x01
#define ALLOW_WRITE 0x02

typedef struct _DRIVER_IO {
  int _resVal;
} DRIVER_IO, *PDRIVER_IO;

#endif // !_REGISTRY_FILTER_H
