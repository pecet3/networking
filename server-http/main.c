#include "../common/common.h"

#define SERVER_PORT 80
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

int main(int argc, char **argv)
{
    int listenfd, connfd, n;
    struct sockaddr_in servaddr;
    uint8_t buf[BUF_SIZE + 1];
    uint8_t recvline[BUF_SIZE];

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

    while (1)
    {
        printf("> waiting for a connection on port %d\n", SERVER_PORT);

        fflush(stdout);
        connfd = accept(listenfd, (SA *)NULL, NULL);
        memset(buf, 0, BUF_SIZE);

        while ((n = read(connfd, recvline, BUF_SIZE - 1)) > 0)
        {
            char *route = extract_routenmethod((const char *)(recvline));

            if (route != NULL)
            {
                printf("Extracted route: %s\n", route);
                free(route); // Zwolnienie pamięci
            }

            fprintf(stdout, "\n%s\n", recvline);

            if (recvline[n - 1] == '\n')
                break;

            memset(recvline, 0, BUF_SIZE);
        }

        if (n < 0)
            print_err_exit("read err");

        snprintf((char *)buf, BUF_SIZE, "HTTP/1.0 200 OK\r\n\r\n<html><b>test</b></html>");

        write(connfd, (char *)buf, strlen((char *)buf));
        close(connfd);
    }

    exit(0);
}
