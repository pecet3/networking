#include "common.h"

void print_err_exit(const char *fmt, ...)
{
    int errno_save = errno;
    va_list ap;

    // print to stdout
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    fflush(stdout);

    // print out err msg if errno was set

    if (errno_save != 0)
    {
        fprintf(stdout, "(errno = %d) : %s\n", errno_save, strerror(errno_save));
        strerror(errno_save);
        fprintf(stdout, "\n");
        fflush(stdout);
    }
    va_end(ap);

    exit(1);
}

int setup_srv(int port, int backlog)
{
    int listenfd;

    struct sockaddr_in servaddr;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        print_err_exit("socket err");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if ((bind(listenfd, &servaddr, sizeof(servaddr))) < 0)
        print_err_exit("bind err");

    if ((listen(listenfd, 1)) < 0)
        print_err_exit("listen err");

    return listenfd;
}

int accept_conn(int srv_socket)
{
    struct sockaddr_in addr;
    socklen_t addr_len;
    char client_address[BUF_SIZE + 1];
    int client_socket = accept(srv_socket, &addr, &addr_len);
    inet_ntop(AF_INET, &addr.sin_addr, client_address, BUF_SIZE);

    return client_socket;
}
