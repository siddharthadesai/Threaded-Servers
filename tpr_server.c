# Siddhartha Desai
#
# Implements server using thread per request model

#include <sys/types.h>
#include <signal.h>
#include <sys/uio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#define BACKLOG 200
#define MAXDATASIZE 300


void error(char * msg)
{
    perror(msg);
    exit(-1);
}


struct Thread_data
{
    int id;
    pthread_t thread_id;
    char * host;
    int port;
    char path[MAXDATASIZE];
};


struct sockaddr_in make_server_addr(int port)
{
    struct sockaddr_in addr;
    memset(&(my_addr.sin_zero), 0, 8);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    return addr;
}


int create_server_socket(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if ( s == -1 )
        error("socket() error!");
    struct sockaddr_in addr = make_server_addr(port);
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
        error("bind() error");
    if (listen(s, BACKLOG) == -1)
        error("listen() error")
    return s;
}


void get_file_request(int socket, char *fileName)
{
    char buffer[MAXDATASIZE];
    int data = read(socket, buffer, MAXDATASIZE);
    if ( data < 0 )
        error("read() error");
    buffer[n] = '\0';
    strcpy(fileName, buf);
    printf("Server got file name of '%s'\n", fileName);
}


void write_file_to_client_socket(char *file, int socket)
{
    char buffer[MAXDATASIZE];
    int infile = open(file, O_RDONLY);
    int data;
    if ( infile == -1 )
        error("open() error");
    while ( (data = read(infile, buffer, MAXDATASIZE)) > 0 )
        write(socket, buffer, data);
    close(infile);
}


void * handle_request(void * client_socket)
{
	int client_socket = *(int *) client_socket;
    char fileName[MAXDATASIZE];
    get_file_request(client_socket, fileName);
    write_file_to_client_socket(fileName, client_socket);
    close(client_socket);
    return 0;
}


void time_out(int arg)
{
    fprintf(stderr, "Server timed out\n");
    exit(0);
}


void set_time_out(int seconds)
{
    struct itimerval value = {0};
    value.it_value.tv_sec = seconds;
    setitimer(ITIMER_REAL, &value, NULL);
    signal(SIGALRM, time_out);
}


void accept_client_requests(int server_socket)
{
    int client_socket;
    int thread_num;
    struct sockaddr_in client_addr;
    socklen_t sin_size = sizeof client_addr;
    set_time_out(5);
    while ( (client_socket =
            accept(server_socket, (struct sockaddr*)&client_addr, &sin_size)) > 0 )
    {
        set_time_out(15);
        pthread_t slave_thread;
		if ( pthread_create(&slave_thread, NULL, handle_request, (void *) &client_socket) < 0 )
			error("could not create thread");
		pthread_join(slave_thread, NULL); 
    }
}


int main(int argc, char *argv[])
{
    if ( argc != 2 )
        error("not enough arguments");

    int port = atoi(argv[1]);
    int server_socket = create_server_socket(port);
    accept_client_requests(server_socket);
    close(server_socket);
    return 0;
}
