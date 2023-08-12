#include "filter.h"
#include "rules_list.h"

#include <ntstrsafe.h>

VOID
PrintAceList(_In_ PRULES_ACE __head);

PRULES_ACE
FindAce(_In_ PRULES_ACE __head, _In_ LPCWSTR __procName);

NTSTATUS
AddAce(
  _Inout_ PRULES_ACE* __head,
  _In_ LPCWSTR __procName,
  _In_ ULONG __accessMask,
  _Out_ PLONG __result
);

NTSTATUS
DelAce(
  _Inout_ PRULES_ACE* __head,
  _In_ PRULES_ACE __node,
  _Out_ PLONG __result
);

NTSTATUS
DelAllAces(
  _Inout_ PRULES_ACE* __head,
  _Out_ PLONG __result
);

VOID
PrintAclList(_In_ PRULES_ACL __head);

PRULES_ACL
FindAcl(
  _In_ PRULES_ACL __head,
  _In_ LPCWSTR __fileName
);

NTSTATUS
AddAcl(
  _Inout_ PRULES_ACL* __head,
  _In_ LPCWSTR __fileName,
  _Out_ PLONG __result
);

NTSTATUS
DelAcl(
  _Inout_ PRULES_ACL* __head,
  _In_ PRULES_ACL __node,
  _Out_ PLONG __result
);

NTSTATUS
DelAllAcls(
  _Inout_ PRULES_ACL* __head,
  _Out_ PLONG __result
);

BOOLEAN
_CompareStrings(
  _In_ LPCWSTR __str1,
  _In_ PUNICODE_STRING __str2
);

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, PrintAceList)
#pragma alloc_text(PAGE, FindAce)
#pragma alloc_text(PAGE, AddAce)
#pragma alloc_text(PAGE, DelAce)
#pragma alloc_text(PAGE, DelAllAces)
#pragma alloc_text(PAGE, PrintAclList)
#pragma alloc_text(PAGE, FindAcl)
#pragma alloc_text(PAGE, AddAcl)
#pragma alloc_text(PAGE, DelAcl)
#pragma alloc_text(PAGE, DelAllAcls)
#pragma alloc_text(PAGE, PrintRules)
#pragma alloc_text(PAGE, AddRule)
#pragma alloc_text(PAGE, DelRule)
#pragma alloc_text(PAGE, DelAllRules)
#pragma alloc_text(PAGE, _CompareStrings)

#endif // ALLOC_PRAGMA


VOID
PrintAceList(_In_ PRULES_ACE __head) {
  PAGED_CODE();

  if (__head == NULL) {
    PRINT("\n\t\tEmpty");
    return;
  }

  while (__head != NULL) {
    PRINT(
      "\n"
      "\t\tProcess: %ws\n"
      "\t\t\tAccess mask: 0x%04x\n"
      "\t\t\tTag: 0x%08x\n"
      "\t\t\tNext: 0x%08x\n",
      __head->_procName, __head->_accessMask, __head->_allocTag, __head->_nextAce
    );
    __head = __head->_nextAce;
  }
}

PRULES_ACE
FindAce(_In_ PRULES_ACE __head, _In_ LPCWSTR __procName) {
  NTSTATUS __res;
  UNICODE_STRING __uProcName;

  PAGED_CODE();

  __res = RtlUnicodeStringInit(&__uProcName, __procName);
  
  if (!NT_SUCCESS(__res)) { return NULL; }

  while (__head != NULL) {
    if (_CompareStrings(__head->_procName, &__uProcName)) {
      break;
    }
    __head = __head->_nextAce;
  }

  return __head;
}

