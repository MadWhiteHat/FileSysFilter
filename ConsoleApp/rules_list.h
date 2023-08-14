#ifndef _RULES_LIST_H
#define _RULES_LIST_H

#include <iostream>
#include <list>
#include <windows.h>

#include "../file_sys_filter.h"

class MyRuleList {
public:
  VOID
  Print();

  DWORD
  FindByIdx(
    size_t __idx,
    std::wstring& __fileName,
    std::wstring& __procName
  );

  DWORD
  Delete(
    std::wstring& __fileName,
    std::wstring& __procName
  );

  DWORD Add(
    std::wstring& __fileName,
    std::wstring& __procName,
    DWORD __accessMask
  );

  VOID
  Save(LPCSTR __fileName, PRESULT __res);

  VOID
  Clear();

private:
  
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
