#include <iostream>
#include <list>
#include <string>
#include <windows.h>
#include <sstream>
#include <exception>

#include "rules_list.h"
#include "driver_control.h"

#define OUTPUT_LEVEL '\t'

inline void usage() {
  std::cout << "Program for setting access rights" << std::endl;
  std::cout << OUTPUT_LEVEL << "Valid parameters for execution:" << std::endl;
  std::cout << OUTPUT_LEVEL << "print - display current permissions\n";
  std::cout << OUTPUT_LEVEL << "del - delete a rule with a number N\n";
  std::cout << OUTPUT_LEVEL << "add - add a new rule\n";
  std::cout << OUTPUT_LEVEL << "install - install FileSysDriver\n";
  std::cout << OUTPUT_LEVEL << "start - load FileSysDriver\n";
  std::cout << OUTPUT_LEVEL << "stop - unload FileSysDriver\n";
  std::cout << OUTPUT_LEVEL << "set - set notifier PsSetCreateThreadNotifyRoutine\n";
  std::cout << OUTPUT_LEVEL << "rm - remove notifier PsSetCreateThreadNotifyRoutine\n";
  std::cout << OUTPUT_LEVEL << "update - update driver rules\n";
  std::cout << OUTPUT_LEVEL << "exit - close program\n";
}

int main() {
  
  MyRuleList __list;
  DriverControl __driver;
  __list.LoadRules();

  std::wstring choice;
  while (choice.compare(L"exit")) {
    system("cls");
    usage();
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
        std::wstring __fileName;
        std::wstring __procName;
        std::wstring __accessMask;
        input >> __fileName;
        input >> __procName;
        input >> __accessMask;
        __list.AddRule(__fileName, __procName, __accessMask);
      }
      catch (std::exception& ex) { std::cout << ex.what() << std::endl; }
    }
    else if (!choice.compare(L"start")) { __driver.Start(); }
    else if (!choice.compare(L"stop")) { __driver.Stop(); }
    else if (!choice.compare(L"install")) { __driver.Install(); }
    else if (!choice.compare(L"set")) { __driver.EnableCreateThreadNotify(); }
    else if (!choice.compare(L"rm")) { __driver.DisableCreateThreadNotify(); }
    else if (!choice.compare(L"update")) { __driver.Update(); }
    else { continue; }
    system("pause");
  }
}
