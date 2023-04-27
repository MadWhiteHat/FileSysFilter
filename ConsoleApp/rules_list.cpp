#include "rules_list.h"

#include <iostream>
#include <string>
#include <fstream>

void
MyRuleList::
Clear() { _aclList.clear(); }

void
MyRuleList::
LoadRules() {
  std::wstring __jsonRules;
  std::wifstream __fdIn(CONF_FILE_PATH);
  
  if (!__fdIn.is_open()) {
    _PrintError("Open File", 0);
    return;
  }
  __fdIn >> __jsonRules;
  if (!__fdIn.good() && !__fdIn.eof()) {
    __fdIn.close();
    _PrintError("Reading rules", 0);
    return;
  }
  __fdIn.close();

  this->Clear();
  {
    std::wstring __fileName;
    std::wstring __procName;
    int32_t __accessMask;
    std::wstring __accessMaskStr(3, 0);
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
    while (__jsonRules.length() > __entryDummy.length()) {
      __shift = __jsonRules.find(__fileStart);
      if (__shift == std::wstring::npos) {
        std::cout << "Error parsing rules file" << std::endl;
        break;
      }
      __shift += __fileStart.length();
      __jsonRules.assign(__jsonRules.data() + __shift);
      __shift = __jsonRules.find(__fileEnd);
      if (__shift == std::wstring::npos) {
        std::cout << "Error parsing rules file" << std::endl;
        break;
      }
      __fileName.assign(__jsonRules, 0, __shift);

      __shift = __jsonRules.find(__procStart);
      if (__shift == std::wstring::npos) {
        std::cout << "Error parsing rules file" << std::endl;
        break;
      }
      __shift += __procStart.length();
      __jsonRules.assign(__jsonRules.data() + __shift);
      __shift = __jsonRules.find(__procEnd);
      if (__shift == std::wstring::npos) {
        std::cout << "Error parsing rules file" << std::endl;
        break;
      }
      __procName.assign(__jsonRules, 0, __shift);

      __shift = __jsonRules.find(__accessMaskStart);
      if (__shift == std::wstring::npos) {
        std::cout << "Error parsing rules file" << std::endl;
        break;
      }
      __shift += __accessMaskStart.length();
      __jsonRules.assign(__jsonRules.data() + __shift);
      __shift = __jsonRules.find(__accessMaskEnd);
      if (__shift == std::wstring::npos) {
        std::cout << "Error parsing rules file" << std::endl;
        break;
      }
      __accessMaskStr.assign(__jsonRules, 0, __shift);
      this->AddRule(__fileName, __procName, __accessMaskStr);
    } 
  }
}

void
MyRuleList::
PrintRules() {
  int32_t __cnt = 0;
  std::wstring __accessMaskStr(2, 0);
  for (const auto& __acl : _aclList) {
    for (const auto& __ace : __acl._aceList) {
      std::wcout << ++__cnt << ' ';
      std::wcout << L"File: " << __acl._fileName << ' ';
      std::wcout << L"Process: " << __ace._procName << ' ';
      std::wcout << L"Permissions: ";
      __accessMaskStr[0] = (__ace._accessmask & ALLOW_READ) ? 'r' : '-';
      __accessMaskStr[1] = (__ace._accessmask & ALLOW_WRITE) ? 'w' : '-';
      std::wcout << __accessMaskStr;
      std::wcout << std::endl;
    }
  }
  std::cout << "Total: " << __cnt << " rule(s)" << std::endl;
}

