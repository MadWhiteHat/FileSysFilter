#ifndef _RULES_LIST_H
#define _RULES_LIST_H

#include <iostream>
#include <list>
#include <windows.h>

#include "../file_sys_filter.h"

#define CONF_FILE_PATH L"rules.xml"

class MyRuleList {
public:
  VOID Clear();
  VOID PrintRules();
  DWORD LoadRules();
  DWORD DeleteRule(size_t __idx);
  DWORD AddRule(std::wstring& __fileName, std::wstring& __procName,
    std::wstring& __accessMaskStr);
private:
  DWORD _RenewRules();
  
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
