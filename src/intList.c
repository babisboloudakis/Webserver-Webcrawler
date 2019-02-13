#include "../headers/intList.h"

void IntListCreate(IntList ** list) {
    /*	Properly initialize an IntList */
    (*list) = malloc( sizeof(IntList) );
    (*list)->length = 0;
    (*list)->head = NULL;
}

void IntListDestroy(IntList * list) {
    /*	Free the IntList and all its ListNodes */
    IntListNode * currentNode = NULL;
    while ( list->length ) {
        /*	Free every list node */
        currentNode = list->head;
        list->head = list->head->next;
        list->length--;
        free(currentNode);
    }
    free(list);
}

int IntListInsert(IntList * list, int d ) {
    /*	Insert int on the start of the list */

    // if ( IntListSearch(list,d) != -1 ) {
    //     return 1;
    // } 
    IntListNode * node = malloc(sizeof(IntListNode));
    if ( node == NULL ) {
        perror("Malloc IntListInsert");
        exit(-2);
    }
    node->data = d;
    node->next = list->head;
    list->head = node;
    list->length++;
    return 0;
}


int IntListSearch(IntList * list, int d ) {
    /*	Search list and return the position of d
    if found, else return -1 */
    if ( list->length == 0 ) {
        return -1;
    }

    IntListNode * currentNode = list->head;
    for ( int i = 0; i < list->length; i++ ) {
        if ( currentNode->data == d ) {
            return i;
        }
        currentNode = currentNode->next;
    }
    return -1;
}

void IntListPrint(IntList * list) {
    /*	Print the list contents 
    ( for debugging purposes ) */
    if ( list->length == 0 ) {
        printf("List is empty...\n");
    }
    printf(" +----------- Our list is -----------+ \n");
    IntListNode * currentNode = list->head;
    for ( int i = 0; i < list->length; i++ ) {
        printf("%d    |", currentNode->data );
        currentNode = currentNode->next;
    }
    printf("\n");
}


int IntListPop(IntList * list ) {
    /*	Pop the first element of the list
    , removing it from the list */
    if ( list->length == 0 ) return -1;
    /*	extract the value from head */
    int value = list->head->data;
    /*	remove it from the list */
    IntListNode * node = list->head;
    list->head = node->next;
    free(node);
    list->length--;
    return value;
}


IntListNode * IntListGet( IntList * list, int index ) {
    if ( list->length == 0 ) {
        return NULL;
    }

    IntListNode * currentNode = list->head;
    for ( int i = 0; i < index; i++ ) {
        currentNode = currentNode->next;
    }
    return currentNode;
}