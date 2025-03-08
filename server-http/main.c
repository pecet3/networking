#include "../common/common.h"

#define SERVER_PORT 80
#define BUF_SIZE 4096

#define SA struct sockaddr

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
        printf("waiting for a connection on port %d\n", SERVER_PORT);

        fflush(stdout);
        connfd = accept(listenfd, (SA *)NULL, NULL);
        memset(buf, 0, BUF_SIZE);

        while ((n = read(connfd, recvline, BUF_SIZE - 1)) > 0)
        {
            fprintf(stdout, "\n%s\n\n%s", bin2hex(recvline, n), recvline);

            if (recvline[n - 1] == '\n')
                break;

            memset(recvline, 0, BUF_SIZE);
        }

        if (n < 0)
            print_err_exit("read err");

        snprintf((char *)buf, BUF_SIZE, "HTTP/1.0 200 OK\r\n\r\nHello");

        write(connfd, (char *)buf, strlen((char *)buf));
        close(connfd);
    }

    exit(0);
}