void
MyRuleList::
DeleteRule(size_t __idx) {
  if (_aclList.empty()) {
    std::cout << "List already empty" << std::endl;
    return;
  }
  size_t __count = 0;
  for (const auto& __acl : _aclList) { __count += __acl._aceList.size(); }
  if (__idx > __count - 1) {
    std::cout << "Index must be in range: 1..." << __count << std::endl;
    return;
  }
  size_t __curr = 0;
  auto __begAcl = _aclList.begin();
  auto __endAcl = _aclList.end();
  for (; __begAcl != __endAcl; ++__begAcl) {
    auto __begAce = __begAcl->_aceList.begin();
    auto __endAce = __begAcl->_aceList.end();
    for (; __begAce != __endAce; ++__begAce) {
      if (__curr == __idx) { break; }
      ++__curr;
    }
    // So remove
    if (__begAce != __endAce) {
      __begAcl->_aceList.erase(__begAce);
      if (__begAcl->_aceList.empty()) { _aclList.erase(__begAcl); }
      break;
    }
  }
  this->_RenewRules();
}

void
MyRuleList::
AddRule(std::wstring& __fileName, std::wstring& __procName,
  std::wstring& __accessMaskStr) {
  int32_t __accessMask = 0;
  if (__accessMaskStr.length() != 2) {
    std::cout << "Wrong access mask for ";
    std::wcout << L"File: " << __fileName << ' ';
    std::wcout << L"Process: " << __procName << std::endl;
    return;
  }

  // 0x72 = 'r'
  if (__accessMaskStr[0] == 0x72) { __accessMask |= ALLOW_READ; }
  // 0x2d = '-'
  else if (__accessMaskStr[0] != 0x2d) {
    std::cout << "Wrong access mask for ";
    std::wcout << L"File: " << __fileName << ' ';
    std::wcout << L"Process: " << __procName << std::endl;
    return;
  }
  // 0x77 = 'w'
  if (__accessMaskStr[1] == 0x77) { __accessMask |= ALLOW_WRITE; }
  // 0x2d = '-'
  else if (__accessMaskStr[1] != 0x2d) {
    std::cout << "Wrong access mask for ";
    std::wcout << L"File: " << __fileName << ' ';
    std::wcout << L"Process: " << __procName << std::endl;
    return;
  }

  auto __beg = _aclList.begin();
  auto __end = _aclList.end();
  for (; __beg != __end; ++__beg) {
    if (!(__beg->_fileName.compare(__fileName))) { break; }
  }
  // Acl doesn't exists
  if (__beg == __end) {
    MyAcl __acl{ std::move(__fileName) };
    MyAce __ace{ std::move(__procName), __accessMask };
    __acl._aceList.push_back(std::move(__ace));
    _aclList.push_back(std::move(__acl));
  }
  // Otherwise
  else {
    MyAce __ace{ std::move(__procName), __accessMask };
    __beg->_aceList.push_back(std::move(__ace));
  }
  this->_RenewRules();
}

void
MyRuleList::
_RenewRules() {
  std::wstring __jsonRules(L"<rules>");
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
        __jsonRules.append(__entryStart);
        __jsonRules.append(__fileStart);
        __jsonRules.append(__acl._fileName);
        __jsonRules.append(__fileEnd);
        __jsonRules.append(__procStart);
        __jsonRules.append(__ace._procName);
        __jsonRules.append(__procEnd);
        __jsonRules.append(__accessMaskStart);
        __accessMaskStr[0] = (__ace._accessmask & ALLOW_READ) ? 'r' : '-';
        __accessMaskStr[1] = (__ace._accessmask & ALLOW_WRITE) ? 'w' : '-';
        __jsonRules.append(__accessMaskStr);
        __jsonRules.append(__accessMaskEnd);
        __jsonRules.append(__entryEnd);
      }
    }
  }
  __jsonRules.append(L"</rules>");

  __fdOut.open(CONF_FILE_PATH, std::ios_base::out | std::ios_base::trunc);
  if (!__fdOut.is_open()) {
    _PrintError("Open file", 0);
    return;
  }

  __fdOut << __jsonRules;
  if (!__fdOut.good()) { _PrintError("Writing rules", 0); }
  __fdOut.close();
  return;
}

inline
void
MyRuleList::
_PrintError(std::string&& __funcName, DWORD __errCode) {
  std::cout << __funcName << " failed with: "
    << std::hex << __errCode << std::endl;
}