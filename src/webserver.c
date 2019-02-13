/*	Charalambos Mpooloudakis
    1115201500103
    project3 - syspro
*/

/*	C Header files */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>

/*	My header files */
#include "../headers/intList.h"

/*	Defines */
#define BUFFERSIZE 2048

int getTask();
void insertTask(int);

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
char * root = NULL;

/*	Task pool */
IntList * task_pool = NULL;

/*	Global flag to terminate threads */
int threadFlag = 1;

/*	Locks for threads etc. */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stats = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t nonempty = PTHREAD_COND_INITIALIZER;

/*	Stats variables */
int totalPages = 0;
long totalBytes = 0;



char * getHttpFile( char * request ) {
    /*	Returns the full filename on the given
    http request. */
    char * temp = request;
    while ( *temp != '/' ) {
        temp++;
    }
    char * temp2 = temp;
    while ( *temp2 != ' ' ) {
        temp2++;
    }
    *temp2 = '\0';
    return temp;
}


char * HttpReply( int filefd , int task ) {

    char buffer[BUFFERSIZE];
    char answer[BUFFERSIZE];
    memset(buffer,0,BUFFERSIZE);

    if ( filefd < 0 ) {

            /*	Error with the file trying to open */
            if ( errno == EACCES ){
                /*	No read permission for the page */
                memset(buffer,0,BUFFERSIZE);
                memset(answer,0,BUFFERSIZE);
                int len = strlen("<html> Error403: Permission denied. </html>");
                sprintf(buffer,"%d",len);
                strcpy(answer,"HTTP/1.1 403 Forbidden\n");
                strcat(answer,"Date: Mon, 27 May 2018 12:28:53 GMT\n");
                strcat(answer,"Server: myhttpd/1.0.0 (Ubuntu64)\n");
                strcat(answer,"Content-Length: ");
                strcat(answer,buffer);
                strcat(answer,"\nContent-Type: text/html\n");
                strcat(answer,"Connection: Closed\n");
                strcat(answer,"\n");
                strcat(answer,"<html> Error403: Permission denied. </html>");
                writeBuffer(answer,task);
            } else {
                /*	File doesn't exist */
                memset(buffer,0,BUFFERSIZE);
                memset(answer,0,BUFFERSIZE);
                int len = strlen("<html>Error404: file not found.</html>");
                sprintf(buffer,"%d",len);
                strcpy(answer,"HTTP/1.1 404 Not Found\n");
                strcat(answer,"Date: Mon, 27 May 2018 12:28:53 GMT\n");
                strcat(answer,"Server: myhttpd/1.0.0 (Ubuntu64)\n");
                strcat(answer,"Content-Length: ");
                strcat(answer,buffer);
                strcat(answer,"\nContent-Type: text/html\n");
                strcat(answer,"Connection: Closed\n");
                strcat(answer,"\n");
                strcat(answer,"<html>Error404: file not found.</html>");
                writeBuffer(answer,task);
            }

    } else {
            /*	File exists and we have permissions
            to READ from it */
            memset(buffer,0,BUFFERSIZE);
            memset(answer,0,BUFFERSIZE);
            /*	Count file size */
            int len = lseek(filefd, 0L, SEEK_END) +BUFFERSIZE;
            lseek(filefd, 0L, SEEK_SET);
            sprintf(buffer,"%d",len);
            /*	Record statistics
            secure the global variables with
            mutex */
            pthread_mutex_lock(&stats);
            totalPages++;
            totalBytes += len;
            pthread_mutex_unlock(&stats);
            /*	Create HTTP request */
            strcpy(answer,"HTTP/1.1 200 OK\n");
            strcat(answer,"Date: Mon, 27 May 2018 12:28:53 GMT\n");
            strcat(answer,"Server: myhttpd/1.0.0 (Ubuntu64)\n");
            strcat(answer,"Content-Length: ");
            strcat(answer,buffer);
            strcat(answer,"\nContent-Type: text/html\n");
            strcat(answer,"Connection: Closed\n");
            strcat(answer,"\r\n");
            writeBuffer(answer,task);
            int ret = 0;
            memset(buffer,0,BUFFERSIZE);
            /*	Send the website content */
            while ( ( ret = read(filefd,buffer,BUFFERSIZE-1) ) > 0 ) {
                if ( ret == -1 ) {
                    perror("read buffer");
                    exit(-2);
                }
                memset(buffer+ret,0,BUFFERSIZE-ret);
                writeBuffer(buffer,task);
                memset(buffer,0,BUFFERSIZE);
            }

        }
        
}


