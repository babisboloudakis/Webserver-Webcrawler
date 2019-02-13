#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// WordNode just contains a dynamically created
// string and a pointer to the next node
typedef struct WordNode {
    struct WordNode * next;
    char * word;
} WordNode;


// WordList is used to store parameters of
// a given command
typedef struct WordList {
    int length;
    WordNode * head;
} WordList;


void WordListCreate(WordList **);
void WordListDestroy(WordList *);
void WordListEmpty(WordList *);
int WordListSearch(WordList *, char *);
int WordListInsert(WordList *, char *);
void WordListPrint(WordList *);
char * WordListGetWord(WordList *, int);
void WordListPop( WordList *, char * );
