#ifndef _MY_LIST_H
#define _MY_LIST_H

#include <ntifs.h>
#include <ntstrsafe.h>

#include "../file_sys_filter.h"

#define DRIVER_SUBKEY_NAME L"\\REGISTRY\\MACHINE" SUBKEY_NAME
#define PROCESS_NAME_LENGTH 1 << 8

typedef struct {
  WCHAR _processName[PROCESS_NAME_LENGTH];
  ULONG _level;
  ULONG _tag;
  struct MyListRule* _next;
} MyRuleList;

#endif // !_MY_LIST_H
