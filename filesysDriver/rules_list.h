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
  struct RULES_ACL* _nextAcl;
} RULES_ACL, *PRULES_ACL;

typedef struct _RULES_LIST {
  UNICODE_STRING _fileName;
  PRULES_ACL _aclList;
  ULONG _aclCnt;
  ULONG _curTag;
} RULES_LIST, *PRULES_LIST;

VOID
PrintAceList(_In_ PRULES_ACE __head);

PRULES_ACE
FindAce(_In_ PRULES_ACE __head, _In_ LPCWSTR __procName);

NTSTATUS
AddAce(
  _Inout_ PRULES_ACE* __head,
  _In_ LPCWSTR __procName,
  _In_ ULONG __accessMask,
  _Inout_ PLONG __result
);

NTSTATUS
DelAce(
  _Inout_ PRULES_ACE* __head,
  _In_ PRULES_ACE __node,
  _Inout_ PLONG __result
);

NTSTATUS
DelAllAces(
  _Inout_ PRULES_ACE* __head,
  PLONG __result
);

VOID
PrintAclList(_In_ PRULES_ACL __head);

PRULES_ACL
FindAcl(_In_ PRULES_ACL __head, _In_ LPCWSTR __procName);

NTSTATUS
AddAcl(
  _Inout_ PRULES_ACL* __head,
  _In_ LPCWSTR __fileName,
  _In_ ULONG __accessMask,
  _Inout_ PLONG __result
);

NTSTATUS
DelAcl(
  _Inout_ PRULES_ACL* __head,
  _In_ PRULES_ACL __node,
  _Inout_ PLONG __result
);

NTSTATUS
DelAllAcls(
  _Inout_ PRULES_ACL* __head,
  PLONG __result
);

#endif // !_MY_LIST_H
