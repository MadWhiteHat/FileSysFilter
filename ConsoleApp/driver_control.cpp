#include "driver_control.h"

#include <iostream>
#include <iomanip>
#include <windows.h>
#include <setupapi.h>

void
DriverControl::
Start() {
  BOOL __res = FALSE;
  SC_HANDLE __hService = NULL;
  __res = this->_GetServiceHandle(&__hService);
  if (__res == FALSE) {
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
  BOOL __res = FALSE;
  SC_HANDLE __hService = NULL;
  SERVICE_STATUS __serviceStatus;
  __res = this->_GetServiceHandle(&__hService);
  if (__res == FALSE) {
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
Install() {
  BOOL __res = FALSE;
  SC_HANDLE __hService = NULL;
  __res = this->_GetServiceHandle(&__hService);
  if (__res == TRUE) {
    std::cout << "Driver has been already installed!\n";
    CloseServiceHandle(__hService);
    return;
  }

  std::wstring __path(L"..\\x64");
#ifdef _DEBUG
  __path += L"\\Debug";
#else
  __path += L"\\Release";
#endif // DEBUG
  __path += L"\\" DRIVER_NAME "\\" DRIVER_NAME ".inf";            
  std::wcout << __path << std::endl;
  DWORD __length = 0;
  WCHAR* __fullPath = nullptr;

  __length = GetFullPathNameW(__path.data(), __length, __fullPath, nullptr);
  if (__length == 0) {
    _PrintError("GetFullPathNameW", GetLastError());
    return;
  }
  __fullPath = new(std::nothrow) WCHAR[__length + 1];
  if (__fullPath == nullptr) {
    _PrintError("Memory allocation", 0);
    return;
  }
  std::memset(__fullPath, 0x00, (__length + 1) * sizeof(WCHAR));
  __length = GetFullPathNameW(__path.data(), __length, __fullPath, nullptr);
  if (__length == 0) {
    _PrintError("GetFullPathNameW", GetLastError());
    delete[] __fullPath;
    return;
  }

  std::wstring __cmdLine(L"DefaultInstall 132 ");
  __cmdLine += __fullPath;
  delete[] __fullPath;

  InstallHinfSectionW(NULL, NULL, __cmdLine.data(), 0);

  return;
}

void
DriverControl::
Uninstall() {
  BOOL __res = FALSE;
  SC_HANDLE __hService = NULL;
  __res = this->_GetServiceHandle(&__hService);
  if (__res == FALSE) {
    this->_PrintError("_GetServiceHandle", GetLastError());
    return;
  }
  CloseServiceHandle(__hService);

  std::wstring __path(L"..\\x64");
#ifdef _DEBUG
  __path += L"\\Debug";
#else
  __path += L"\\Release";
#endif // DEBUG
  __path += L"\\" DRIVER_NAME "\\" DRIVER_NAME ".inf";            
  DWORD __length = 0;
  WCHAR* __fullPath = nullptr;

  __length = GetFullPathNameW(__path.data(), __length, __fullPath, nullptr);
  if (__length == 0) {
    _PrintError("GetFullPathNameW", GetLastError());
    return;
  }
  __fullPath = new(std::nothrow) WCHAR[__length + 1];
  if (__fullPath == nullptr) {
    _PrintError("Memory allocation", 0);
    return;
  }
  std::memset(__fullPath, 0x00, (__length + 1) * sizeof(WCHAR));
  __length = GetFullPathNameW(__path.data(), __length, __fullPath, nullptr);
  if (__length == 0) {
    _PrintError("GetFullPathNameW", GetLastError());
    delete[] __fullPath;
    return;
  }

  std::wstring __cmdLine(L"DefaultUninstall 132 ");
  __cmdLine += __fullPath;
  delete[] __fullPath;

  InstallHinfSectionW(NULL, NULL, __cmdLine.data(), 0);

  return;
}

void
DriverControl::
EnableLoadImageNotify() { this->_SendIOCTLCode(IOCTL_SET_LOAD_IMAGE); }

void
DriverControl::
DisableLoadImageNotify() { this->_SendIOCTLCode(IOCTL_REMOVE_LOAD_IMAGE); }

void
DriverControl::
Update() { this->_SendIOCTLCode(IOCTL_UPDATE_RULES); }

BOOL
DriverControl::
_GetServiceHandle(SC_HANDLE* __hService) {
  BOOL __res = FALSE;
  SC_HANDLE __hSCM = NULL;
  *__hService = NULL;

  __hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (__hSCM == NULL) {
    _PrintError("OpenSCManagerW", GetLastError());
    return __res;
  }
  *__hService = OpenServiceW(__hSCM, DRIVER_NAME, SERVICE_ALL_ACCESS);
  if (*__hService == NULL) { _PrintError("OpenServiceW", GetLastError()); }
  else { __res = TRUE; }
  CloseServiceHandle(__hSCM);
  return __res;
}

void
DriverControl::
_SendIOCTLCode(DWORD __ioctlCode) {
  HANDLE __hDriver = NULL;
  BOOL __res = FALSE;
  DRIVER_IO __output = { 0 };
  DWORD __bytesRet = 0;

  __hDriver = CreateFileW(
    DRIVER_USERMODE_NAME,
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
  std::cout << "Driver returns: " << std::hex << __output._resVal << std::endl;
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
  std::cout << __funcName << " failed with: 0x";
  std::cout << std::setw(8) << std::setfill('0') << std::right
    << std::hex << __errCode << '\n';
}