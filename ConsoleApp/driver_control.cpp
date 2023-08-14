#ifndef _CONSOLE_APP
#define _CONSOLE_APP
#endif // !_CONSOLE_APP

#include "../file_sys_filter.h"
#include "driver_control.h"

#include <iostream>
#include <iomanip>
#include <windows.h>
#include <setupapi.h>

VOID
DriverControl::
Start(PRESULT __res) {
  BOOL __bRes;
  SC_HANDLE __hService = NULL;

  _GetServiceHandle(&__hService, __res);

  if (!FSFLT_SUCCESS(__res->_internalErrCode)) { return; }
  
  __bRes = StartServiceW(__hService, 0, NULL);

  if (__bRes == FALSE) {
    InitResult(
      __res,
      L"StartServiceW",
      GetLastError(),
      FSFLT_DRIVER_CONTROL_ERROR_START_SERVICE
    );

    CloseServiceHandle(__hService);

    return;
  }

  _WaitForServiceState(__hService, SERVICE_RUNNING, __res);

  if (!FSFLT_SUCCESS(__res->_internalErrCode)) { return; }

  CloseServiceHandle(__hService);

  InitResult(
    __res,
    L"Start",
    ERROR_SUCCESS,
    FSFLT_ERROR_SUCCESS
  );
}

VOID
DriverControl::
Stop(PRESULT __res) {
  BOOL __bRes;
  SC_HANDLE __hService = NULL;
  SERVICE_STATUS __serviceStatus;

  _GetServiceHandle(&__hService, __res);

  if (!FSFLT_SUCCESS(__res->_internalErrCode)) { return; }

  __bRes = ControlService(__hService, SERVICE_CONTROL_STOP, &__serviceStatus);

  if (__bRes == FALSE) {
    InitResult(
      __res,
      L"ControlService",
      GetLastError(),
      FSFLT_DRIVER_CONTROL_ERROR_STOP_SERVICE
    );

    CloseServiceHandle(__hService);
    return;
  }

  _WaitForServiceState(__hService, SERVICE_STOPPED, __res);

  CloseServiceHandle(__hService);
}

VOID
DriverControl::
Install(PRESULT __res) {
  SC_HANDLE __hService = NULL;
  std::wstring __relPath;
  std::wstring __fullPath;

  _GetServiceHandle(&__hService, __res);

  if (FSFLT_SUCCESS(__res->_internalErrCode)) {
    InitResult(
      __res,
      L"Install",
      ERROR_SUCCESS,
      FSFLT_DRIVER_CONTROL_ERROR_SERVICE_ALREADY_INSTALLED
    );

    CloseServiceHandle(__hService);

    return;
  }

  __relPath = L"..\\x64";
#ifdef _DEBUG
  __relPath += L"\\Debug";
#else
  __relPath += L"\\Release";
#endif // DEBUG
  __relPath += L"\\" DRIVER_NAME "\\" DRIVER_NAME ".inf";            

  _GetFullPath(__relPath, __fullPath, __res);

  if (!FSFLT_SUCCESS(__res->_internalErrCode)) { return; }

  std::wstring __cmdLine(L"DefaultInstall 132 ");
  __cmdLine += __fullPath;

  InstallHinfSectionW(NULL, NULL, __cmdLine.data(), 0);

  InitResult(
    __res,
    L"Install",
    ERROR_SUCCESS,
    FSFLT_ERROR_SUCCESS
  );
}

