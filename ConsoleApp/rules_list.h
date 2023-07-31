#ifndef _RULES_LIST_H
#define _RULES_LIST_H

#include <iostream>
#include <list>
#include <windows.h>

#include "../file_sys_filter.h"

#define CONF_FILE_PATH L"rules.xml"

#define RULES_ERROR_STATE (1L << 31)
#define RULES_ERROR_SUCCESS 0L
#define RULES_ERROR_OPEN_RULES_FILE \
  1L | RULES_ERROR_STATE
#define RULES_ERROR_READ_RULES \
  2L | RULES_ERROR_STATE
#define RULES_ERROR_WRITE_RULES \
  3L | RULES_ERROR_STATE
#define RULES_ERROR_PARSE_RULES \
  4L | RULES_ERROR_STATE
#define RULES_ERROR_INVALID_PERMISSIONS \
  5L | RULES_ERROR_STATE
#define RULES_ERROR_LIST_EMPTY \
  6L | RULES_ERROR_STATE
#define RULES_ERROR_LIST_INVALID_RANGE \
  7L | RULES_ERROR_STATE

#define RULES_SUCCESS(STATUS) \
  !(STATUS & RULES_ERROR_STATE)


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
