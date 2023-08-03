#include "controller.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <exception>
#include <cstdarg>
#include <windows.h>

Controller::Controller() : Controller(true) {}
Controller::Controller(bool __verbose) : _verbose(__verbose) {}

VOID
Controller::Run(VOID) {
  DriverControl::Result __res;
  DWORD __dwRes;

  __dwRes = _rules.LoadRules();

  if (!FSFLT_SUCCESS(__dwRes)) { LOG_ERROR("Loading rules", __res); }
  else { LOG_STATUS("Rules were loaded successfully"); }

  std::wstring choice;
  while (choice.compare(L"exit")) {
    system("cls");
    _Usage();
    std::cout << "Input command: ";
    std::getline(std::wcin, choice);
    if (!choice.compare(L"print")) { _rules.PrintRules(); }
    else if (!choice.compare(0, 4, L"del ")) {
      choice.erase(0, 4);
      try {
        int32_t __num = std::stoi(choice);

        __dwRes = _rules.DeleteRule(--__num);

        if (!FSFLT_SUCCESS(__dwRes)) { LOG_ERROR("Deleting rule", __res); }
        else { LOG_STATUS("Rule #%d was deleted", __num + 1); }
      }
      catch (std::exception& __ex) { std::cout << __ex.what() << '\n'; }
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

        __dwRes = _rules.AddRule(__fileName, __procName, __accessMask);

        if (!FSFLT_SUCCESS(__dwRes)) {
          LOG_ERROR("Adding rule", __res);
        } else {
          LOG_STATUS(
            "Rule: Process: %ws File: %ws Permissions: %ws was added successfully",
            __fileName.data(), __procName.data(), __accessMask.data()
          );
        }
      }
      catch (std::exception& ex) { std::cout << ex.what() << '\n'; }
    }
    else if (!choice.compare(L"start")) {
      __res = _drv.Start();

      if (!FSFLT_SUCCESS(__res._internalErrCode)) {
        if (__res._winErrCode == ERROR_SERVICE_ALREADY_RUNNING) {
          LOG_STATUS("Service already running");
        } else { LOG_RESULT("Starting service", __res); }
      } else { LOG_STATUS("Service has been started successfully"); }
    }
    else if (!choice.compare(L"stop")) {
      __res = _drv.Stop();

      if (!FSFLT_SUCCESS(__res._internalErrCode)) {
        if (__res._winErrCode == ERROR_SERVICE_NOT_ACTIVE) {
          LOG_STATUS("Service already stopped");
        } else { LOG_RESULT("Stopping service", __res); }
      } else { LOG_STATUS("Service has been stopped successfully"); }
    }
    else if (!choice.compare(L"install")) {
      __res = _drv.Install();
      
      if (!FSFLT_SUCCESS(__res._internalErrCode)) {
        if (__res._internalErrCode == FSFLT_DRIVER_CONTROL_ERROR_SERVICE_ALREADY_INSTALLED) {
          LOG_STATUS("Servce has already been installed");
        } else { LOG_RESULT("Installing service", __res); }
      } else { LOG_STATUS("Service has been installed successfully"); }
    }
    else if (!choice.compare(L"uninstall")) {
      __res = _drv.Uninstall();
      
      if (!FSFLT_SUCCESS(__res._internalErrCode)) {
        if (__res._internalErrCode == FSFLT_DRIVER_CONTROL_ERROR_SERVICE_ALREADY_UNINSTALLED) {
          LOG_STATUS("Servce has already been uninstalled");
        } else { LOG_RESULT("Uninstalling service", __res); }
      } else { LOG_STATUS("Service has been uninstalled successfully"); }
    }
    else if (!choice.compare(L"set")) {
      __res = _drv.EnableLoadImageNotify();
      
      if (!FSFLT_SUCCESS(__res._internalErrCode)) {
        LOG_RESULT("Setting LoadImage notifier", __res);
      } else { LOG_STATUS("LoadImage notifier has been successfully set"); }
    }
    else if (!choice.compare(L"rm")) {
      __res = _drv.DisableLoadImageNotify();
      
      if (!FSFLT_SUCCESS(__res._internalErrCode)) {
        LOG_RESULT("Removing LoadImage notifier", __res);
      } else {
        LOG_STATUS("LoadImage notifier has been successfully removed");
      }
    }
    else if (!choice.compare(L"update")) {
      _drv.Update();
    }
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
  std::cout << OUTPUT_LEVEL << "update - update drv rules\n";
  std::cout << OUTPUT_LEVEL << "exit - close program\n";

}

VOID
Controller::_LogMsg(const char* __fmt, ...) {
  std::ofstream __log;
  char __logBuf[LOG_BUFFER_SIZE];
  std::va_list __args;

  va_start(__args, __fmt);
  std::vsnprintf(__logBuf, LOG_BUFFER_SIZE, __fmt, __args);
  va_end(__args);

  if (_verbose) { std::cout << __logBuf; }
  __log.open(LOG_FILENAME, std::ios_base::out | std::ios_base::app);
  if (__log.is_open()) {
    __log << __logBuf;
    __log.close();
  }
}

VOID
Controller::
_LogResult(const char* __msg, DriverControl::Result __res) {
  _LogMsg(
    "%s failed with 0x%08x %s returns: 0x%08x\n",
    __msg, __res._internalErrCode, __res._winFuncName.data(), __res._winErrCode
  );
}