VOID
DriverControl::
Uninstall(PRESULT __res) {
  SC_HANDLE __hService = NULL;
  std::wstring __relPath;
  std::wstring __fullPath;

  _GetServiceHandle(&__hService, __res);

  if (!FSFLT_SUCCESS(__res->_internalErrCode)) {
    InitResult(
      __res,
      L"Install",
      ERROR_SUCCESS,
      FSFLT_DRIVER_CONTROL_ERROR_SERVICE_ALREADY_UNINSTALLED
    );

    return;
  }

  CloseServiceHandle(__hService);

  __relPath = L"..\\x64";
#ifdef _DEBUG
  __relPath += L"\\Debug";
#else
  __relPath += L"\\Release";
#endif // DEBUG
  __relPath += L"\\" DRIVER_NAME "\\" DRIVER_NAME ".inf";            

  _GetFullPath(__relPath, __fullPath, __res);

  if (!FSFLT_SUCCESS(__res->_internalErrCode)) { return; }

  std::wstring __cmdLine(L"DefaultUninstall 132 ");
  __cmdLine += __fullPath;

  InstallHinfSectionW(NULL, NULL, __cmdLine.data(), 0);

  InitResult(
    __res,
    L"Install",
    ERROR_SUCCESS,
    FSFLT_ERROR_SUCCESS
  );
}

VOID
DriverControl::
EnableLoadImageNotify(PRESULT __res) {
  DRIVER_IO __ioctlInfo;
  _SendIOCTLCode(IOCTL_SET_LOAD_IMAGE, &__ioctlInfo, __res);
}

VOID
DriverControl::
DisableLoadImageNotify(PRESULT __res) {
  DRIVER_IO __ioctlInfo;
  _SendIOCTLCode(IOCTL_REMOVE_LOAD_IMAGE, &__ioctlInfo, __res);
}

VOID
DriverControl::
AddRule(
  std::wstring& __fileName,
  std::wstring& __procName,
  DWORD __accessMask,
  PRESULT __res
) { 
  DRIVER_IO __ioctlInfo;

  std::swprintf(
    __ioctlInfo._ruleInfo._fileName,
    FILE_BUFFER_SIZE,
    __fileName.c_str()
  );
  std::swprintf(
    __ioctlInfo._ruleInfo._procName,
    PROCESS_BUFFER_SIZE,
    __procName.c_str()
  );
  __ioctlInfo._ruleInfo._accessMask = __accessMask;

  _SendIOCTLCode(IOCTL_ADD_RULE, &__ioctlInfo, __res);
}

VOID
DriverControl::
DelRule(
  std::wstring& __fileName,
  std::wstring& __procName,
  PRESULT __res
) { 
  DRIVER_IO __ioctlInfo;

  std::swprintf(
    __ioctlInfo._ruleInfo._fileName,
    FILE_BUFFER_SIZE,
    __fileName.c_str()
  );
  std::swprintf(
    __ioctlInfo._ruleInfo._procName,
    PROCESS_BUFFER_SIZE,
    __procName.c_str()
  );

  _SendIOCTLCode(IOCTL_DEL_RULE, &__ioctlInfo, __res);
}

VOID
DriverControl::
PrintRules() {
  DRIVER_IO __ioctlInfo;
  RESULT __res;

  _SendIOCTLCode(IOCTL_PRINT_RULES, &__ioctlInfo, &__res);
}

VOID
DriverControl::
ClearRules(PRESULT __res) {
  DRIVER_IO __ioctlInfo;

  _SendIOCTLCode(IOCTL_CLEAR_RULES, &__ioctlInfo, __res);
}

VOID
DriverControl::
_GetServiceHandle(SC_HANDLE* __hService, PRESULT __res) {
  SC_HANDLE __hSCM = NULL;
  *__hService = NULL;

  __hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);

  if (__hSCM == NULL) {
    InitResult(
      __res,
      L"OpenSCManagerW",
      GetLastError(),
      FSFLT_DRIVER_CONTROL_ERROR_OPEN_SCM
    );

    return;
  }

  *__hService = OpenServiceW(__hSCM, DRIVER_NAME, SERVICE_ALL_ACCESS);
  if (*__hService == NULL) {
    InitResult(
      __res,
      L"OpenServiceW",
      GetLastError(),
      FSFLT_DRIVER_CONTROL_ERROR_OPEN_SERVICE
    );

    CloseServiceHandle(__hSCM);

    return;
  }

  InitResult(
    __res,
    L"_GetServiceHandle",
    ERROR_SUCCESS,
    FSFLT_ERROR_SUCCESS
  );

  CloseServiceHandle(__hSCM);
}

