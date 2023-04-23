#include <iostream>
#include <list>
#include <string>
#include <windows.h>
#include <sstream>
#include <exception>

#include "rules_list.h"
#include "driver_control.h"

#define OUTPUT_LEVEL '\t'

int main() {
  
  MyRuleList __list;
  DriverControl __driver;
  __list.LoadRules();

  std::wstring choice;
  std::cout << "Program for setting access rights" << std::endl;
  std::cout << OUTPUT_LEVEL << "Valid parameters for execution:" << std::endl;
  std::cout << OUTPUT_LEVEL << "print - display current permissions;\n";
  std::cout << OUTPUT_LEVEL << "del   - delete a rule with a number N;\n";
  std::cout << OUTPUT_LEVEL << "add   - add a new rule;\n";
  std::cout << OUTPUT_LEVEL << "start - load RegFltDriver;\n";
  std::cout << OUTPUT_LEVEL << "stop  - unload RegFltDriver;\n";
  std::cout << OUTPUT_LEVEL << "set   - set notifier PsSetCreateThreadNotifyRoutine;\n";
  std::cout << OUTPUT_LEVEL << "rm   - remove notifier PsSetCreateThreadNotifyRoutine;\n";
  std::cout << OUTPUT_LEVEL << "update - update driver rules" << std::endl;
  std::cout << OUTPUT_LEVEL << "exit  - close program\n";
  while (choice.compare(L"exit")) {
    std::cout << "Input command: ";
    std::getline(std::wcin, choice);
    if (!choice.compare(L"print")) { __list.PrintRules(); }
    else if (!choice.compare(0, 4, L"del ")) {
      choice.erase(0, 4);
      try {
        int32_t __num = std::stoi(choice);
        __list.DeleteRule(--__num);
      }
      catch (std::exception& ex) { std::cout << ex.what() << std::endl; }
    }
    else if (!choice.compare(0, 4, L"add ")) {
      choice.erase(0, 4);
      try {
        std::wstringstream input(std::move(choice));
        std::wstring __procName;
        std::wstring __levelStr;
        input >> __procName;
        input >> __levelStr;
        int32_t __level = std::stoi(__levelStr);
        __list.AddRule(__procName, __level);
      }
      catch (std::exception& ex) { std::cout << ex.what() << std::endl; }
    }
    else if (!choice.compare(L"start")) { __driver.Start(); }
    else if (!choice.compare(L"stop")) { __driver.Stop(); }
    else if (!choice.compare(L"set")) { __driver.EnableCreateThreadNotify(); }
    else if (!choice.compare(L"rm")) { __driver.DisableCreateThreadNotify(); }
    else if (!choice.compare(L"update")) { __driver.Update(); }
  }
}
