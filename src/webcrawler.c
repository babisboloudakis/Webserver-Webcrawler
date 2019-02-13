/*	Charalambos Mpooloudakis
    1115201500103
    project3 - syspro
*/

/*	C Header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/poll.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>

/*	My header files */
#include "../headers/wordList.h"

/*	Defines */
#define BUFFERSIZE 2048

int writeBuffer(char * buffer, int fd) {
    /* Safe function to send a buffer str to the 
    given file descriptor */
    int ret = 0;
    int totalRet = 0;
    int len = strlen(buffer);
    while (totalRet <= len) {
        /*	Write until you send the whole
        request */
        ret = write(fd,buffer+totalRet,BUFFERSIZE-totalRet);
        if ( ret == -1 ) {
            perror("Write buffer");
            exit(-2);
        }
        totalRet += ret;
    }
}


/* :----------	GLOBAL VARIABLES ---------:  */

/*	Root folder for websites
used inside threads */
char * ip = NULL;
int port = 0;
char * dest = NULL;

/*	Task pool */
WordList * task_pool = NULL;
WordList * uniqueUrls = NULL;

/*	Global flag to terminate threads */
int threadFlag = 1;

/*	Locks for threads etc. */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stats = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t nonempty = PTHREAD_COND_INITIALIZER;

/*	Stats variables */
int totalPages = 0;
long totalBytes = 0;

/*	Get item from buffer function */
void getTask(char * url) {

    /*	Lock mutex to safely extract task */
    pthread_mutex_lock(&mutex);

    /*	Get a task from the buffer */
    while ( task_pool->length <= 0 ) {
        /*	Do nothing and wait
        for tasks to arrive */
        pthread_cond_wait(&nonempty,&mutex);
        if ( threadFlag == 0 ) {
            pthread_mutex_unlock(&mutex);
            pthread_exit(0);
        }
    }
    WordListPop(task_pool,url);

    /*	Unlock mutex so other threads can get tasks too */
    pthread_mutex_unlock(&mutex);

}


/*	Function that threads use when they are
    created */
