#include "filter.h"
#include "rules_list.h"

#include <ntstrsafe.h>

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
#pragma alloc_text(PAGE, _CompareStrings)

#endif // ALLOC_PRAGMA


VOID
PrintAceList(_In_ PRULES_ACE __head) {
  PAGED_CODE();

  if (__head == NULL) {
    PRINT_STATUS("\t\tEmpty\n");
  }
  while (__head != NULL) {
    PRINT_STATUS(
      "\n"
      "\t\tProcess: %ws\n"
      "\t\t\tAccess mask: 0x%04x\n"
      "\t\t\tTag: 0x%04x\n"
      "\t\t\tNext: %p\n",
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
  _Inout_ PLONG __result
) {
  NTSTATUS __res;
  PRULES_ACE __newNode = NULL;
  PRULES_ACE __tmpNode = NULL;
  ULONG __allocTag;
  size_t __procNameLen;

  PAGED_CODE();

  if (__procName == NULL) {
    *__result = FSFLT_DRIVER_ERROR_ADD_ACE_INVALID_PROCESS_NAME;
    return STATUS_INVALID_PARAMETER_2;
  }

  if (__accessMask & ~MASK_ALLOW_ALL) {
    *__result = FSFLT_DRIVER_ERROR_ADD_ACE_INVALID_ACCESS_MASK;
    return STATUS_INVALID_PARAMETER_3;
  }

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
  _Inout_ PLONG __result
) {
  ULONG __tmpTag;
  PRULES_ACE __tmpNode = NULL;

  PAGED_CODE();

  if (*__head == NULL) {
    *__result = FSFLT_DRIVER_ERROR_DEL_ACE_INVALID_HEAD_PTR;
    return STATUS_INVALID_PARAMETER_1;
  }

  if (__node == NULL) {
    *__result = FSFLT_DRIVER_ERROR_DEL_ACE_INVALID_NODE_PTR;
    return STATUS_INVALID_PARAMETER_2;
  }

  if (__node->_nextAce == NULL) {
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
  _Inout_ PLONG __result
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
  while (__head != NULL) {
    PRINT_STATUS(
      "\n\tFilename: %ws\n"
      "\n\tACE list ptr: %p\n"
      "\n\tACE count: %lu\n"
      "\t\tTag: 0x%04x\n"
      "\t\tNext: %p\n",
      __head->_fileName, __head->_aceList, __head->_aceCnt,
      __head->_allocTag, __head->_nextAcl
    );
    PrintAceList(__head->_aceList);
  }
}

PRULES_ACL
FindAcl(_In_ PRULES_ACL __head, _In_ LPCWSTR __fileName) {
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
  _Inout_ PLONG __result
) {
  NTSTATUS __res;
  PRULES_ACL __newNode = NULL;
  PRULES_ACL __tmpNode = NULL;
  ULONG __allocTag;
  size_t __fileNameLen;

  PAGED_CODE();

  if (__fileName == NULL) {
    *__result = FSFLT_DRIVER_ERROR_ADD_ACL_INVALID_FILE_NAME;
    return STATUS_INVALID_PARAMETER_2;
  }

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
  _Inout_ PLONG __result
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

  if (__node->_nextAcl == NULL) {
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
  _Inout_ PLONG __result
) {
  NTSTATUS __res;

  PAGED_CODE();

  while (*__head != NULL) {
    __res = DelAcl(__head, *__head, __result);
    
    if (!NT_SUCCESS(__res)) { return __res; }
  }

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
