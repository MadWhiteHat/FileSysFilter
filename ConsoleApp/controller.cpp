#include "controller.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <exception>
#include <vector>
#include <cstdarg>
#include <windows.h>

void
InitResult(
  PRESULT __res,
  LPCWSTR __funcName,
  DWORD __winErrCode,
  DWORD __internalErrCode
) {
  size_t __funcNameLen = 0;

  if (__res == NULL) { return; }

  memset(__res->_funcName, 0x00, FUNC_BUFFER_SIZE);

  if (__funcName != NULL) {
    __funcNameLen = wcslen(__funcName);

    std::swprintf(
      __res->_funcName,
      FUNC_BUFFER_SIZE,
      __funcName
    );
  }

  __res->_winErrCode = __winErrCode;
  __res->_internalErrCode = __internalErrCode;
}

Controller::
Controller() : Controller(true) {}

Controller::
Controller(bool __verbose) : _verbose(__verbose) {}

VOID
Controller::
Run() {
  RESULT __res;

  std::wstring choice;
  while (choice.compare(L"exit")) {
    system("cls");
    _Usage();
    std::cout << "Input command: ";
    std::getline(std::wcin, choice);
    if (!choice.compare(L"print")) {
      _rules.Print();
      _drv.PrintRules();
    }
    else if (!choice.compare(0, 4, L"del ")) {
      choice.erase(0, 4);
      try {
        DWORD __idx = std::stoi(choice);

        DeleteRule(__idx, &__res);

        if (!FSFLT_SUCCESS(__res._internalErrCode)) {
          LOG_RESULT("Deleting rule", &__res);
        } else {
          LOG_STATUS("Rule #%d was deleted", __idx);
        }

      } catch (std::exception& __ex) { std::cout << __ex.what() << '\n'; }
    }
    else if (!choice.compare(0, 4, L"add ")) {
      choice.erase(0, 4);
      try {
        std::wstringstream __input(std::move(choice));
        std::wstring __fileName;
        std::wstring __procName;
        std::wstring __accessMaskStr;

        __input >> __fileName;
        __input >> __procName;
        __input >> __accessMaskStr;

        AddRule(__fileName, __procName, __accessMaskStr, &__res);

        if (!FSFLT_SUCCESS(__res._internalErrCode)) {
          LOG_RESULT("Adding rule", &__res);
        } else {
          LOG_STATUS(
            "Rule: File: %ws Process: %ws Permissions: %ws was added succesfully",
            __fileName.data(), __procName.data(), __accessMaskStr.data()
          );
        }

      } catch (std::exception& ex) { std::cout << ex.what() << '\n'; }
    }
    else if (!choice.compare(L"load")) {
      Load(&__res);

      if (!FSFLT_SUCCESS(__res._internalErrCode)) {
        LOG_RESULT("Loading rules", &__res);
      } else { LOG_STATUS("Rules has been loaded successfully"); }
    }
    else if (!choice.compare(L"save")) {
      _rules.Save(CONF_FILEPATH, &__res);

      if (!FSFLT_SUCCESS(__res._internalErrCode)) {
        LOG_RESULT("Saving rules", &__res);
      } else { LOG_STATUS("Rules has been saved successfully"); }
    }
    else if (!choice.compare(L"start")) {
      _drv.Start(&__res);

      if (!FSFLT_SUCCESS(__res._internalErrCode)) {
        if (__res._winErrCode == ERROR_SERVICE_ALREADY_RUNNING) {
          LOG_STATUS("Service already running");
        } else { LOG_RESULT("Starting service", &__res); }
      } else { LOG_STATUS("Service has been started successfully"); }
    }
    else if (!choice.compare(L"stop")) {

      _drv.Stop(&__res);

      if (!FSFLT_SUCCESS(__res._internalErrCode)) {
        if (__res._winErrCode == ERROR_SERVICE_NOT_ACTIVE) {
          LOG_STATUS("Service already stopped");
        } else { LOG_RESULT("Stopping service", &__res); }
      } else { LOG_STATUS("Service has been stopped successfully"); }
    }
    else if (!choice.compare(L"install")) {
      _drv.Install(&__res);
      
      if (!FSFLT_SUCCESS(__res._internalErrCode)) {
        if (__res._internalErrCode
          == FSFLT_DRIVER_CONTROL_ERROR_SERVICE_ALREADY_INSTALLED) {
          LOG_STATUS("Servce has already been installed");
        } else { LOG_RESULT("Installing service", &__res); }
      } else { LOG_STATUS("Service has been installed successfully"); }
    }
    else if (!choice.compare(L"uninstall")) {
      _drv.Uninstall(&__res);
      
      if (!FSFLT_SUCCESS(__res._internalErrCode)) {
        if (__res._internalErrCode
          == FSFLT_DRIVER_CONTROL_ERROR_SERVICE_ALREADY_UNINSTALLED) {
          LOG_STATUS("Servce has already been uninstalled");
        } else { LOG_RESULT("Uninstalling service", &__res); }
      } else { LOG_STATUS("Service has been uninstalled successfully"); }
    }
    else if (!choice.compare(L"set")) {
      _drv.EnableLoadImageNotify(&__res);
      
      if (!FSFLT_SUCCESS(__res._internalErrCode)) {
        LOG_RESULT("Setting LoadImage notifier", &__res);
      } else { LOG_STATUS("LoadImage notifier has been successfully set"); }
    }
    else if (!choice.compare(L"rm")) {
      _drv.DisableLoadImageNotify(&__res);
      
      if (!FSFLT_SUCCESS(__res._internalErrCode)) {
        LOG_RESULT("Removing LoadImage notifier", &__res);
      } else { LOG_STATUS("LoadImage notifier has been successfully removed"); }
    }
    else { continue; }
    system("pause");
  }
}

