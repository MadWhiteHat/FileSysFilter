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

  VOID
  Start(PRESULT __res);

  VOID
  Stop(PRESULT __res);

  VOID
  Install(PRESULT __res);

  VOID
  Uninstall(PRESULT __res);

  VOID
  EnableLoadImageNotify(PRESULT __res);

  VOID
  DisableLoadImageNotify(PRESULT __res);

  VOID
  AddRule(
    std::wstring& __fileName,
    std::wstring& __procName,
    DWORD __accessMask,
    PRESULT __res
  );

  VOID
  DelRule(
    std::wstring& __fileName,
    std::wstring& __procName,
    PRESULT __res
  );

  VOID
  PrintRules();

  VOID
  ClearRules(PRESULT __res);

private:
  VOID
  _GetServiceHandle(SC_HANDLE* __hService, PRESULT __res);

  VOID
  _WaitForServiceState(
    SC_HANDLE __hService,
    DWORD __serviceState,
    PRESULT __res
  );

  VOID
  _GetServiceState(
    SC_HANDLE __hService,
    DWORD* __currState,
    PRESULT __res
  ); 

  VOID
  _GetFullPath(
    std::wstring& __relPath,
    std::wstring& __fullPath,
    PRESULT __res
  );

  VOID
  _SendIOCTLCode(
    DWORD __ioctlCode,
    PDRIVER_IO __drvIO,
    PRESULT __res
  );
};

#endif // !_DRIVER_CONTROL_H