VOID
DriverControl::
_SendIOCTLCode(
  DWORD __ioctlCode,
  PDRIVER_IO __drvIo,
  PRESULT __res
) {
  BOOL __bRes;
  HANDLE __hDriver = NULL;
  DWORD __bytesRet;

  if (__drvIo == NULL) {
    InitResult(
      __res,
      L"_SendIOCTLCode",
      ERROR_INVALID_PARAMETER,
      FSFLT_DRIVER_CONTROL_ERROR_INVALID_PARAMETER
    );

    return;
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
    InitResult(
      __res,
      L"CreateFileW",
      GetLastError(),
      FSFLT_DRIVER_CONTROL_ERROR_OPEN_DRIVER_BY_LINK
    );

    return;
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

  InitResult(
    __res,
    L"DeviceIoControl",
    (__bRes) ? ERROR_SUCCESS : GetLastError(),
    (__bRes) ? FSFLT_ERROR_SUCCESS : __drvIo->_result._internalErrCode
  );

  CloseHandle(__hDriver);
}

VOID
DriverControl::
_WaitForServiceState(
  SC_HANDLE __hService,
  DWORD __serviceState,
  PRESULT __res 
) {
  DWORD __currState;

  while (TRUE) {
    _GetServiceState(__hService, &__currState, __res);

    if (!FSFLT_SUCCESS(__res->_internalErrCode)) { return; }
    else if (__currState == __serviceState) { break; }

    Sleep(500);
  }

  InitResult(
    __res,
    L"_WaitForServiceState",
    ERROR_SUCCESS,
    FSFLT_ERROR_SUCCESS
  );
}

VOID
DriverControl::
_GetServiceState(
  SC_HANDLE __hService,
  DWORD* __currState,
  PRESULT __res
) {
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
    InitResult(
      __res,
      L"QueryServiceStatusEx",
      GetLastError(),
      FSFLT_DRIVER_CONTROL_ERROR_QUERY_SERVICE_STATE
    );
    return;
  }

  *__currState = __serviceStatus.dwCurrentState;

  InitResult(
    __res,
    L"_GetServiceState",
    ERROR_SUCCESS,
    FSFLT_ERROR_SUCCESS
  );
}

VOID
DriverControl::
_GetFullPath(
  std::wstring& __relPath,
  std::wstring& __fullPath,
  PRESULT __res
) {
  DWORD __length = 0;
  WCHAR* __tmpPath = nullptr;

  __length = GetFullPathNameW(__relPath.data(), __length, __tmpPath, NULL);

  if (__length == 0) {
    InitResult(
      __res,
      L"GetFullPathNameW",
      GetLastError(),
      FSFLT_DRIVER_CONTROL_ERROR_GET_FULL_PATH
    );

    return;
  }

  __tmpPath = new(std::nothrow) WCHAR[__length + 1];

  if (__tmpPath == nullptr) {
    InitResult(
      __res,
      L"new",
      ERROR_SUCCESS,
      FSFLT_DRIVER_CONTROL_ERROR_MEMORY_ALLOC
    );

    return;
  }
  
  std::memset(__tmpPath, 0x00, (__length + 1) * sizeof(WCHAR));

  __length = GetFullPathNameW(__relPath.data(), __length, __tmpPath, NULL);

  if (__length == 0) {
    InitResult(
      __res,
      L"GetFullPathNameW",
      GetLastError(),
      FSFLT_DRIVER_CONTROL_ERROR_GET_FULL_PATH
    );

    delete[] __tmpPath;

    return;
  }

  __fullPath = __tmpPath;
  delete[] __tmpPath;

  InitResult(
    __res,
    L"_GetFullPath",
    ERROR_SUCCESS,
    FSFLT_ERROR_SUCCESS
  );

  return;
}