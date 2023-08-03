#include "rules_list.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

VOID
MyRuleList::
Clear() { _aclList.clear(); }

DWORD 
MyRuleList::
LoadRules() {
  DWORD __res = FSFLT_ERROR_SUCCESS;
  std::wstring __rules;
  std::wstringstream __rulesBuf;
  std::wifstream __fdIn(CONF_FILE_PATH);
  
  if (!__fdIn.is_open()) { return FSFLT_RULES_ERROR_OPEN_RULES_FILE; }

  __rulesBuf << __fdIn.rdbuf();
  __rules = std::move(__rulesBuf.str());

  if (!__fdIn.good() && !__fdIn.eof()) {
    __fdIn.close();
    return FSFLT_RULES_ERROR_READ_RULES;
  }
  __fdIn.close();

  Clear();

  {
    std::wstring __fileName;
    std::wstring __procName;
    std::wstring __accessMaskStr(2, 0);
    std::wstring __entryDummy(
      L"<entry>"
        L"<file></file>"
        L"<process></process>"
        L"<permissions></permissions>"
      L"</entry>"
    );
    std::wstring __fileStart(L"<file>");
    std::wstring __fileEnd(L"</file>");
    std::wstring __procStart(L"<process>");
    std::wstring __procEnd(L"</process>");
    std::wstring __accessMaskStart(L"<permissions>");
    std::wstring __accessMaskEnd(L"</permissions>");
    size_t __shift = 0;
    while (__rules.length() > __entryDummy.length()) {
      __shift = __rules.find(__fileStart);

      if (__shift == std::wstring::npos) { return FSFLT_RULES_ERROR_PARSE_RULES; }

      __shift += __fileStart.length();
      __rules.erase(0, __shift);
      __shift = __rules.find(__fileEnd);

      if (__shift == std::wstring::npos) { return FSFLT_RULES_ERROR_PARSE_RULES; }

      __fileName.assign(__rules, 0, __shift);

      __shift = __rules.find(__procStart);

      if (__shift == std::wstring::npos) { return FSFLT_RULES_ERROR_PARSE_RULES; }

      __shift += __procStart.length();
      __rules.erase(0, __shift);
      __shift = __rules.find(__procEnd);

      if (__shift == std::wstring::npos) { return FSFLT_RULES_ERROR_PARSE_RULES; }

      __procName.assign(__rules, 0, __shift);

      __shift = __rules.find(__accessMaskStart);

      if (__shift == std::wstring::npos) { return FSFLT_RULES_ERROR_PARSE_RULES; }

      __shift += __accessMaskStart.length();
      __rules.assign(__rules.data() + __shift);
      __shift = __rules.find(__accessMaskEnd);

      if (__shift == std::wstring::npos) { return FSFLT_RULES_ERROR_PARSE_RULES; }

      __accessMaskStr.assign(__rules, 0, __shift);

      __res = AddRule(__fileName, __procName, __accessMaskStr);

      if (!FSFLT_SUCCESS(__res)) { break; }
    } 
  }

  return FSFLT_ERROR_SUCCESS;
}

VOID
MyRuleList::
PrintRules() {
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
DeleteRule(size_t __idx) {
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

    // remove
    if (__begAce != __endAce) {
      __begAcl->_aceList.erase(__begAce);
      if (__begAcl->_aceList.empty()) { _aclList.erase(__begAcl); }
      break;
    }
  }
  return FSFLT_ERROR_SUCCESS;
}

DWORD
MyRuleList::
AddRule(std::wstring& __fileName, std::wstring& __procName,
  std::wstring& __accessMaskStr) {

  int32_t __accessMask = 0;

  if (__accessMaskStr.length() != 2) {
    return FSFLT_RULES_ERROR_INVALID_PERMISSIONS;
  }

  if (__fileName.empty() || __procName.empty()) {
    return FSFLT_RULES_ERROR_INVALID_PERMISSIONS;
  }

  // 0x72 = 'r'
  if (__accessMaskStr[0] == 0x72) {
    __accessMask |= MASK_ALLOW_READ;
  }
  // 0x2d = '-'
  else if (__accessMaskStr[0] != 0x2d) {
    return FSFLT_RULES_ERROR_INVALID_PERMISSIONS;
  }
  // 0x77 = 'w'
  if (__accessMaskStr[1] == 0x77) {
    __accessMask |= MASK_ALLOW_WRITE;
  }
  // 0x2d = '-'
  else if (__accessMaskStr[1] != 0x2d) {
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
_RenewRules() {
  std::wstring __rules(L"<rules>");
  std::wofstream __fdOut;

  {
    std::wstring __entryStart(L"<entry>");
    std::wstring __entryEnd(L"</entry>");
    std::wstring __fileStart(L"<file>");
    std::wstring __fileEnd(L"</file>");
    std::wstring __procStart(L"<process>");
    std::wstring __procEnd(L"</process>");
    std::wstring __accessMaskStart(L"<permissions>");
    std::wstring __accessMaskEnd(L"</permissions>");
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
  __rules.append(L"</rules>");

  __fdOut.open(CONF_FILE_PATH, std::ios_base::out | std::ios_base::trunc);
  if (!__fdOut.is_open()) { return FSFLT_RULES_ERROR_OPEN_RULES_FILE; }

  __fdOut << __rules;
  if (!__fdOut.good()) {
    __fdOut.close();
    return FSFLT_RULES_ERROR_WRITE_RULES;
  }
  __fdOut.close();
  return FSFLT_ERROR_SUCCESS;
}