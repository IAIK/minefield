#include "logging.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace logging {

static int fd;

void open(char const *ip, int port) {
    struct sockaddr_in server_adr {};

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if ( fd == -1 ) {
        perror("LOGGING: cannot open socket");
        return;
    }

    server_adr.sin_family      = AF_INET;
    server_adr.sin_addr.s_addr = inet_addr(ip);
    server_adr.sin_port        = htons(port);

    int con_fd = connect(fd, (struct sockaddr *)&server_adr, sizeof(struct sockaddr));

    if ( con_fd == -1 ) {
        perror("LOGGING: cannot connect");
        ::close(fd);
        fd = -1;
        return;
    }

    // dup2(fd, STDOUT_FILENO);
}

void close() {
    if ( fd != -1 ) {
        ::close(fd);
    }
}

void write(const char *format, ...) {
    char    buf[1000] = { '\0' };
    va_list ap;

    va_start(ap, format);
    vsnprintf(buf, BUFSIZ, format, ap);
    va_end(ap);

    if ( fd != -1 ) {
        send(fd, buf, sizeof(buf), 0);
    }
    printf("%s", buf);
    fflush(stdout);
}

} // namespace logging