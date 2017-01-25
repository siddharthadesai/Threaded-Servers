# Siddhartha Desai
# Stack implementation by Ray Klefstad, UCI
#
# Implements server using thread pool model

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
#include "Timer.h"

#define BACKLOG 200
#define QUE_MAX 1024
#define ELE int

// Queue implementation
ELE _que[QUE_MAX];
int _front = 0, _rear = 0;

void que_error(char *msg)
{
    fprintf(stderr, "Error: %s\n", msg);
    exit(-1);
}


int que_is_full()
{
    return (_rear + 1) % QUE_MAX == _front; /* this is why one slot is unused */
}


int que_is_empty()
{
    return _front == _rear;
}


void que_enq(ELE v)
{
    if ( que_is_full() )
        que_error("enq on full queue");
    _que[_rear++] = v;
    if ( _rear >= QUE_MAX )
        _rear = 0;
}


ELE que_deq()
{
    if ( que_is_empty() )
        que_error("deq on empty queue");
    ELE ret = _que[_front++];
    if ( _front >= QUE_MAX )
        _front = 0;
    return ret;
}


pthread_mutex_t lock;
pthread_cond_t empty;


void error(char *msg)
{
    perror(msg);
    exit(-1);
}


typedef struct thread_pool
{
    pthread_t thread;
    int client_socket;
} thread_pool;


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


void * handle_request(void * pool)
{
	thread_pool *thread = (thread_pool *) pool;
    char fileName[MAXDATASIZE];

    pthread_mutex_lock(&lock);

    while (1)
    {
        while (que_is_empty())
            pthread_cond_wait(&empty, &lock);

    	thread->client_socket = que_deq();
        pthread_mutex_unlock(&lock);

        get_file_request(thread->client_socket, fileName);
        write_file_to_client_socket(fileName, thread->client_socket);
        close(thread->client_socket);
    }
}


void time_out(int arg)
{
    fprintf(stderr,  "Server timed out\n");
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
        set_time_out(5);
        que_enq(client_socket);
        pthread_cond_signal(&empty);
    }
}


void create_thread_pool(thread_pool * pool[5])
{
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&empty, NULL);

    for (int i = 0; i < 5; i++)
    {
        pool[i] = malloc(sizeof(thread_pool));
        if ( pthread_create(&(pool[i]->thread), 0, handle_request, (void *) pool[i]) > 0 )
        {
            error("could not create thread");
            return;
        }
    }
}


void destroy_thread_pool(thread_pool * pool[5])
{
    for (int i = 0; i < 5; i++)
        pthread_join(pool[i]->thread, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&empty);
    pthread_exit(0);
}


int main(int argc, char *argv[])
{
    if ( argc != 2 )
        error("not enough arguments");
    
    thread_pool * pool[5];
    create_thread_pool(pool);

    int port = atoi(argv[1]);
    int server_socket = create_server_socket(port);
    accept_client_requests(server_socket);
    close(server_socket);

    destroy_thread_pool(pool);

    return 0;
}