NTSTATUS
AddAce(
  _Inout_ PRULES_ACE* __head,
  _In_ LPCWSTR __procName,
  _In_ ULONG __accessMask,
  _Out_ PLONG __result
) {
  NTSTATUS __res;
  PRULES_ACE __newNode = NULL;
  PRULES_ACE __tmpNode = NULL;
  ULONG __allocTag;
  size_t __procNameLen;

  PAGED_CODE();

  __allocTag = _global._rulesList._curTag;
  __newNode = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(RULES_ACE), __allocTag);

  if (__newNode == NULL) {
    *__result = FSFLT_DRIVER_ERROR_ADD_ACE_MEMORY_ALLOC;
    return STATUS_INSUFFICIENT_RESOURCES;
  }
  RtlSecureZeroMemory(__newNode, sizeof(RULES_ACE));

  __res = RtlStringCchLengthW(__procName, PROCESS_BUFFER_SIZE, &__procNameLen);

  if (!NT_SUCCESS(__res)) {
    ExFreePoolWithTag(__newNode, __allocTag);
    *__result = FSFLT_DRIVER_ERROR_ADD_ACE_GET_PROCESS_NAME_LENGTH;
    return __res;
  }
  __res = RtlStringCchCopyNW(
    __newNode->_procName,
    PROCESS_BUFFER_SIZE,
    __procName,
    __procNameLen
  );

  if (!NT_SUCCESS(__res)) {
    ExFreePoolWithTag(__newNode, __allocTag);
    *__result = FSFLT_DRIVER_ERROR_ADD_ACE_COPY_PROCESS_NAME;
    return __res;
  }

  __newNode->_accessMask = __accessMask;
  __newNode->_allocTag = __allocTag;
  __newNode->_nextAce = NULL;

  if (*__head == NULL) { *__head = __newNode; }
  else {
    __tmpNode = *__head;
    while (__tmpNode->_nextAce != NULL) { __tmpNode = __tmpNode->_nextAce; }
    __tmpNode->_nextAce = __newNode;
  }

  ++(_global._rulesList._curTag);

  *__result = FSFLT_ERROR_SUCCESS;
  return STATUS_SUCCESS;
}

NTSTATUS
DelAce(
  _Inout_ PRULES_ACE* __head,
  _In_ PRULES_ACE __node,
  _Out_ PLONG __result
) {
  ULONG __tmpTag;
  PRULES_ACE __tmpNode = NULL;

  PAGED_CODE();

  if (*__head == __node) {
    __tmpNode = *__head;
    *__head = (*__head)->_nextAce;
    
    ExFreePoolWithTag(__tmpNode, __tmpNode->_allocTag);
  } else if (__node->_nextAce == NULL) {
    __tmpNode = *__head;
    
    while (__tmpNode->_nextAce != __node) {
      if (__tmpNode->_nextAce == NULL) {
        *__result = FSFLT_DRIVER_ERROR_DEL_ACE_INVALID_LIST;
        return STATUS_INVALID_PARAMETER_2;
      }
      __tmpNode = __tmpNode->_nextAce;
    }

    __tmpNode->_nextAce = NULL;

    ExFreePoolWithTag(__node, __node->_allocTag);
  } else {
    __tmpNode = __node->_nextAce;
    __tmpTag = __node->_allocTag;

    // Copy next node, but keep tag of the current node
    RtlCopyMemory(__node, __tmpNode, sizeof(*__tmpNode));
    __node->_allocTag = __tmpTag;

    // Free next node
    ExFreePoolWithTag(__tmpNode, __tmpNode->_allocTag);
  }

  *__result = FSFLT_ERROR_SUCCESS;
  return STATUS_SUCCESS;

}

NTSTATUS
DelAllAces(
  _Inout_ PRULES_ACE* __head,
  _Out_ PLONG __result
) {
  NTSTATUS __res;

  PAGED_CODE();

  while (*__head != NULL) {
    __res = DelAce(__head, *__head, __result);

    if (!NT_SUCCESS(__res)) { return __res; }
  }

  *__result = FSFLT_ERROR_SUCCESS;
  return STATUS_SUCCESS;
}