VOID
Controller::
DeleteRule(DWORD __idx, PRESULT __res) {
  std::wstring __fileName;
  std::wstring __procName;
  DWORD __dwRes;

  __dwRes = _rules.FindByIdx(--__idx, __fileName, __procName);
  if (!FSFLT_SUCCESS(__dwRes)) {
    InitResult(
      __res,
      L"MyRulesList::FindByIdx",
      ERROR_SUCCESS,
      __dwRes
    );

    return;
  }

  _drv.DelRule(__fileName, __procName, __res);
  if (!FSFLT_SUCCESS(__res->_internalErrCode)) { return; }

  __dwRes = _rules.Delete(__fileName, __procName);
  if (!FSFLT_SUCCESS(__dwRes)) {
    InitResult(
      __res,
      L"MyRulesList::Delete",
      ERROR_SUCCESS,
      __dwRes
    );

    return;
  }

  InitResult(
    __res,
    L"Controller::DeleteRule",
    ERROR_SUCCESS,
    FSFLT_ERROR_SUCCESS
  );
}

VOID
Controller::
AddRule(
  std::wstring& __fileName,
  std::wstring& __procName,
  std::wstring& __accessMaskStr,
  PRESULT __res
) {
  DWORD __dwRes;
  DWORD __accessMask;

  __dwRes = _ConvertMask(__accessMaskStr, &__accessMask);
  if (!FSFLT_SUCCESS(__dwRes)) {
    InitResult(
      __res,
      L"Controller::_ConvertMask",
      ERROR_INVALID_PARAMETER,
      __dwRes
    );

    return;
  }

  _drv.AddRule(__fileName, __procName, __accessMask, __res);
  if (!FSFLT_SUCCESS(__res->_internalErrCode)) { return; }

  __dwRes = _rules.Add(__fileName, __procName, __accessMask);

  if (!FSFLT_SUCCESS(__dwRes)) {
    InitResult(
      __res,
      L"MyRulesList::Add",
      ERROR_SUCCESS,
      __dwRes
    );

    return;
  }

  InitResult(
    __res,
    L"Controller::AddRule",
    ERROR_SUCCESS,
    FSFLT_ERROR_SUCCESS
  );
}