void * crawlerThread( ) {

    /*	Keep running until main thread decides to shutdown */
    while( threadFlag ) { 

        /*	Get task from task pool */
        char task[BUFFERSIZE];
        getTask(task);

        /*	Connect with the server */
        int sock = -1;
        struct sockaddr_in server;
        struct sockaddr * serverptr = (struct sockaddr * ) &server;
        struct hostent * rem;

        /*	Create socket to talk to server*/
        if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
            perror("socket");
            exit(-2);
        }
        if ( ( rem = gethostbyname(ip)) == NULL ) {
            perror("Gethostbyname");
            exit(-2);
        }
        server.sin_family = AF_INET;
        memcpy(&server.sin_addr, rem->h_addr ,rem->h_length);
        server.sin_port = htons(port);

        /*	Connect to the web server */
        if ( connect(sock,serverptr,sizeof(server)) < 0 ) {
            perror("connect");
            exit(-2);
        }
        
        /*	Generate HTTP request asking for the file on the url 
        from the webserver */
        char buffer[BUFFERSIZE];
        char filename[BUFFERSIZE];
        memset(buffer,0,BUFFERSIZE);
        memset(filename,0,BUFFERSIZE);
        /*	Look for the "/site" substring */
        for ( int i = 0; i < strlen(task); i++ ) {
            if( task[i] == '/' && task[i+1] == 's' && task[i+2] == 'i' ) {
                strcpy(filename,task+i);
                break;
            }
        }
        /*	Write the http request into a string */
        char request[BUFFERSIZE];
        memset(request,0,BUFFERSIZE);
        strcpy(request,"GET ");
        strcat(request,filename);
        strcat(request, " HTTP/1.1\r\n");
        strcat(request, "User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\n");
        strcat(request, "Host: www.tutorialspoint.com\r\n");
        strcat(request, "Accept-Language: en-us\r\n");
        strcat(request, "Accept-Encoding: gzip, deflate\r\n");
        strcat(request, "Connection: Closed\r\n");
        strcat(request, "\r\n");

        /*	Safely send HTTP request to server */
        writeBuffer(request,sock);

        /*	Create directory for the copy */
        memset(buffer,0,BUFFERSIZE);
        strcpy(buffer,dest);
        mkdir(buffer,0777);
        memset(request,0,BUFFERSIZE);
        strcpy(request,buffer);
        strncat(buffer,filename,7);
        mkdir(buffer,0777);
        strcat(request,filename);
        FILE * file = fopen(request,"w");

        /*	Receive reply from server */
        int ret = 0, gotLen = 0, len = 0, conLen = -1;
        int conFlag = 0;
        int validHttp = -1;
        memset(buffer,0,BUFFERSIZE);
        while ( 1 ) {
            ret = read(sock,buffer,BUFFERSIZE);
            len += ret;
            /*	Check if the http reply is valid */
            if ( validHttp == -1 ) {
                if ( strstr(buffer,"HTTP/1.1 200 OK") == NULL ) {
                    validHttp = 0;
                    break;
                }
                validHttp = 1;
            }
            /*	Take the content length  */
            if ( gotLen == 0 ) {
                if ( len > 102 ) {
                    conLen = atoi(strstr(buffer,"Content-Length: ") + strlen("Content-Length: "));
                    gotLen = 1;
                }
            }
            /*	If we are done with header
            start writting html content on the file copy */
            if ( conFlag > 0 ) {
                fwrite(buffer,1, strlen(buffer), file);
            }
            if ( strstr(buffer,"<!DOCTYPE html>") != NULL ) {
                conFlag = 1;
                fwrite(strstr(buffer,"<!DOCTYPE html>"), 1, strlen(strstr(buffer,"<!DOCTYPE html>")), file);
            }
            /*	Keep reading until you find the end of the html document */
            if ( strstr(buffer,"</html>") != NULL ) {
                break;
            }
            memset(buffer,0,BUFFERSIZE);
        }

        fclose(file);
        if ( validHttp == 0 ) {
            continue;
        }

        /*	Increase appropriate stat variables 
        , also make sure to use mutexes to prevent
        multiple threads concurent use of these global
        variables. */
        pthread_mutex_lock(&stats);
        totalPages++;
        totalBytes += conLen;
        pthread_mutex_unlock(&stats); 

        /*	Read the file we just copied and look for
        links. Every "new" link found will be added into
        the task pool */
        file = fopen(request,"r");
        memset(buffer,0,BUFFERSIZE);
        char url[256];
        char finalUrl[256];
        while ( fgets(buffer,BUFFERSIZE,file) != NULL ) {
            /*	If you find a link add it, else scan next line */
            char * p = strstr(buffer,"href=\"");
            if ( p == NULL ) {
                memset(buffer,0,BUFFERSIZE);
                continue;
            }
            p += strlen("href=\"");
            /*	Position pointer on link */
            int offset=0;
            while( *p != '"') {
                offset++;
                p++;
            }
            p -= offset;
            /*	Copy link */
            memset(url,0,256);
            memset(finalUrl,0,256);
            strncpy(url,p,offset);
            strcpy(finalUrl,"https://");
            strcat(finalUrl,ip);
            strcat(finalUrl,":");
            memset(buffer,0,BUFFERSIZE);
            sprintf(buffer,"%d",port);
            strcat(finalUrl,buffer);
            memset(buffer,0,BUFFERSIZE);
            strcat(finalUrl,url+2);
            /*	Insert into list but first make sure
            we haven't already got that page. */
            pthread_mutex_lock(&mutex);
            if ( WordListSearch(uniqueUrls,finalUrl) == -1 ) {
                WordListInsert(task_pool,finalUrl);
                WordListInsert(uniqueUrls,finalUrl);
                pthread_cond_signal(&nonempty);
            }
            pthread_mutex_unlock(&mutex);
            memset(buffer,0,BUFFERSIZE);
        }
        fclose(file);

    }
    /*	Thread is done */
    pthread_exit(0);
}

