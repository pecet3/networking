#include "../common/common.h"
#include <pthread.h>

#define SA struct sockaddr
#define SERVER_PORT 80
#define BUF_SIZE 4096

#define THREAD_POOL_SIZE 3

pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t mux = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;

///// for thread pool ////////////

struct queue_node
{
    struct queue_node *next;
    int *client_socket;
};
typedef struct queue_node node_t;

node_t *head = NULL;
node_t *tail = NULL;
void enqueue(int *client_socket)
{
    node_t *newnode = malloc(sizeof(node_t));
    newnode->client_socket = client_socket;

    if (tail == NULL)
    {
        head = newnode;
    }
    else
    {
        tail->next = newnode;
    }
    tail = newnode;
}

int *dequeue()
{
    if (head == NULL)
    {
        return NULL;
    }
    else
    {
        int *result = head->client_socket;
        node_t *temp = head;
        head = head->next;
        if (head == NULL)
        {
            tail = NULL;
        }
        free(temp);
        return result;
    }
}

//////////////////////////////////////

char *extract_routenmethod(const char *input)
{
    if (input == NULL)
        return NULL;

    const char *first_space = strchr(input, ' ');
    if (first_space == NULL)
        return NULL;

    const char *second_space = strchr(first_space + 1, ' ');
    if (second_space == NULL)
        return NULL;

    size_t length = second_space - (input);

    char *route = malloc(length + 1);
    if (route == NULL)
        return NULL;

    strncpy(route, input, length);
    route[length] = '\0'; // Correct null-termination
    return route;
}

void *handle_conn(void *connfd_ptr)
{

    int connfd = *((int *)connfd_ptr);
    free(connfd_ptr);

    uint8_t buf[BUF_SIZE + 1];
    uint8_t recvline[BUF_SIZE];

    int n;
    memset(buf, 0, BUF_SIZE);

    while ((n = read(connfd, recvline, BUF_SIZE - 1)) > 0)
    {
        char *route = extract_routenmethod((const char *)(recvline));

        if (route != NULL)
        {
            printf("Extracted route: %s\n", route);
            if (!strcmp(route, "GET /"))
            {
                snprintf((char *)buf, BUF_SIZE, "HTTP/1.0 200 OK\r\n\r\n<html><b>index</b></html>");
                write(connfd, (char *)buf, strlen((char *)buf));
            }
            else if (!strcmp(route, "GET /test"))
            {
                snprintf((char *)buf, BUF_SIZE, "HTTP/1.0 200 OK\r\n\r\n<html><b>test</b></html>");
                write(connfd, (char *)buf, strlen((char *)buf));
            }
            else if (!strcmp(route, "GET /test2"))
            {
                snprintf((char *)buf, BUF_SIZE, "HTTP/1.0 200 OK\r\n\r\n<html><b>test2</b></html>");
                write(connfd, (char *)buf, strlen((char *)buf));
            }
            else if (!strncmp(route, "GET /file", 9))
            {
                const char *path = route + 10;
                printf("path: %s\n", path);
                FILE *fp = fopen(path, "r");
                if (fp == NULL)
                {
                    snprintf((char *)buf, BUF_SIZE, "HTTP/1.0 404 Not Found\r\n\r\n<html><b>File not found</b></html>");
                    write(connfd, (char *)buf, strlen((char *)buf));
                }
                else
                {
                    snprintf((char *)buf, BUF_SIZE, "HTTP/1.0 200 OK\r\n\r\n");
                    write(connfd, buf, strlen((char *)buf));

                    int bytes_read;
                    while ((bytes_read = fread(buf, 1, BUF_SIZE, fp)) > 0)
                    {
                        write(connfd, buf, bytes_read);
                    }
                    fclose(fp);
                }
            }
        }

        if (n < 0)
            print_err_exit("read err");

        if (recvline[n - 1] == '\n')
            break;

        memset(recvline, 0, BUF_SIZE);
        free(route);
    }
    close(connfd);
    return NULL;
}

void *thread_func(void *arg)
{
    while (1)
    {
        int *sock;
        pthread_mutex_lock(&mux);
        while ((sock = dequeue()) == NULL)
        {
            pthread_cond_wait(&condition_var, &mux);
        }
        pthread_mutex_unlock(&mux);

        if (sock != NULL)
        {
            handle_conn(sock);
        }
    }
}

int main(int argc, char **argv)
{

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_create(&thread_pool[i], NULL, thread_func, NULL);
    }

    int listenfd = setup_srv(SERVER_PORT, 10);
    printf(">Server is listening\n");
    while (1)
    {
        struct sockaddr_in addr;
        socklen_t addr_len;
        char client_address[BUF_SIZE + 1];
        int connfd = accept(listenfd, (SA *)&addr, &addr_len);
        inet_ntop(AF_INET, &addr.sin_addr, client_address, BUF_SIZE);

        printf("> client connected, ip: %s\n", client_address);
        fflush(stdout);

        // Allocate memory for the socket descriptor and pass it to the thread
        int *sock = malloc(sizeof(int));
        if (sock == NULL)
            print_err_exit("malloc err");
        *sock = connfd;
        pthread_mutex_lock(&mux);
        enqueue(sock);
        pthread_cond_signal(&condition_var);
        pthread_mutex_unlock(&mux);
    }

    close(listenfd);
    return 0;
}
