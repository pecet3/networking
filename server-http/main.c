#include "../common/common.h"
#define SERVER_PORT 8080
#define BUF_SIZE 4096

#define SA struct sockaddr

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

    strncpy(route, input, length);
    route[length + 1] = '\0';
    return route;
}

void handle_connection(int listenfd)
{
    int connfd;
    uint8_t buf[BUF_SIZE + 1];
    uint8_t recvline[BUF_SIZE];

    int n;
    while (1)
    {
        struct sockaddr_in addr;
        socklen_t addr_len;
        char client_address[BUF_SIZE + 1];
        connfd = accept(listenfd, (SA *)&addr, &addr_len);
        inet_ntop(AF_INET, &addr.sin_addr, client_address, BUF_SIZE);

        printf("> client connected, ip: %s\n", client_address);
        fflush(stdout);

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
                if (!strcmp(route, "GET /test"))
                {
                    snprintf((char *)buf, BUF_SIZE, "HTTP/1.0 200 OK\r\n\r\n<html><b>test</b></html>");
                    write(connfd, (char *)buf, strlen((char *)buf));
                }
                if (!strcmp(route, "GET /test2"))
                {
                    snprintf((char *)buf, BUF_SIZE, "HTTP/1.0 200 OK\r\n\r\n<html><b>test2</b></html>");
                    write(connfd, (char *)buf, strlen((char *)buf));
                }
                if (!strncmp(route, "GET /file", 9))
                // strstr - alternative
                {
                    const char *path = route + 10;
                    printf("path: %s\n", path);

                    FILE *fp = fopen(path, "r");
                    if (fp == NULL)
                    {
                        snprintf((char *)buf, BUF_SIZE, "HTTP/1.0 404 Not Found\r\n\r\n<html><b>File not found</b></html>");
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

            if (recvline[n - 1] == '\n')
                break;
            free(route);

            memset(recvline, 0, BUF_SIZE);
        }

        if (n < 0)
            print_err_exit("read err");

        close(connfd);
    }
}

int main(int argc, char **argv)
{
    int listenfd;
    struct sockaddr_in servaddr;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        print_err_exit("socket err");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    if ((bind(listenfd, (SA *)&servaddr, sizeof(servaddr))) < 0)
        print_err_exit("bind err");

    if ((listen(listenfd, 10)) < 0)
        print_err_exit("listen err");

    handle_connection(listenfd);
    pthread_t thread;

    exit(0);
}
