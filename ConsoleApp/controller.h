#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#include <iostream>
#include <windows.h>

#ifndef _CONSOLE_APP
#define _CONSOLE_APP
#endif // !_CONSOLE_APP

#include "../file_sys_filter.h"
#include "rules_list.h"
#include "driver_control.h"

#define OUTPUT_LEVEL '\t'

#define LOG_BUFFER_SIZE 1L << 10
#define LOG_FILEPATH  "log.txt"
#define CONF_FILEPATH "rules.xml"

#define LOG_ERROR(MSG, ERROR_CODE) \
  LOG_STATUS(MSG " failed with 0x%08x", ERROR_CODE)
#define LOG_STATUS(FMT, ...) \
  _LogMsg(FMT, __VA_ARGS__)
#define LOG_RESULT(MSG, RESULT) \
  _LogResult(MSG, RESULT)

class Controller {
public:

  Controller();

  Controller(bool __verbose);

  VOID
  Run();

  VOID
  DeleteRule(DWORD __idx, PRESULT __res);

  VOID
  AddRule(
    std::wstring& __fileName,
    std::wstring& __procName,
    std::wstring& __accessMaskStr,
    PRESULT __res
  );

  VOID
  Load(PRESULT __res);

private:

  VOID
  _Usage();

  VOID
  _LogMsg(const char* __fmt, ...);

  VOID
  _LogResult(const char* __fmt, PRESULT __res);

  DWORD
  _ConvertMask(std::wstring& __accessMaskStr, LPDWORD __accessMask);

  DriverControl _drv;
  MyRuleList _rules;
  bool _verbose;
};

#endif // !_CONTROLLER_H
