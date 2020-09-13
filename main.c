#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <stddef.h>

#define QLEN 10
#define STALE 30

//创建服务端，成功返回fd，错误返回值<0
int serv_listen(const char *name)
{
    int fd,err,rval, len;
    struct sockaddr_un un;
    
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return -1;
    unlink(name);  //存在文件，先解除连接
    
    //填充socket地址结构
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);
    len = sizeof(un);

    //绑定地址到描述符
    if (bind(fd, (struct sockaddr *)&un, len) < 0) {
        rval = -2;
        goto errout;
    }
    
    if(listen(fd, QLEN) < 0) {
        rval = -3;
        goto errout;
    }
    return(fd);
    
    errout:
        err = errno;
        close(fd);
        errno = err;
        return(rval);
}

//等待客户连接，并接受它
//同时验证客户的身份
int serv_accept(int listenfd, uid_t *uidptr)
{
    int clifd, rval, err;
    socklen_t len;
    struct sockaddr_un un;
    struct stat statbuf;
    time_t staletime;
    
    len = sizeof(un);
    if ((clifd = accept(listenfd, (struct sockaddr *)&un, &len)) < 0) 
        return -1;
    
    char r_buf[64] = {0};
    int r_n = read(clifd, r_buf, sizeof(r_buf) - 1);
    printf("server read num %d content: %s\n", r_n, r_buf);
    //确定客户进程的身份是该套接字的所有者
    len -= offsetof(struct sockaddr_un, sun_path); //路径长
    un.sun_path[len]=0; //增加\0结束符
    
    if (stat(un.sun_path, &statbuf) < 0) {
        rval = -2;
        goto errout;
    }
    
    // 文件类型检查
    if (S_ISSOCK(statbuf.st_mode)==0) {
        rval = -3;
        goto errout;
    }
    
    // 文件权限检查
    if((statbuf.st_mode & (S_IRWXG | S_IRWXO)) ||
        statbuf.st_mode & S_IRWXU != S_IRWXU) {
        rval = -4;
        goto errout;
    }
    
    staletime = time(NULL) - STALE;
    if (statbuf.st_atime < staletime ||
        statbuf.st_ctime < staletime ||
        statbuf.st_mtime < staletime) {
        rval = -5;
        goto errout;
    }
    
    if (uidptr != NULL)
        *uidptr = statbuf.st_uid; //返回uid
    unlink(un.sun_path);
    return(clifd);
    
    errout:
        err = errno;
        close(clifd);
        errno = err;
        return rval;
}

int main(void)
{
    int fd = serv_listen("/var/tmp/hello");
    printf("server listen fd %d\n", fd);

    uid_t client_id;
    int ret = serv_accept(fd, &client_id);
    printf("accpet return %d\n", ret);

    return ret;

}

