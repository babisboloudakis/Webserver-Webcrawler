#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

int main ( int argc, char * argv[] ) {

    //Handle arguments
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

    int command_port = atoi(argv[4]);

    int sock,csock;
    struct sockaddr_in server;
    struct sockaddr * serverptr = (struct sockaddr * ) &server;
    /*	Create command socket */
    if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror("socket");
        exit(-2);
    }
    server.sin_family = AF_INET;
    inet_pton(AF_INET,"127.0.0.1",&(server.sin_addr));
    server.sin_port = htons(command_port);
    
    /*	Listen for connections on command socket */
    if ( connect(sock,&server,sizeof(server)) < 0 ) {
        perror("connect");
        exit(-2);
    }
    printf("Connected to port %d\n",command_port);
    char buffer[256];
    while(1) {
        /*	handle connections */
        printf("Tell me what to send\n");
        fgets(buffer,256,stdin);
        write(sock,buffer,256);
    }

}
