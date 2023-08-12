#ifndef _DRIVER_CONTROL_H
#define _DRIVER_CONTROL_H

#include <iostream>
#include <windows.h>

#ifndef _CONSOLE_APP
#define _CONSOLE_APP
#endif // !_CONSOLE_APP

#include "../file_sys_filter.h"

class DriverControl {
public:

  struct Result {
    std::string _winFuncName;
    DWORD _winErrCode;
    DWORD _internalErrCode;
  };

  DriverControl::Result
  Start();

  DriverControl::Result
  Stop();

  DriverControl::Result
  Install();

  DriverControl::Result
  Uninstall();

  DriverControl::Result
  EnableLoadImageNotify();

  DriverControl::Result
  DisableLoadImageNotify();

  DriverControl::Result
  AddRule(
    std::wstring& __fileName,
    std::wstring& __procName,
    DWORD __accessMask
  );

  DriverControl::Result
  DelRule(
    std::wstring& __fileName,
    std::wstring& __procName
  );

private:
  DriverControl::Result
  _GetServiceHandle(SC_HANDLE* __hService);
  DriverControl::Result
  _WaitForServiceState(
    SC_HANDLE __hService,
    DWORD __serviceState
  );
  DriverControl::Result
  _GetServiceState(
    SC_HANDLE __hService,
    DWORD* __currState
  ); 

  DriverControl::Result
  _GetFullPath(
    std::wstring& __relPath,
    std::wstring& __fullPath
  );

  DriverControl::Result
  _SendIOCTLCode(DWORD __ioctlCode, PDRIVER_IO __drvIO);
};

#endif // !_DRIVER_CONTROL_H
