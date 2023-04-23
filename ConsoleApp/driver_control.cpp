#include "driver_control.h"

void
DriverControl::
Start() {
  SC_HANDLE __hService = this->_GetServiceHandle();
  BOOL __res = FALSE;
  if (__hService == NULL) {
    this->_PrintError("_GetServiceHandle", GetLastError());
    return;
  }
  __res = StartServiceW(__hService, 0, NULL);
  if (__res == FALSE) {
    if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) {
      std::cout << "Service already running" << std::endl;
    } else {
      _PrintError("StartServiceW", GetLastError());
      CloseServiceHandle(__hService);
      return;
    }
  }
  __res = _WaitForServiceState(__hService, SERVICE_RUNNING);
  if (__res  == FALSE) { _PrintError("_WaitForServiceState", GetLastError()); }
  CloseServiceHandle(__hService);
  return;
}

void
DriverControl::
Stop() {
  SC_HANDLE __hService = this->_GetServiceHandle();
  SERVICE_STATUS __serviceStatus;
  BOOL __res = FALSE;
  if (__hService == NULL) {
    this->_PrintError("_GetServiceHandle", GetLastError());
    return;
  }
  __res = ControlService(__hService, SERVICE_CONTROL_STOP, &__serviceStatus);
  if (__res == FALSE) {
    if (GetLastError() == ERROR_SERVICE_NOT_ACTIVE) {
      std::cout << "Service already has been stooped" << std::endl;
    } else {
      _PrintError("ControlService", GetLastError());
      CloseServiceHandle(__hService);
      return;
    }
  }
  __res = _WaitForServiceState(__hService, SERVICE_STOPPED);
  if (__res  == FALSE) { _PrintError("_WaitForServiceState", GetLastError()); }
  CloseServiceHandle(__hService);
  return;
}

void
DriverControl::
EnableCreateThreadNotify() { this->_SendIOCTLCode(IOCTL_SET_CREATE_THREAD); }

void
DriverControl::
DisableCreateThreadNotify() {
  this->_SendIOCTLCode(IOCTL_REMOVE_CREATE_THREAD);
}

void
DriverControl::
Update() { this->_SendIOCTLCode(IOCTL_DO_UPDATE_RULES); }

SC_HANDLE
DriverControl::_GetServiceHandle() {
  SC_HANDLE __hSCM = NULL;
  SC_HANDLE __hService = NULL;

  __hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (__hSCM != NULL) {
    __hService = OpenServiceW(__hSCM, DRIVER_NAME, SERVICE_ALL_ACCESS);
    if (__hService == NULL) {
      this->_PrintError("OpenServiceW", GetLastError());
      CloseServiceHandle(__hSCM);
    }
  } else { _PrintError("OpenSCManagerW", GetLastError()); }

  return __hService;
}

void
DriverControl::
_SendIOCTLCode(DWORD __ioctlCode) {
  HANDLE __hDriver = NULL;
  BOOL __res = FALSE;
  DRIVER_IOCTL_OUTPUT __output = { 0 };
  DWORD __bytesRet = 0;

  __hDriver = CreateFileW(
    KERNEL_MODE_DRIVER_NAME,
    GENERIC_READ | GENERIC_WRITE,
    0,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_SYSTEM,
    NULL
  );
  if (__hDriver == INVALID_HANDLE_VALUE) {
    _PrintError("CreateFileW", GetLastError());
    return;
  }
  __res = DeviceIoControl(
    __hDriver,
    __ioctlCode,
    NULL,
    0,
    &__output,
    sizeof(__output),
    &__bytesRet,
    NULL
  );
  if (__res == FALSE) {
    _PrintError("DeviceIoControl", GetLastError());
    CloseHandle(__hDriver);
    return;
  }
  std::cout << "Driver returns: " << std::hex << __output._retVal << std::endl;
  CloseHandle(__hDriver);
}

BOOL
DriverControl::
_WaitForServiceState(SC_HANDLE __hService, DWORD __serviceState) {
  BOOL __res = FALSE;
  DWORD __currState;
  while (TRUE) {
    __res = _GetServiceState(__hService, &__currState);
    if (__res == FALSE) {
      _PrintError("_GetServiceState", GetLastError());
      break;
    } else if (__currState == __serviceState) { break; }
    Sleep(500);
  }
  return __res;
}

BOOL
DriverControl::
_GetServiceState(SC_HANDLE __hService, DWORD* __currState) {
  SERVICE_STATUS_PROCESS __serviceStatus;
  BOOL __res = FALSE;
  DWORD __bytesNeeded;

  __res = QueryServiceStatusEx(
    __hService,
    SC_STATUS_PROCESS_INFO,
    reinterpret_cast<LPBYTE>(&__serviceStatus),
    sizeof(__serviceStatus),
    &__bytesNeeded
  );
  if (__res == FALSE) { _PrintError("QueryServiceStatusEx", GetLastError()); }
  else { *__currState = __serviceStatus.dwCurrentState; }
  return __res;
}

inline
void
DriverControl::
_PrintError(std::string&& __funcName, DWORD __errCode) {
  std::cout << __funcName << " failed with: "
    << std::hex << __errCode << std::endl;
}