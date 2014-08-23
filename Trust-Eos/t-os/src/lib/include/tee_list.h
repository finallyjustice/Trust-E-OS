#ifndef _LIB_TEE_LIST_H_
#define _LIB_TEE_LIST_H_

#include "tee_internal_api.h"

typedef struct _LIST {
    struct _LIST *prev;
    struct _LIST *next;
}LIST;

typedef struct _NODE {
    LIST list;
    int data;
}NODE;

// 获取成员在结构体中的偏移
#define OFFSET_MEMBER(type, member)  (uint32_t)(&((type*)0)->member)

// 已知链表地址，获取其所属结构体的指针
// list_addr : 链表指针
// type      : 链表所述结构体类型
// list_mem_name : 链表在该结构体中的成员名字
// return *type;
#define LIST_ENTRY(list_ptr,type,list_mem_name) ((type*)((uint32_t)list_ptr - OFFSET_MEMBER(type,list_mem_name)))

static inline void ListInit(LIST *head)
{
    head->prev = head;
    head->next = head;
}

static inline void ListAdd(LIST *head, LIST *node)
{
    head->next->prev = node;
    node->next = head->next;
    head->next = node;
    node->prev = head;
}

static inline void ListAddTail(LIST *head, LIST *node)
{
    head->prev->next = node;
    node->prev = head->prev;
    head->prev = node;
    node->next = head;
}

static inline bool ListEmpty(LIST *head)
{
    return head->next == head;
}

static inline void ListDel(LIST *head, LIST *node)
{
    if (ListEmpty(head)) {
        return;
    }
    while (head->next != node) {
        head = head->next;
    }
    head->next = node->next;
    node->next->prev = head;
}


// 说明：返回删除的节点
static inline LIST* ListDelLast(LIST *head) 
{
    LIST *del = NULL;
    if (!ListEmpty(head)) {
        del = head->prev;
        head->prev->prev->next = head;
        head->prev = head->prev->prev;
    }
    return del;
}

static inline LIST* ListDelFirst(LIST *head) 
{
    LIST *del = NULL;
    if (!ListEmpty(head)) {
        del = head->next;
        head->next->next->prev = head;
        head->next = head->next->next;
    }
    return del;
}

static inline int32_t ListCount(LIST *head)
{
    LIST *p = head;
    int count = 0;
    while (p->next != head) {
        count++;
        p = p->next;
    }
    return count;
}

#endif