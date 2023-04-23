#include "rules_list.h"

#include <iostream>
#include <string>
#include <windows.h>

void
MyRuleList::
Clear() { _rules.clear(); }

void
MyRuleList::
LoadRules() {
  LSTATUS __res = ERROR_SUCCESS;
  HKEY __hKey = NULL;
  DWORD __dataSize = 0;
  PWCHAR __data = nullptr;
  std::wstring __jsonKeyValue;

  __res = RegOpenKeyExW(
    HKEY_LOCAL_MACHINE,
    SUBKEY,
    0,
    KEY_READ,
    &__hKey
  );
  if (__res != ERROR_SUCCESS) {
    this->_PrintError("RegOpenKeyExW", __res);
    return;
  }
  
  __res = RegGetValueW(
    __hKey,
    NULL,
    KEY_NAME,
    RRF_RT_REG_SZ,
    NULL,
    NULL,
    &__dataSize
  );
  if (__res != ERROR_SUCCESS) {
    this->_PrintError("RegGetValueW size", __res);
    return;
  }
  __data = reinterpret_cast<PWCHAR>(new(std::nothrow) BYTE[__dataSize]);
  if (__data == nullptr) {
    RegCloseKey(__hKey);
    this->_PrintError("Allocation memory", 0);
  }
  __res = RegGetValueW(
    __hKey,
    NULL,
    KEY_NAME,
    RRF_RT_REG_SZ,
    NULL,
    __data,
    &__dataSize
  );
  if (__res != ERROR_SUCCESS || __data == NULL) {
    this->_PrintError("RegGetValueW string", __res);
    return;
  }
  __jsonKeyValue.assign(__data);

  delete[] reinterpret_cast<PBYTE>(__data);

  this->Clear();
  {
    MyRuleNode __rule;
    std::wstring __levelStr(2, 0);
    std::wstring __entryDummy(L"{\"Process\":\"\",\"level\":}");
    std::wstring __processDummy(L"\"Process\":\"");
    std::wstring __levelDummy(L"\"level\":");
    size_t __shift = 0;
    while (__jsonKeyValue.length() > __entryDummy.length()) {
      __shift = __jsonKeyValue.find(__processDummy);
      if (__shift == std::wstring::npos) { break; }
      __shift += __processDummy.length();
      __jsonKeyValue.assign(__jsonKeyValue.data() + __shift);
      __shift = __jsonKeyValue.find(L"\"");
      if (__shift == std::wstring::npos) { break; }
      __rule._procName.assign(__jsonKeyValue, 0, __shift);
      __shift = __jsonKeyValue.find(__levelDummy);
      if (__shift == std::wstring::npos) { break; }
      try {
        __shift += __levelDummy.length();
        __levelStr[0] = __jsonKeyValue.at(__shift);
        __rule._level = std::stoi(__levelStr);
      }
      catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
      }
      _rules.push_back(__rule);
    } 
  }

  RegCloseKey(__hKey);
}

void
MyRuleList::
PrintRules() {
  int32_t cnt = 0;
  for (const auto& __el : _rules) {
    std::wcout << ++cnt << ' '
      << __el._procName << ' ' << __el._level << std::endl;
  }
  std::cout << "Total: " << _rules.size() << " rule(s)" << std::endl;
}

void
MyRuleList::
DeleteRule(size_t __idx) {
  if (_rules.empty()) {
    std::cout << "List already empty" << std::endl;
    return;
  }
  if (__idx > _rules.size() - 1) {
    std::cout << "Index must be in range: 1..." << _rules.size() << std::endl;
    return;
  }
  size_t __cnt = 0;
  for (auto __beg = _rules.cbegin(); __beg != _rules.cend(); ++__beg) {
    if (__cnt == __idx) {
      _rules.erase(__beg);
      break;
    }
    ++__cnt;
  }
  this->_RenewKeyValue();
}

void
MyRuleList::
AddRule(std::wstring& __procName, int32_t __level) {
  if (__level < 1 || __level > 4) {
    std::cout << "Level must be in range 1...4" << std::endl;
    return;
  }
  _rules.push_back(MyRuleNode{std::move(__procName), __level});
  this->_RenewKeyValue();
}

void
MyRuleList::
_RenewKeyValue() {
  std::wstring __jsonKeyValue(L"[");

  {
    std::wstring __processDummy(L"{\"Process\":\"");
    std::wstring __levelDummy(L"\",\"level\":");

    size_t __curr = 0;
    for (const auto& __el : _rules) {
      __jsonKeyValue.append(__processDummy);
      __jsonKeyValue.append(__el._procName);
      __jsonKeyValue.append(__levelDummy);
      __jsonKeyValue.append(std::to_wstring(__el._level));
      __jsonKeyValue.append(L"}");
      if (++__curr != _rules.size()) { __jsonKeyValue.append(L","); }
    }
  }
  __jsonKeyValue.append(L"]");

  LSTATUS __res = ERROR_SUCCESS;
  HKEY __hKey = NULL;

  __res = RegOpenKeyExW(
    HKEY_LOCAL_MACHINE,
    SUBKEY,
    0,
    KEY_WRITE,
    &__hKey
  );
  if (__res != ERROR_SUCCESS) {
    this->_PrintError("RegOpenKeyExW", __res);
    return;
  }

  __res = RegSetKeyValueW(
    __hKey,
    NULL,
    KEY_NAME,
    REG_SZ,
    __jsonKeyValue.data(),
    __jsonKeyValue.length() * sizeof(std::wstring::value_type)
  );
  if (__res != ERROR_SUCCESS) {
    this->_PrintError("RegSetKeyValueW", __res);
  }
  RegCloseKey(__hKey);
  return;
}

inline
void
MyRuleList::
_PrintError(std::string&& __funcName, DWORD __errCode) {
  std::cout << __funcName << " failed with: "
    << std::hex << __errCode << std::endl;
}