/*	Main function */
int main ( int argc, char * argv[] ) {

    /*	Check if command line arguments are OK */
    if ( argc < 12 ) {
        printf("insufficient arguments \n./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL\n");
        exit(2);
    }
    if ( strcmp(argv[1],"-h") != 0 ) {
        printf("-h not found \n./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL\n");
        exit(3);
    }
    if ( strcmp(argv[3], "-p") != 0 ) {
        printf("-p not found\n./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL\n");
        exit(4);
    }
    if ( strcmp(argv[5], "-c") != 0 ) {
        printf("-c not found\n./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL\n");
        exit(5);
    }
    if ( strcmp(argv[7], "-t") != 0 ) {
        printf("-t not found\n./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL\n");
        exit(6);
    }
    if ( strcmp(argv[9], "-d") != 0 ) {
        printf("-d not found\n./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL\n");
        exit(6);
    }

    /*	Store command line arguments */
    char * host = argv[2]; 
    int serving_port = atoi(argv[4]);
    int command_port = atoi(argv[6]);
    int num_of_threads = atoi(argv[8]);
    char * dir = argv[10];
    char * startUrl = argv[11];
    ip = host;
    port = serving_port;
    dest = dir;



    /*	Create task pool and insert starting url
    into the task pool */
    WordListCreate(&task_pool);
    WordListCreate(&uniqueUrls);
    WordListInsert(uniqueUrls,startUrl);
    WordListInsert(task_pool,startUrl);
    pthread_cond_signal(&nonempty);

    /*	Create thread pool 
        First malloc thread pool
        Then create threads */
    pthread_t * thread_pool = NULL;
    if ( ( thread_pool = malloc(sizeof(pthread_t) * num_of_threads) ) == NULL ) {
        perror("thread pool malloc");
        exit(-2);
    }

    /*	Declarations for sockets and for poll */
    struct pollfd sockets[2];
    int sock,csock;
    struct sockaddr_in  cserver;
    struct sockaddr * cserverptr = ( struct sockaddr * ) &cserver;

    /*	Create both sockets
    and initiaze the options for TCP connection with any adress */

    /*	command socket creation */
    if ( (csock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror("socket");
        exit(-2);
    }
    memset(&cserver, 0, sizeof(cserver));
    cserver.sin_family = AF_INET;
    cserver.sin_addr.s_addr = htonl(INADDR_ANY);
    cserver.sin_port = htons(command_port);

    /*	set option to be able to reuse adress */
    int flag = 1;
    if ( (setsockopt(csock,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(int))) < 0 ) {
        perror("setsockopt");
        exit(-2);
    }
    if ( bind(csock,cserverptr,sizeof(cserver)) < 0 ) {
        perror("bind");
        exit(-2);
    }
    
    /*	Listen for connections on command socket */
    if ( listen(csock,10) < 0 ) {
        perror("listen");
        exit(-2);
    }
    printf("Listening to port %d for commands...\n",command_port);

    /*	Variables used to track server running time */
    struct timeval start;
    gettimeofday(&start,0);

    /*	Create threads */
    for (int i = 0; i < num_of_threads; i++) {
        pthread_create(&(thread_pool[i]),0,crawlerThread,0);
    }

    /*	Accept any connections on command port */
    int fdsock = 0;
    while(1) {
        if ( (fdsock = accept(csock,NULL,NULL)) < 0 ) {
            if ( errno != EWOULDBLOCK ) {
                perror("accept");
                exit(-2);
            }
            break;
        }
        printf("\nAccepted command connection\n");
        char buffer[BUFFERSIZE];
        memset(buffer,0,BUFFERSIZE);
        /*	Receive command from client */
        if ( read(fdsock,buffer,BUFFERSIZE) < 0 ) {
            perror("read command");
            exit(-2);                    
        }
        /*	Handle SHUTDOWN, or STATS command */
        if ( strncmp(buffer,"SHUTDOWN\r",9) == 0 ) { 
            printf("Closing the crawler...\n");
            threadFlag = 0;
            pthread_cond_broadcast(&nonempty);
            break;
        } else if ( strncmp(buffer,"STATS\r",6) == 0 ) {
            /*	Calculate elapsed time
            and print it */
            struct timeval end;
            gettimeofday(&end,0);
            time_t sec = end.tv_sec - start.tv_sec;
            time_t msec = end.tv_usec;
            printf("Crawler up for %ld:%ld.%ld , downloaded %d pages, %ld bytes\n",sec/60, sec % 60, msec, totalPages,totalBytes);
        } else if ( strncmp(buffer,"SEARCH ",7) == 0 ) {
                    printf("Sorry, not implemented\n");
        } else {
            printf("Unknown command\n");
        }
    }

    printf("Waiting for threads to finish ...\n");
    /*	Wait for threads to finish their job 
    before exiting */
    for ( int i = 0; i < num_of_threads; i++ ) {
        pthread_join(thread_pool[i],0);
    }

    printf("Free-ing data structures ...\n");
    /*	Frees etc. */
    free(thread_pool);
    WordListDestroy(task_pool);
    WordListDestroy(uniqueUrls);

}
/*	End of webcrawler */