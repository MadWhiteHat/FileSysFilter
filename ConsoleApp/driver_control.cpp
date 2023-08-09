#ifndef _CONSOLE_APP
#define _CONSOLE_APP
#endif // !_CONSOLE_APP

#include "driver_control.h"
#include "../file_sys_filter.h"

#include <iostream>
#include <iomanip>
#include <windows.h>
#include <setupapi.h>

DriverControl::Result
DriverControl::
Start() {
  DriverControl::Result __res;
  BOOL __bRes;
  SC_HANDLE __hService = NULL;

  __res = _GetServiceHandle(&__hService);

  if (!FSFLT_SUCCESS(__res._internalErrCode)) { return __res; }
  
  __bRes = StartServiceW(__hService, 0, NULL);

  if (__bRes == FALSE) {
    __res._winFuncName = "StartServiceW";
    __res._winErrCode = GetLastError();
    __res._internalErrCode = FSFLT_DRIVER_CONTROL_ERROR_START_SERVICE;

    CloseServiceHandle(__hService);

    return __res;
  }

  __res = _WaitForServiceState(__hService, SERVICE_RUNNING);

  if (!FSFLT_SUCCESS(__res._internalErrCode)) { return __res; }

  CloseServiceHandle(__hService);
  
  __res._internalErrCode = FSFLT_ERROR_SUCCESS;
  __res._winErrCode = ERROR_SUCCESS;
  return __res;
}

DriverControl::Result
DriverControl::
Stop() {
  DriverControl::Result __res;
  BOOL __bRes;
  SC_HANDLE __hService = NULL;
  SERVICE_STATUS __serviceStatus;

  __res = _GetServiceHandle(&__hService);

  if (!FSFLT_SUCCESS(__res._internalErrCode)) { return __res; }

  __bRes = ControlService(__hService, SERVICE_CONTROL_STOP, &__serviceStatus);

  if (__bRes == FALSE) {
    __res._winFuncName = "ControlService";
    __res._winErrCode = GetLastError();
    __res._internalErrCode = FSFLT_DRIVER_CONTROL_ERROR_STOP_SERVICE;

    CloseServiceHandle(__hService);

    return __res;
  }
  __res = _WaitForServiceState(__hService, SERVICE_STOPPED);

  CloseServiceHandle(__hService);

  return __res;;
}

DriverControl::Result
DriverControl::
Install() {
  DriverControl::Result __res;
  SC_HANDLE __hService = NULL;
  std::wstring __relPath;
  std::wstring __fullPath;

  __res = _GetServiceHandle(&__hService);

  if (FSFLT_SUCCESS(__res._internalErrCode)) {
    __res._internalErrCode =
      FSFLT_DRIVER_CONTROL_ERROR_SERVICE_ALREADY_INSTALLED;
    __res._winErrCode = ERROR_SUCCESS;

    CloseServiceHandle(__hService);

    return __res;
  }

  __relPath = L"..\\x64";
#ifdef _DEBUG
  __relPath += L"\\Debug";
#else
  __relPath += L"\\Release";
#endif // DEBUG
  __relPath += L"\\" DRIVER_NAME "\\" DRIVER_NAME ".inf";            

  __res = _GetFullPath(__relPath, __fullPath);

  if (!FSFLT_SUCCESS(__res._internalErrCode)) { return __res; }

  std::wstring __cmdLine(L"DefaultInstall 132 ");
  __cmdLine += __fullPath;

  InstallHinfSectionW(NULL, NULL, __cmdLine.data(), 0);

  __res._winErrCode = ERROR_SUCCESS;
  __res._internalErrCode = FSFLT_ERROR_SUCCESS;

  return __res;
}

DriverControl::Result
DriverControl::
Uninstall() {
  DriverControl::Result __res;
  SC_HANDLE __hService = NULL;
  std::wstring __relPath;
  std::wstring __fullPath;

  __res = _GetServiceHandle(&__hService);

  if (!FSFLT_SUCCESS(__res._internalErrCode)) {
    __res._internalErrCode
      = FSFLT_DRIVER_CONTROL_ERROR_SERVICE_ALREADY_UNINSTALLED;
    __res._winErrCode = ERROR_SUCCESS;

    return __res;
  }

  CloseServiceHandle(__hService);

  __relPath = L"..\\x64";
#ifdef _DEBUG
  __relPath += L"\\Debug";
#else
  __relPath += L"\\Release";
#endif // DEBUG
  __relPath += L"\\" DRIVER_NAME "\\" DRIVER_NAME ".inf";            

  __res = _GetFullPath(__relPath, __fullPath);

  if (!FSFLT_SUCCESS(__res._internalErrCode)) { return __res; }

  std::wstring __cmdLine(L"DefaultUninstall 132 ");
  __cmdLine += __fullPath;

  InstallHinfSectionW(NULL, NULL, __cmdLine.data(), 0);

  __res._winErrCode = ERROR_SUCCESS;
  __res._internalErrCode = FSFLT_ERROR_SUCCESS;

  return __res;
}

DriverControl::Result
DriverControl::
EnableLoadImageNotify() {
  DRIVER_IO __ioctlInfo;
  return _SendIOCTLCode(IOCTL_SET_LOAD_IMAGE, &__ioctlInfo);
}

DriverControl::Result
DriverControl::
DisableLoadImageNotify() {
  DRIVER_IO __ioctlInfo;
  return _SendIOCTLCode(IOCTL_REMOVE_LOAD_IMAGE, &__ioctlInfo);
}

DriverControl::Result
DriverControl::
AddRule() { 
  DRIVER_IO __ioctlInfo;

  return _SendIOCTLCode(IOCTL_ADD_RULE, &__ioctlInfo);
}

