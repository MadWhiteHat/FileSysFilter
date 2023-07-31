#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#include <iostream>
#include <windows.h>

#include "rules_list.h"
#include "driver_control.h"

#define OUTPUT_LEVEL '\t'

class Controller {
public:
  VOID Run(VOID);
private:
  inline VOID _Usage(VOID);

  MyRuleList _list;
  DriverControl _drv;
};

#endif // !_CONTROLLER_H