/*	Function that threads use when they are
    created */
void * threadFunction( ) {

    while( threadFlag ) {

        /*	Get task from task pool */
        int task = getTask();
        /*	READ HTTP GET REQUEST */
        char buffer[BUFFERSIZE];
        memset(buffer,0,BUFFERSIZE);
        char temp[BUFFERSIZE];
        memset(temp,0,BUFFERSIZE);
        int ret = 0;
        int len = 0;
        int firstRead = 1;
        /*	Keep reading until the end of HTTP request */
        while ( 1 ) {
            ret = read(task,temp,BUFFERSIZE);
            if ( ret < 0 ) {
                perror("read http");
                exit(-2);
            } 
            strcat(buffer,temp);
            memset(temp,0,BUFFERSIZE);
            len = strlen(buffer);
            /*	Check if its a GET request */
            if ( firstRead ) {
                if ( buffer[0] != 'G' || buffer[1] != 'E' || buffer[2] != 'T' ) {
                    firstRead = -1;
                    break;
                }
                firstRead = 0;
            }
            if ( buffer[len-1] == '\n' && buffer[len-2] == '\r' && buffer[len-3] == '\n' ) {
                /*	An http request should end with blank line
                ........\n
                \r\n */
                break;
            }
        }
        /*	In case that we received Invalid HTTP request */
        if ( firstRead == -1 ) {
            printf("Recieved invalid http request\n");
            continue;
        }
        
        /*	Extract file from HTTP request */
        strcpy(buffer,getHttpFile(buffer));
        /*	Open the given file */
        memset(temp,0,BUFFERSIZE);
        strcpy(temp,root);
        strcat(temp,buffer);

        /*	Try to open the given page */
        int filefd = open(temp,O_RDONLY);
        HttpReply(filefd,task);
        /*	Close file */ 
        close(filefd);
    }
    /*	Thread is done */
    pthread_exit(0);
}


/*	Get item from buffer function */
int getTask() {

    int task = -1;
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
    task = IntListPop(task_pool);

    /*	Unlock mutex so other threads can get tasks too */
    pthread_mutex_unlock(&mutex);

    return task;

}


/*	Inset item into buffer function */
void insertTask(int fd) {
    /*	Lock mutex to safely insert Task on pool */
    pthread_mutex_lock(&mutex);
    IntListInsert(task_pool,fd);
    /*	Unlock mutex so the threads can get the item */
    pthread_mutex_unlock(&mutex);
}