DriverControl::Result
DriverControl::
DelRule() { 
  DRIVER_IO __ioctlInfo;

  return _SendIOCTLCode(IOCTL_DEL_RULE, &__ioctlInfo);
}

DriverControl::Result
DriverControl::
_GetServiceHandle(SC_HANDLE* __hService) {
  DriverControl::Result __res;
  SC_HANDLE __hSCM = NULL;
  *__hService = NULL;

  __hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);

  if (__hSCM == NULL) {
    __res._winFuncName = "OpenSCManagerW";
    __res._winErrCode = GetLastError();
    __res._internalErrCode = FSFLT_DRIVER_CONTROL_ERROR_OPEN_SCM;
    return __res;
  }

  *__hService = OpenServiceW(__hSCM, DRIVER_NAME, SERVICE_ALL_ACCESS);

  if (*__hService == NULL) {
    __res._winFuncName = "OpenServiceW";
    __res._winErrCode = GetLastError();
    __res._internalErrCode = FSFLT_DRIVER_CONTROL_ERROR_OPEN_SERVICE;

    CloseServiceHandle(__hSCM);

    return __res;
  }

  __res._internalErrCode = FSFLT_ERROR_SUCCESS;
  __res._winErrCode = ERROR_SUCCESS;

  CloseServiceHandle(__hSCM);

  return __res;
}

DriverControl::Result
DriverControl::
_SendIOCTLCode(DWORD __ioctlCode, PDRIVER_IO __drvIo) {
  DriverControl::Result __res;
  BOOL __bRes;
  HANDLE __hDriver = NULL;
  DWORD __bytesRet;

  if (__drvIo == NULL) {
    __res._winErrCode = ERROR_INVALID_PARAMETER;
    __res._winFuncName = "IsNull";
    __res._internalErrCode = FSFLT_DRIVER_CONTROL_ERROR_INVALID_PARAMETER;

    return __res;
  }

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
    __res._winFuncName = "CreateFileW";
    __res._winErrCode = GetLastError();
    __res._internalErrCode = FSFLT_DRIVER_CONTROL_ERROR_OPEN_DRIVER_BY_LINK;

    return __res;
  }

  __bRes = DeviceIoControl(
    __hDriver,
    __ioctlCode,
    __drvIo,
    sizeof(DRIVER_IO),
    __drvIo,
    sizeof(DRIVER_IO),
    &__bytesRet,
    NULL
  );

  __res._internalErrCode = __drvIo->_result;

  if (__bRes == FALSE) {
    __res._winFuncName = "DeviceIoControl";
    __res._winErrCode = GetLastError();
  } else { __res._winErrCode = ERROR_SUCCESS; }

  CloseHandle(__hDriver);

  return __res;
}

DriverControl::Result
DriverControl::
_WaitForServiceState(SC_HANDLE __hService, DWORD __serviceState) {
  DriverControl::Result __res;
  DWORD __currState;

  while (TRUE) {
    __res = _GetServiceState(__hService, &__currState);

    if (!FSFLT_SUCCESS(__res._internalErrCode)) { break; }
    else if (__currState == __serviceState) { break; }

    Sleep(500);
  }

  return __res;
}

DriverControl::Result
DriverControl::
_GetServiceState(SC_HANDLE __hService, DWORD* __currState) {

  DriverControl::Result __res;
  BOOL __bRes;
  SERVICE_STATUS_PROCESS __serviceStatus;
  DWORD __bytesNeeded;

  __bRes = QueryServiceStatusEx(
    __hService,
    SC_STATUS_PROCESS_INFO,
    reinterpret_cast<LPBYTE>(&__serviceStatus),
    sizeof(__serviceStatus),
    &__bytesNeeded
  );

  if (__bRes == FALSE) {
    __res._winFuncName = "QueryServiceStatusEx";
    __res._winErrCode = GetLastError();
    __res._internalErrCode = FSFLT_DRIVER_CONTROL_ERROR_QUERY_SERVICE_STATE;
  } else { *__currState = __serviceStatus.dwCurrentState; }

  __res._winErrCode = ERROR_SUCCESS;
  __res._internalErrCode = FSFLT_ERROR_SUCCESS;

  return __res;
}

DriverControl::Result
DriverControl::
_GetFullPath(std::wstring& __relPath, std::wstring& __fullPath) {
  DriverControl::Result __res;
  DWORD __length = 0;
  WCHAR* __tmpPath = nullptr;

  __length = GetFullPathNameW(__relPath.data(), __length, __tmpPath, nullptr);

  if (__length == 0) {
    __res._winFuncName = "GetFullPathNameW";
    __res._winErrCode = GetLastError();
    __res._internalErrCode = FSFLT_DRIVER_CONTROL_ERROR_GET_FULL_PATH;

    return __res;
  }

  __tmpPath = new(std::nothrow) WCHAR[__length + 1];

  if (__tmpPath == nullptr) {
    __res._winFuncName = "new";
    __res._winErrCode = ERROR_SUCCESS;
    __res._internalErrCode = FSFLT_DRIVER_CONTROL_ERROR_MEMORY_ALLOC;

    return __res;
  }
  
  std::memset(__tmpPath, 0x00, (__length + 1) * sizeof(WCHAR));

  __length = GetFullPathNameW(__relPath.data(), __length, __tmpPath, nullptr);

  if (__length == 0) {
    __res._winFuncName = "GetFullPathNameW";
    __res._winErrCode = GetLastError();
    __res._internalErrCode = FSFLT_DRIVER_CONTROL_ERROR_GET_FULL_PATH;

    delete[] __tmpPath;

    return __res;
  }

  __fullPath = __tmpPath;
  delete[] __tmpPath;

  __res._winErrCode = ERROR_SUCCESS;
  __res._internalErrCode = FSFLT_ERROR_SUCCESS;

  return __res;
}