VOID
Controller::
Load(PRESULT __res) {
  std::vector<std::wstring> __rules;
  std::wstringstream __rulesBuf;
  std::wifstream __fdIn(CONF_FILEPATH);
  
  if (!__fdIn.is_open()) {
    InitResult(
      __res,
      L"wifstream::open",
      ERROR_SUCCESS,
      FSFLT_CONTROLLER_ERROR_OPEN_RULES_FILE
    );

    return;
  }

  {
    std::wstring __tmpLine;
    while(std::getline(__fdIn, __tmpLine)) {

      if (!__fdIn.good() && !__fdIn.eof()) {
        __fdIn.close();
        InitResult(
          __res,
          L"wifstream::getline",
          ERROR_SUCCESS,
          FSFLT_CONTROLLER_ERROR_READ_RULES
        );

        __fdIn.close();
        return;
      }

      __rules.push_back(std::move(__tmpLine));
    }
  }

  __fdIn.close();


  _drv.ClearRules(__res);
  if (!FSFLT_SUCCESS(__res->_internalErrCode)) { return; }

  _rules.Clear();

  {
    std::wstring __fileName;
    std::wstring __procName;
    std::wstring __accessMaskStr(2, 0);
    std::wstring::size_type __shift = 0;

    for (auto& __rule : __rules) {
      __shift = __rule.find(';');
      if (__shift == std::wstring::npos) {
        InitResult(
          __res,
          L"wstring::find",
          ERROR_SUCCESS,
          FSFLT_CONTROLLER_ERROR_PARSE_RULES
        );

        return;
      }

      __fileName.assign(__rule, 0, __shift);
      __rule.erase(0, __shift + 1);

      __shift = __rule.find(';');
      if (__shift == std::wstring::npos) {
        InitResult(
          __res,
          L"wstring::find",
          ERROR_SUCCESS,
          FSFLT_CONTROLLER_ERROR_PARSE_RULES
        );

        return;
      }
      
      __procName.assign(__rule, 0, __shift);
      __rule.erase(0, __shift + 1);

      __accessMaskStr.assign(__rule, 0, 2);

      AddRule(__fileName, __procName, __accessMaskStr, __res);
      if (!FSFLT_SUCCESS(__res->_internalErrCode)) { return; }
    }
  }

  InitResult(
    __res,
    L"Controller::Load",
    ERROR_SUCCESS,
    FSFLT_ERROR_SUCCESS
  );
}

VOID
Controller::_Usage(VOID) {

  std::cout << "Program for setting access rights" << std::endl;
  std::cout << OUTPUT_LEVEL << "Valid parameters for execution:" << std::endl;
  std::cout << OUTPUT_LEVEL << "print - display current permissions\n";
  std::cout << OUTPUT_LEVEL << "del {NUMBER} - delete a rule with a number N\n";
  std::cout << OUTPUT_LEVEL << "add {FILE_NAME} {PROCESS_NAME} {PERMISSIONS} - add a new rule\n";
  std::cout << OUTPUT_LEVEL << "load - load rules from config file\n";
  std::cout << OUTPUT_LEVEL << "save - save current rules to config file\n";
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
  __log.open(LOG_FILEPATH, std::ios_base::out | std::ios_base::app);
  if (__log.is_open()) {
    __log << __logBuf;
    __log.close();
  }
}

VOID
Controller::
_LogResult(const char* __msg, PRESULT __res) {
  _LogMsg(
    "%s failed with 0x%08x %ws returns: 0x%08x\n",
    __msg, __res->_internalErrCode, __res->_funcName, __res->_winErrCode
  );
}

DWORD
Controller::
_ConvertMask(std::wstring& __accessMaskStr, LPDWORD __accessMask) {
  
  if (__accessMaskStr.length() != 2) {
    return FSFLT_RULES_ERROR_INVALID_PERMISSIONS;
  }

  *__accessMask = 0;

  // 0x72 = 'r'
  if (__accessMaskStr[0] == 0x72) {
    *__accessMask |= MASK_ALLOW_READ;
  }
  // 0x2d = '-'
  else if (__accessMaskStr[0] != 0x2d) {
    return FSFLT_RULES_ERROR_INVALID_PERMISSIONS;
  }
  // 0x77 = 'w'
  if (__accessMaskStr[1] == 0x77) {
    *__accessMask |= MASK_ALLOW_WRITE;
  }
  // 0x2d = '-'
  else if (__accessMaskStr[1] != 0x2d) {
    return FSFLT_RULES_ERROR_INVALID_PERMISSIONS;
  }

  return FSFLT_ERROR_SUCCESS;
}