/*	Main function */
int main ( int argc, char * argv[] ) {

    /*	Check if command line arguments are OK */
    if ( argc <= 8 ) {
        printf("insufficient arguments \n./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\n");
        exit(2);
    }
    if ( strcmp(argv[1],"-p") != 0 ) {
        printf("-p not found\n./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\n");
        exit(3);
    }
    if ( strcmp(argv[3], "-c") != 0 ) {
        printf("-c not found\n./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\n");
        exit(4);
    }
    if ( strcmp(argv[5], "-t") != 0 ) {
        printf("-t not found\n./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\n");
        exit(5);
    }
    if ( strcmp(argv[7], "-d") != 0 ) {
        printf("-d not found\n./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\n");
        exit(6);
    }

    /*	Store command line arguments */
    int serving_port = atoi(argv[2]);
    int command_port = atoi(argv[4]);
    int num_of_threads = atoi(argv[6]);
    char * root_dir = argv[8];
    root = root_dir;

    /*	Initialize condition variables, task pool etc. */
    IntListCreate(&task_pool);

    /*	Create thread pool 
        First malloc thread pool
        Then create threads */
    pthread_t * thread_pool;
    if ( ( thread_pool = malloc(sizeof(pthread_t) * num_of_threads) ) == NULL ) {
        perror("thread pool malloc");
        exit(-2);
    }

    /*	Declarations for sockets and for poll */
    struct pollfd sockets[2];
    int sock,csock;
    struct sockaddr_in server, cserver;
    struct sockaddr * serverptr = (struct sockaddr * ) &server;
    struct sockaddr * cserverptr = ( struct sockaddr * ) &cserver;

    /*	Create both sockets
    and initiaze the options for TCP connection with any adress */

    /*	serving socket creation */
    if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror("socket");
        exit(-2);
    }
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(serving_port);
    /*	command socket creation */
    if ( (csock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror("socket");
        exit(-2);
    }
    memset(&cserver, 0, sizeof(server));
    cserver.sin_family = AF_INET;
    cserver.sin_addr.s_addr = htonl(INADDR_ANY);
    cserver.sin_port = htons(command_port);

    /*	set option to be able to reuse adress */
    int flag = 1;
    if ( (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(int))) < 0 ) {
        perror("setsockopt");
        exit(-2);
    }
    if ( (setsockopt(csock,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(int))) < 0 ) {
        perror("setsockopt");
        exit(-2);
    }

    /*	Bind socket on command port */
    if ( bind(sock,serverptr,sizeof(server)) < 0 ) {
        perror("bind");
        exit(-2);
    }
    if ( bind(csock,cserverptr,sizeof(server)) < 0 ) {
        perror("bind");
        exit(-2);
    }
    
    /*	Listen for connections on command socket */
    if ( listen(sock,10) < 0 ) {
        perror("listen");
        exit(-2);
    }
    if ( listen(csock,10) < 0 ) {
        perror("listen");
        exit(-2);
    }
    printf("Listening to port %d for requests...\n",serving_port);
    printf("Listening to port %d for commands...\n",command_port);

    /*	Variables used to track server running time */
    struct timeval start;
    gettimeofday(&start,0);

    /*	Loop and wait for any incoming data on
    any of the two ports */
    sockets[0].fd = sock;
    sockets[0].events = POLLIN;
    sockets[1].fd = csock;
    sockets[1].events = POLLIN;

    /*	Create threads */
    for (int i = 0; i < num_of_threads; i++) {
        pthread_create(&(thread_pool[i]),0,threadFunction,0);
    }

    /*	Call poll with timeout */
    int fdsock = 0;
    int run = 1;
    while(1) {
        /*	Keep looking for a socket ready to write */
        int ret = poll(sockets,2,600000);
        if ( ret < 0 ) { /*	error */
            perror("poll");
            exit(-2);
        } else if ( ret == 0 ) {  /* poll timeout */
            printf("Timeout, server has been waiting for commands for too long ...\n");
            exit(-1);
        }
        /*	Serving port handling */        
        if( sockets[0].revents == POLLIN ) {
                if ( (fdsock = accept(sock,NULL,NULL)) < 0 ) {
                    if ( errno != EWOULDBLOCK ) {
                        perror("accept");
                        exit(-2);
                    }
                    break;
                }
                printf("\nAccepted serving connection\n");
                /*	Insert the file descriptor into the buffer
                for threads to read */
                insertTask(fdsock);
                pthread_cond_signal(&nonempty);
        }
        /*	Command port handling */
        if( sockets[1].revents == POLLIN ) {
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
                    printf("Closing the server...\r");
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
                    printf("Server up for %ld:%ld.%ld , served %d pages, %ld bytes\n",sec/60, sec % 60, msec, totalPages,totalBytes);
                } else {
                    printf("Unknown command\n");
                }
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
    IntListDestroy(task_pool);

}
/*	End of webserver */