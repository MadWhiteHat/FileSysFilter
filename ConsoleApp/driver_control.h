#ifndef _DRIVER_CONTROL_H
#define _DRIVER_CONTROL_H

#include <iostream>
#include <windows.h>

#include "../file_sys_filter.h"

#define KERNEL_MODE_DRIVER_NAME L"\\\\.\\" DRIVER_NAME

class DriverControl {
public:
  void Start();
  void Stop();
  void Install();
  void EnableCreateThreadNotify();
  void DisableCreateThreadNotify();
  void Update();

private:
  SC_HANDLE _GetServiceHandle();
  void _SendIOCTLCode(DWORD __ioctlCode);
  static BOOL _WaitForServiceState(SC_HANDLE __hService, DWORD __serviceState);
  static BOOL _GetServiceState(SC_HANDLE __hService, DWORD* __currState);
  static void _PrintError(std::string&& __funcName, DWORD __errCode);

};

#endif // !_DRIVER_CONTROL_H
