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
#define LOG_FILENAME  "log.txt"

#define LOG_ERROR(MSG, ERROR_CODE) \
  LOG_STATUS(MSG " failed with 0x%08x", ERROR_CODE)
#define LOG_STATUS(FMT, ...) \
  _LogMsg(FMT "\n", __VA_ARGS__)
#define LOG_RESULT(MSG, RESULT) \
  _LogResult(MSG, RESULT)

class Controller {
public:
  Controller();
  Controller(bool __verbose);
  VOID Run(VOID);
private:
  VOID _Usage(VOID);
  VOID _LogMsg(const char* __fmt, ...);
  VOID _LogResult(const char* __fmt, DriverControl::Result __res);

  MyRuleList _rules;
  DriverControl _drv;
  bool _verbose;
};

#endif // !_CONTROLLER_H
