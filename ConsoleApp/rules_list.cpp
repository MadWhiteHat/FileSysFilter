#include "rules_list.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

VOID
MyRuleList::
Clear() { _aclList.clear(); }

VOID
MyRuleList::
Print() {
  DWORD __cnt = 0;
  std::wstring __accessMaskStr(2, 0);

  for (const auto& __acl : _aclList) {
    for (const auto& __ace : __acl._aceList) {

      std::wcout << ++__cnt << ' ';
      std::wcout << L"File: " << __acl._fileName << ' ';
      std::wcout << L"Process: " << __ace._procName << ' ';
      std::wcout << L"Permissions: ";
      __accessMaskStr[0] =
        (__ace._accessmask & MASK_ALLOW_READ) ? 'r' : '-';
      __accessMaskStr[1]
        = (__ace._accessmask & MASK_ALLOW_WRITE) ? 'w' : '-';
      std::wcout << __accessMaskStr << '\n';
    }
  }
  std::cout << "Total: " << __cnt << " rule(s)\n";
}

DWORD
MyRuleList::
Delete(
  std::wstring& __fileName,
  std::wstring& __procName
) {
  size_t __count = 0;
  size_t __curr = 0;

  if (_aclList.empty()) { return FSFLT_RULES_ERROR_LIST_EMPTY; }

  // After removing element all end iterators are invalidated

  auto __begAcl = _aclList.begin();

  for (; __begAcl != _aclList.end(); ++__begAcl) {

    if (__begAcl->_fileName == __fileName) {

      auto __begAce = __begAcl->_aceList.begin();

      for (; __begAce != __begAcl->_aceList.end(); ++__begAce) {
        if (__begAce->_procName == __procName) {
          __begAcl->_aceList.erase(__begAce);
          if (__begAcl->_aceList.empty()) {
            _aclList.erase(__begAcl);
          }
        }
      }
    }
  }
  if (__begAcl == _aclList.end()) {
    return FSFLT_RULES_INVALID_PARAMETERS;
  }

  return FSFLT_ERROR_SUCCESS;
}

DWORD
MyRuleList::
Add(
  std::wstring& __fileName,
  std::wstring& __procName,
  DWORD __accessMask
) {

  if (__fileName.empty() || __procName.empty()) {
    return FSFLT_RULES_ERROR_INVALID_PERMISSIONS;
  }

  auto __beg = _aclList.begin();
  auto __end = _aclList.end();
  for (; __beg != __end; ++__beg) {
    if (!(__beg->_fileName.compare(__fileName))) { break; }
  }

  // Acl doesn't exists

  if (__beg == __end) {
    MyAcl __acl{__fileName};
    MyAce __ace{__procName, __accessMask};
    __acl._aceList.push_back(std::move(__ace));
    _aclList.push_back(std::move(__acl));
  }

  // Otherwise
  else {
    MyAce __ace{__procName, __accessMask};
    __beg->_aceList.push_back(std::move(__ace));
  }

  return FSFLT_ERROR_SUCCESS;
}

DWORD
MyRuleList::
FindByIdx(
  size_t __idx,
  std::wstring& __fileName,
  std::wstring& __procName
) {
  size_t __count = 0;
  size_t __curr = 0;

  if (_aclList.empty()) { return FSFLT_RULES_ERROR_LIST_EMPTY; }

  for (const auto& __acl : _aclList) { __count += __acl._aceList.size(); }
  if (__idx > __count - 1) { return FSFLT_RULES_ERROR_LIST_INVALID_RANGE; }

  auto __begAcl = _aclList.begin();
  auto __endAcl = _aclList.end();

  for (; __begAcl != __endAcl; ++__begAcl) {

    auto __begAce = __begAcl->_aceList.begin();
    auto __endAce = __begAcl->_aceList.end();
    for (; __begAce != __endAce; ++__begAce) {
      if (__curr == __idx) { break; }
      ++__curr;
    }

    if (__begAce == __endAce) { return FSFLT_RULES_ERROR_LIST_INVALID_RANGE; }

    __fileName = __begAcl->_fileName;
    __procName = __begAce->_procName;
  }

  if (__begAcl == __endAcl) { return FSFLT_RULES_ERROR_LIST_INVALID_RANGE; }

  return FSFLT_ERROR_SUCCESS;
}

VOID
MyRuleList::
Save(LPCSTR __fileName, PRESULT __res) {
  std::wstring __rules(L"<rules>\n");
  std::wofstream __fdOut;

  {
    std::wstring __entryStart(L"\t<entry>\n");
    std::wstring __entryEnd(L"\t</entry>\n");
    std::wstring __fileStart(L"\t\t<file>");
    std::wstring __fileEnd(L"</file>\n");
    std::wstring __procStart(L"\t\t<process>");
    std::wstring __procEnd(L"</process>\n");
    std::wstring __accessMaskStart(L"\t\t<permissions>");
    std::wstring __accessMaskEnd(L"</permissions>\n");
    std::wstring __accessMaskStr(2, 0);
    size_t __curr = 0;

    for (const auto& __acl : _aclList) {
      for (const auto& __ace : __acl._aceList) {
        __rules.append(__entryStart);
        __rules.append(__fileStart);
        __rules.append(__acl._fileName);
        __rules.append(__fileEnd);
        __rules.append(__procStart);
        __rules.append(__ace._procName);
        __rules.append(__procEnd);
        __rules.append(__accessMaskStart);
        __accessMaskStr[0] = (__ace._accessmask & MASK_ALLOW_READ) ? 'r' : '-';
        __accessMaskStr[1] = (__ace._accessmask & MASK_ALLOW_WRITE) ? 'w' : '-';
        __rules.append(__accessMaskStr);
        __rules.append(__accessMaskEnd);
        __rules.append(__entryEnd);
      }
    }
  }
  __rules.append(L"</rules>\n");

  __fdOut.open(__fileName, std::ios_base::out);
  if (!__fdOut.is_open()) {
    InitResult(
      __res,
      L"wofstream::open",
      ERROR_SUCCESS,
      FSFLT_RULES_ERROR_OPEN_RULES_FILE
    );

    return;
  }

  __fdOut << __rules;
  if (!__fdOut.good()) {
    __fdOut.close();
    InitResult(
      __res,
      L"wofstream::open",
      ERROR_SUCCESS,
      FSFLT_RULES_ERROR_WRITE_RULES
    );

    return;
  }
  __fdOut.close();
  InitResult(
    __res,
    L"wofstream::open",
    ERROR_SUCCESS,
    FSFLT_ERROR_SUCCESS
  );
}