VOID
PrintAclList(_In_ PRULES_ACL __head) {
  PAGED_CODE();

  if (__head == NULL) { PRINT("\n\tEmpty"); }

  while (__head != NULL) {
    PRINT(
      "\n"
      "\tFilename: %ws\n"
      "\tACE list ptr: 0x%08x\n"
      "\tACE count: %lu\n"
      "\tTag: 0x%08x\n"
      "\tNext: 0x%08x\n",
      __head->_fileName, __head->_aceList, __head->_aceCnt,
      __head->_allocTag, __head->_nextAcl
    );

    PrintAceList(__head->_aceList);

    __head = __head->_nextAcl;
  }
}

PRULES_ACL
FindAcl(
  _In_ PRULES_ACL __head,
  _In_ LPCWSTR __fileName
) {
  NTSTATUS __res;
  UNICODE_STRING __uFileName;

  PAGED_CODE();

  __res = RtlUnicodeStringInit(&__uFileName, __fileName);
  
  if (!NT_SUCCESS(__res)) { return NULL; }

  while (__head != NULL) {
    if (_CompareStrings(__head->_fileName, &__uFileName)) {
      break;
    }
    __head = __head->_nextAcl;
  }

  return __head;
}

NTSTATUS
AddAcl(
  _Inout_ PRULES_ACL* __head,
  _In_ LPCWSTR __fileName,
  _Out_ PLONG __result
) {
  NTSTATUS __res;
  PRULES_ACL __newNode = NULL;
  PRULES_ACL __tmpNode = NULL;
  ULONG __allocTag;
  size_t __fileNameLen;

  PAGED_CODE();

  __allocTag = _global._rulesList._curTag;
  __newNode = ExAllocatePool2(POOL_FLAG_PAGED, sizeof(RULES_ACL), __allocTag);

  if (__newNode == NULL) {
    *__result = FSFLT_DRIVER_ERROR_ADD_ACL_MEMORY_ALLOC;
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  RtlSecureZeroMemory(__newNode, sizeof(RULES_ACL));

  __res = RtlStringCchLengthW(__fileName, FILE_BUFFER_SIZE, &__fileNameLen);

  if (!NT_SUCCESS(__res)) {
    ExFreePoolWithTag(__newNode, __allocTag);
    *__result = FSFLT_DRIVER_ERROR_ADD_ACL_GET_FILE_NAME_LENGTH;
    return __res;
  }

  __res = RtlStringCchCopyNW(
    __newNode->_fileName,
    FILE_BUFFER_SIZE,
    __fileName,
    __fileNameLen
  );

  if (!NT_SUCCESS(__res)) {
    *__result = FSFLT_DRIVER_ERROR_ADD_ACL_COPY_FILE_NAME;
    return __res;
  }

  __newNode->_aceList = NULL;
  __newNode->_aceCnt = 0;
  __newNode->_allocTag = __allocTag;
  __newNode->_nextAcl = NULL;

  if (*__head == NULL) { *__head = __newNode; }
  else {
    __tmpNode = *__head;
    while (__tmpNode->_nextAcl != NULL) { __tmpNode = __tmpNode->_nextAcl; }
    __tmpNode->_nextAcl = __newNode;
  }

  ++(_global._rulesList._curTag);

  *__result = FSFLT_ERROR_SUCCESS;
  return STATUS_SUCCESS;
}

NTSTATUS
DelAcl(
  _Inout_ PRULES_ACL* __head,
  _In_ PRULES_ACL __node,
  _Out_ PLONG __result
) {
  ULONG __tmpTag;
  PRULES_ACL __tmpNode = NULL;

  PAGED_CODE();

  if (*__head == NULL) {
    *__result = FSFLT_DRIVER_ERROR_DEL_ACL_INVALID_HEAD_PTR;
    return STATUS_INVALID_PARAMETER_1;
  }

  if (__node == NULL) {
    *__result = FSFLT_DRIVER_ERROR_DEL_ACL_INVALID_NODE_PTR;
    return STATUS_INVALID_PARAMETER_2;
  }

  if (*__head == __node) {
    __tmpNode = *__head;
    *__head = (*__head)->_nextAcl;
    
    ExFreePoolWithTag(__tmpNode, __tmpNode->_allocTag);
  } else if (__node->_nextAcl == NULL) {
    __tmpNode = *__head;

    while (__tmpNode->_nextAcl != __node) {
      if (__tmpNode->_nextAcl == NULL) {
        *__result = FSFLT_DRIVER_ERROR_DEL_ACL_INVALID_LIST;
        return STATUS_INVALID_PARAMETER_2;
      }

      __tmpNode = __tmpNode->_nextAcl;
    }

    __tmpNode->_nextAcl = NULL;

    ExFreePoolWithTag(__node, __node->_allocTag);
  } else {
    __tmpNode = __node->_nextAcl;
    __tmpTag = __node->_allocTag;

    // Copy next node, but keep tag of the current node
    RtlCopyMemory(__node, __tmpNode, sizeof(*__tmpNode));
    __node->_allocTag = __tmpTag;

    // Free next node
    ExFreePoolWithTag(__tmpNode, __tmpNode->_allocTag);
  }

  *__result = FSFLT_ERROR_SUCCESS;
  return STATUS_SUCCESS;
}

NTSTATUS
DelAllAcls(
  _Inout_ PRULES_ACL* __head,
  _Out_ PLONG __result
) {
  NTSTATUS __res;

  PAGED_CODE();

  while (*__head != NULL) {

     __res = DelAllAces(&((*__head)->_aceList), __result);
     if (!NT_SUCCESS(__res)) { return __res; }

    __res = DelAcl(__head, *__head, __result);
    if (!NT_SUCCESS(__res)) { return __res; }
  }

  *__result = FSFLT_ERROR_SUCCESS;
  return STATUS_SUCCESS;
}

VOID
PrintRules(_In_ RULES_LIST __head) {

  PAGED_CODE();

  PRINT_STATUS(
    "\n"
    "Current allocation tag: 0x%08x\n"
    "Rules list ptr: %p\n"
    "Rules count: %lu\n"
    "Rules:\n",
    __head._curTag, __head._aclList, __head._aclCnt
  );

  PrintAclList(__head._aclList);
}

NTSTATUS
AddRule(
  _Inout_ PRULES_LIST __head,
  _In_ LPCWSTR __fileName,
  _In_ LPCWSTR __procName,
  _In_ ULONG __accessMask,
  _Out_ PLONG __result
) {
  NTSTATUS __res;
  PRULES_ACL __acl;
  PRULES_ACE __ace;

  PAGED_CODE();

  if (__head == NULL) {
    *__result = FSFLT_DRIVER_ERROR_ADD_RULE_INVALID_LIST_PTR;
    return STATUS_INVALID_PARAMETER_1;
  }
  if (__fileName == NULL) {
    *__result = FSFLT_DRIVER_ERROR_ADD_RULE_INVALID_FILE_NAME;
    return STATUS_INVALID_PARAMETER_2;
  }

  if (__procName == NULL) {
    *__result = FSFLT_DRIVER_ERROR_ADD_RULE_INVALID_PROCESS_NAME;
    return STATUS_INVALID_PARAMETER_3;
  }

  if (__accessMask & ~MASK_ALLOW_ALL) {
    *__result = FSFLT_DRIVER_ERROR_ADD_RULE_INVALID_ACCESS_MASK;
    return STATUS_INVALID_PARAMETER_4;
  }

  __acl = FindAcl(__head->_aclList, __fileName);
  // Acl with this fileName doesn't exist

  if (__acl == NULL) {
    __res = AddAcl(&(__head->_aclList), __fileName, __result);

    if (!NT_SUCCESS(__res)) { return __res; }

    ++(__head->_aclCnt);

    __acl = FindAcl(__head->_aclList, __fileName);

    if (__acl == NULL) {
      *__result = FSFLT_DRIVER_ERROR_ADD_RULE_FIND_ACL;
      return STATUS_INVALID_PARAMETER_2;
    }
  }

  __ace = FindAce(__acl->_aceList, __procName);
  // Ace with this process name already exists
  if (__ace != NULL) {
    *__result = FSFLT_DRIVER_ERROR_ADD_RULE_ACE_ALREADY_EXISTS;
    return STATUS_INVALID_PARAMETER_3;
  }
  // Otherwise add ace to list
  __res = AddAce(&(__acl->_aceList), __procName, __accessMask, __result);

  if (!NT_SUCCESS(__res)) { return __res; }

  ++(__acl->_aceCnt);

  *__result = FSFLT_ERROR_SUCCESS;
  return STATUS_SUCCESS;
}

NTSTATUS
DelRule(
  _Inout_ PRULES_LIST __head,
  _In_ LPCWSTR __fileName,
  _In_ LPCWSTR __procName,
  _Out_ PLONG __result
) {
  NTSTATUS __res;
  PRULES_ACL __acl;
  PRULES_ACE __ace;

  UNREFERENCED_PARAMETER(__res);
  UNREFERENCED_PARAMETER(__acl);
  UNREFERENCED_PARAMETER(__ace);

  PAGED_CODE();

  if (__head == NULL) {
    *__result = FSFLT_DRIVER_ERROR_DEL_RULE_INVALID_LIST_PTR;
    return STATUS_INVALID_PARAMETER_1;
  }
  if (__fileName == NULL) {
    *__result = FSFLT_DRIVER_ERROR_DEL_RULE_INVALID_FILE_NAME;
    return STATUS_INVALID_PARAMETER_2;
  }

  if (__procName == NULL) {
    *__result = FSFLT_DRIVER_ERROR_DEL_RULE_INVALID_PROCESS_NAME;
    return STATUS_INVALID_PARAMETER_3;
  }

  __acl = FindAcl(__head->_aclList, __fileName);
  if (__acl == NULL) {
    *__result = FSFLT_DRIVER_ERROR_DEL_RULE_FIND_ACL;
    return STATUS_INVALID_PARAMETER_2;
  }

  __ace = FindAce(__acl->_aceList, __procName);
  if (__ace == NULL) {
    *__result = FSFLT_DRIVER_ERROR_DEL_RULE_FIND_ACE;
    return STATUS_INVALID_PARAMETER_3;
  }

  __res = DelAce(&(__acl->_aceList), __ace, __result);
  if (!NT_SUCCESS(__res)) { return __res; }

  --(__acl->_aceCnt);

  if (__acl->_aceCnt == 0) {
    __res = DelAcl(&(__head->_aclList), __acl, __result);
    if (!NT_SUCCESS(__res)) { return __res; }
    
    --(_global._rulesList._aclCnt);
  }

  *__result = FSFLT_ERROR_SUCCESS;
  return STATUS_SUCCESS;
}

NTSTATUS
DelAllRules(
  _Inout_ PRULES_LIST __head,
  _Out_ PLONG __result
) {
  NTSTATUS __res;

  PAGED_CODE();

  if (__head == NULL) {
    *__result = FSFLT_DRIVER_ERROR_DEL_ALL_RULES_INVALID_LIST_PTR;
    return STATUS_INVALID_PARAMETER_1;
  }

   __res = DelAllAcls(&(__head->_aclList), __result);
   if (!NT_SUCCESS(__res)) { return __res; }

  __head->_aclCnt = 0;
  __head->_curTag = START_TAG;

  *__result = FSFLT_ERROR_SUCCESS;
  return STATUS_SUCCESS;
}


BOOLEAN
_CompareStrings(
  _In_ LPCWSTR __str1,
  _In_ PUNICODE_STRING __str2
) {
  NTSTATUS __res;
  UNICODE_STRING __uStr1;

  PAGED_CODE();

  __res = RtlUnicodeStringInit(&__uStr1, __str1);
  if (!NT_SUCCESS(__res)) { return FALSE; }

  return RtlEqualUnicodeString(&__uStr1, __str2, TRUE);
}
