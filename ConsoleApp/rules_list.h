#ifndef _RULES_LIST_H
#define _RULES_LIST_H


#include <iostream>
#include <list>
#include <windows.h>

#include "../registry_filter.h"

#define OUTPUT_LEVEL '\t'
#define SUBKEY_NAME L"SOFTWARE\\"##_DRIVER_NAME

class MyRuleList {
public:
  void Clear();
  void LoadRules();
  void PrintRules();
  void DeleteRule(size_t __idx);
  void AddRule(std::wstring& __procName, int32_t __level);
private:
  void _RenewKeyValue();
  void _PrintError(std::string&& __funcName, DWORD __errCode);

  struct MyRuleNode {
    std::wstring _procName;
    int32_t _level = 0;
  };

  LPCWSTR SUBKEY = SUBKEY_NAME;
  LPCWSTR KEY_NAME = L"rule";
  std::list<MyRuleNode> _rules;
};

#endif // !_RULES_LIST_H
