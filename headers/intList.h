#include <stdio.h>
#include <stdlib.h>


typedef struct IntListNode { 
    struct IntListNode * next;
    int data;
} IntListNode;


typedef struct IntList { 
    int length;
    IntListNode * head;
}IntList;


void IntListCreate(IntList **);
void IntListDestroy(IntList *);
int IntListInsert(IntList *, int );
int IntListSearch(IntList *, int );
void IntListPrint(IntList *);
int IntListPop(IntList *);
IntListNode * IntListGet(IntList *, int);
