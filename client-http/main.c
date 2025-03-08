#include "../common/common.h"

#define SERVER_PORT 80
#define BUF_SIZE 4096

#define SA struct sockaddr

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        print_err_exit("no enough args!\nusage: %s <server address>", argv[0]);
    }
    int sockfd, n;
    int sendbytes;
    struct sockaddr_in servaddr;
    char sendline[BUF_SIZE];
    char recvline[BUF_SIZE];

    // create a socket
    // AF_INET - internet socket, SOCK_STREAM - stream socket, 0 - tcp/ip
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        print_err_exit("error during creating socket");

    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);

    // converting string ip to bytes
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        print_err_exit("error during conveerting ip addr");

    // connecting to the server
    if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) < 0)
        print_err_exit("error during connecting to the server");

    // connected, preparing a http header
    sprintf(sendline, "GET / HTTP/1.1\r\nHost: %s\r\n\r\n", argv[1]);
    sendbytes = strlen(sendline);

    // write to socket

    if (write(sockfd, sendline, sendbytes) != sendbytes)
        print_err_exit("write error");

    memset(recvline, 0, BUF_SIZE);

    // read server response

    while ((n = read(sockfd, recvline, BUF_SIZE - 1)) > 0)
    {
        printf("%s", recvline);
    }
    printf("SERVER RESPONSE:\n\n%s", recvline);
    if (n < 0)
    {
        print_err_exit("undefined error");
    }
    else if (n == 0)
    {
        printf("Server closed the connection.\n");
    }
    close(sockfd);
    exit(0);
}
