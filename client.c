#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/un.h>
#include <stddef.h>

#define CLI_PATH "/var/tmp/"
#define CLI_PERM S_IRWXU

int cli_conn(const char *name)
{
    int fd, len, err, rval;
    struct sockaddr_un un;
    
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return -1;
    
    //填充客户端地址
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    sprintf(un.sun_path, "%s%05d", CLI_PATH, getpid()); 
    len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
    unlink(un.sun_path);
    //绑定到套接字
    if (bind(fd, (struct sockaddr *)&un, len) < 0) {
        rval = -2;
        goto errout;
    }
    
    if (chmod(un.sun_path, CLI_PERM) < 0) {
        rval = -3;
        goto errout;
    }
    
    //填充服务端地址
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name); 
    len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
    if (connect(fd, (struct sockaddr *)&un, len) < 0) {
        rval = -4;
        goto errout;
    }
    char *send_buf = "i am client";
    int w_len =  write(fd, send_buf, strlen(send_buf) + 1);
    printf("client write len %d\n", w_len);
    return(fd);
    
errout:
    err = errno;
    close(fd);
    errno = err;
    return(rval);
}

int main(void)
{
    int ret = cli_conn("/var/tmp/hello");
    printf("clien to server ret %d\n", ret);

    exit(0);
}

