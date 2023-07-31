#include <iostream>
#include <string>
#include <sstream>
#include <exception>
#include <windows.h>

#include "controller.h"
#include "rules_list.h"
#include "driver_control.h"

VOID
Controller::Run(VOID) {
  _list.LoadRules();
  std::wstring choice;
  while (choice.compare(L"exit")) {
    system("cls");
    _Usage();
    std::cout << "Input command: ";
    std::getline(std::wcin, choice);
    if (!choice.compare(L"print")) { _list.PrintRules(); }
    else if (!choice.compare(0, 4, L"del ")) {
      choice.erase(0, 4);
      try {
        DWORD __res = RULES_ERROR_SUCCESS;
        int32_t __num = std::stoi(choice);

        __res = _list.DeleteRule(--__num);

        if (!RULES_SUCCESS(__res)) {
          
        }
      }
      catch (std::exception& ex) { std::cout << ex.what() << std::endl; }
    }
    else if (!choice.compare(0, 4, L"add ")) {
      choice.erase(0, 4);
      try {
        DWORD __res = RULES_ERROR_SUCCESS;
        std::wstringstream input(std::move(choice));
        std::wstring __fileName;
        std::wstring __procName;
        std::wstring __accessMask;
        input >> __fileName;
        input >> __procName;
        input >> __accessMask;

        __res = _list.AddRule(__fileName, __procName, __accessMask);

        if (RULES_SUCCESS(__res)) {
          std::cout << "Wrong access mask for ";
          std::wcout << L"File: " << __fileName << ' ';
          std::wcout << L"Process: " << __procName << '\n';
        }
      } catch (std::exception& ex) { std::cout << ex.what() << '\n'; }
    }
    else if (!choice.compare(L"start")) { _driver.Start(); }
    else if (!choice.compare(L"stop")) { _driver.Stop(); }
    else if (!choice.compare(L"install")) { _driver.Install(); }
    else if (!choice.compare(L"uninstall")) { _driver.Uninstall(); }
    else if (!choice.compare(L"set")) { _driver.EnableLoadImageNotify(); }
    else if (!choice.compare(L"rm")) { _driver.DisableLoadImageNotify(); }
    else if (!choice.compare(L"update")) { _driver.Update(); }
    else { continue; }
    system("pause");
  }
}

VOID
Controller::_Usage(VOID) {

  std::cout << "Program for setting access rights" << std::endl;
  std::cout << OUTPUT_LEVEL << "Valid parameters for execution:" << std::endl;
  std::cout << OUTPUT_LEVEL << "print - display current permissions\n";
  std::cout << OUTPUT_LEVEL << "del - delete a rule with a number N\n";
  std::cout << OUTPUT_LEVEL << "add - add a new rule\n";
  std::cout << OUTPUT_LEVEL << "install - install FileSysDriver\n";
  std::cout << OUTPUT_LEVEL << "uninstall - uninstall FileSysDriver\n";
  std::cout << OUTPUT_LEVEL << "start - load FileSysDriver\n";
  std::cout << OUTPUT_LEVEL << "stop - unload FileSysDriver\n";
  std::cout << OUTPUT_LEVEL << "set - set notifier PsSetLoadImageNotifyRoutine\n";
  std::cout << OUTPUT_LEVEL << "rm - remove notifier PsSetLoadImageNotifyRoutine\n";
  std::cout << OUTPUT_LEVEL << "update - update driver rules\n";
  std::cout << OUTPUT_LEVEL << "exit - close program\n";

}
