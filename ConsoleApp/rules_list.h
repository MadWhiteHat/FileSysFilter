#ifndef _RULES_LIST_H
#define _RULES_LIST_H

#include <iostream>
#include <list>
#include <windows.h>

#include "../file_sys_filter.h"

#define CONF_FILE_PATH L"C:\\Windows\\" _CONF_FILE_PATH

class MyRuleList {
public:
  void Clear();
  void LoadRules();
  void PrintRules();
  void DeleteRule(size_t __idx);
  void AddRule(std::wstring& __fileName, std::wstring& __procName,
    std::wstring& __accessMaskStr);
private:
  void _RenewRules();
  static void _PrintError(std::string&& __funcName, DWORD __errCode);
  
  struct MyAce {
    std::wstring _procName;
    int32_t _accessmask = 0;
  };

  struct MyAcl {
    std::wstring _fileName;
    std::list<MyAce> _aceList;
  };

  std::list<MyAcl> _aclList;
};

#endif // !_RULES_LIST_H
