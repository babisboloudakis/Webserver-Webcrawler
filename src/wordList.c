#include "../headers/wordList.h"

void WordListCreate(WordList ** wordList ) {
    // Used to initialize a WordList

    (*wordList) = malloc( sizeof(WordList) );
    (*wordList)->length = 0;
    (*wordList)->head = NULL;

}

void WordListDestroy(WordList * wordList ) {
    // Used to free our WordList including 
    // its nodes and its strings

    WordNode * node = NULL;
    while ( wordList->length ) {

        node = wordList->head;
        wordList->head = wordList->head->next;
        wordList->length--;
        free(node->word);
        free(node);
    }

    free(wordList);

}

void WordListEmpty(WordList * wordList ) {
    // Used to free our WordList including 
    // its nodes and its strings
    WordNode * node = NULL;
    while ( wordList->length ) {

        node = wordList->head;
        wordList->head = wordList->head->next;
        wordList->length--;
        free(node->word);
        free(node);

    }
}

int WordListSearch(WordList * wordList , char * word ) {
    // Searches through the WordList, return position
    // if found, -1 otherwise

    if ( wordList->length == 0 ) {
        return -1;
    }

    WordNode * currentNode = wordList->head;
    for ( int i = 0; i < wordList->length; i++ ) {
        if ( strcmp(currentNode->word,word) == 0 ) {
            return i;
        }
        currentNode = currentNode->next;
    }
    return -1;

}

int WordListInsert( WordList * wordList, char * w) {
    // Used to insert a string in our WordList
    // string is stored in heap

    WordNode * wordNode;
    wordNode = malloc(sizeof(WordNode));
    wordNode->word = malloc( sizeof(char) * strlen(w) + 1);
    strcpy(wordNode->word, w);

    if ( wordList->length == 0 ) {
        wordList->head = wordNode;
    } else {
        wordNode->next = wordList->head;
        wordList->head = wordNode;
    }
    wordList->length++;
    return 0;

}

void WordListPrint( WordList * wordList ) {
    // Prints out the WordList, used
    // for debugging reassons

    if ( wordList->length == 0 ) {
        printf("List is empty...\n");
    }
    WordNode * currentNode = wordList->head;
    for ( int i = 0; i < wordList->length; i++ ) {
        printf("        %s\n", currentNode->word );
        currentNode = currentNode->next;
    }
    printf("\n");

}

void WordListPop( WordList * list, char * ret) {
    /*	Pop the first element of the list
    , removing it from the list */
    
    /*	extract the value from head */
    char * value = list->head->word;
    strcpy(ret,value);
    /*	remove it from the list */
    WordNode * node = list->head;
    list->head = node->next;
    free(node->word);
    free(node);
    list->length--;
    return ;
}

char * WordListGetWord( WordList * wordList, int index ) {
    // Used to get a specific word of the list specified
    // with an index

    if ( wordList->length == 0 ) {
        return NULL;
    }

    WordNode * node = wordList->head;
    for ( int i = 0; i < index; i++ ) {
        node = node->next;
    }
    return node->word;

}
