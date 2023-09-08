#ifndef _MY_LIST_H
#define _MY_LIST_H

#ifndef _DRIVER
#define _DRIVER
#endif // !_DRIVER

#include "../file_sys_filter.h"

typedef struct _RULES_ACE {
  WCHAR _procName[PROCESS_BUFFER_SIZE];
  ULONG _accessMask;
  ULONG _allocTag;
  struct _RULES_ACE* _nextAce;
} RULES_ACE, *PRULES_ACE;

typedef struct _RULES_ACL {
  WCHAR _fileName[FILE_BUFFER_SIZE];
  PRULES_ACE _aceList;
  ULONG _aceCnt;
  ULONG _allocTag;
  struct _RULES_ACL* _nextAcl;
} RULES_ACL, *PRULES_ACL;

typedef struct _RULES_LIST {
  PRULES_ACL _aclList;
  ULONG _aclCnt;
  ULONG _curTag;
} RULES_LIST, *PRULES_LIST;

VOID
PrintRules(_In_ RULES_LIST __head);

NTSTATUS
AddRule(
  _Inout_ PRULES_LIST __head,
  _In_ LPCWSTR __fileName,
  _In_ LPCWSTR __procName,
  _In_ ULONG __accessMask,
  _Out_ PLONG __result
);

NTSTATUS
DelRule(
  _Inout_ PRULES_LIST __head,
  _In_ LPCWSTR __fileName,
  _In_ LPCWSTR __procName,
  _Out_ PLONG __result
);

NTSTATUS
DelAllRules(
  _Inout_ PRULES_LIST __head,
  _Out_ PLONG __result
);

NTSTATUS
GetRulePermissions(
  _In_ PRULES_LIST __head,
  _In_ LPCWSTR __fileName,
  _In_ LPCWSTR __procName,
  _Inout_ PULONG __accessMask
);

#endif // !_MY_LIST_H
