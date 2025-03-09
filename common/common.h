#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>

void print_err_exit(const char *fmt, ...);
char *bin2hex(const unsigned char *input, size_t len);

struct queue_node
{
    struct queue_node *next;
    int *client_socket;
};
typedef struct queue_node node_t;

void enqueue(int *client_socket);
int *dequeue();