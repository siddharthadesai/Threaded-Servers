# Siddhartha Desai
#
# Implements server using leader follower model

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

pthread_mutex_t lock;


void error(char *msg)
{
    perror(msg);
    exit(-1);
}


struct sockaddr_in make_server_addr(short port)
{
    struct sockaddr_in addr;
    memset(&(my_addr.sin_zero), 0, 8);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    return addr;
}


int create_server_socket(short port)
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


void handle_request(int client_socket)
{
    char fileName[BUFSIZ];
    get_file_request(client_socket, fileName);
    write_file_to_client_socket(fileName, client_socket);
    close(client_socket);
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


void * accept_client_requests(void * server_socket_param)
{
    int client_socket;
    int server_socket = *(int *) server_socket_param;
    int thread_num;
    struct sockaddr_in client_addr;
    socklen_t sin_size = sizeof client_addr;
    set_time_out(5);

    while (1)
    { 
        pthread_mutex_lock(&lock);
        if ((client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &sin_size)) == -1)
            error("accept");
        pthread_mutex_unlock(&lock);
        set_time_out(5);
        handle_request(client_socket);
    }
}


void create_thread_pool(pthread_t pool[5], int server_socket)
{
    pthread_mutex_init(&lock, NULL);

    for (int i = 0; i < 5; i++)
    {
        if ( pthread_create(&pool[i], 0, accept_client_requests, (void *) &server_socket) > 0 )
        {
            error("could not create thread");
            return;
        }
    }
}


void destroy_thread_pool(pthread_t pool[5])
{
    for (int i = 0; i < 5; i++)
        pthread_join(pool[i], NULL);

    pthread_mutex_destroy(&lock);
    pthread_exit(0);
}


int main(int argc, char *argv[])
{
    if ( argc != 2 )
        error("not enough arguments");

    pthread_t pool[5];

    int port = atoi(argv[1]);
    int server_socket = create_server_socket(port);

    create_thread_pool(pool, server_socket);
    destroy_thread_pool(pool);

    close(server_socket);

    return 0;
}
