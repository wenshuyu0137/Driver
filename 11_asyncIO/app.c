#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <poll.h>
#include <signal.h>


static int fd;
unsigned char status;

static void async_func(int arg)
{
    int ret;
    unsigned char status;
    read(fd,&status,sizeof(status));

    if(status == 0){
        printf("key press\n");
    }
    if(status == 1){
        printf("key release\n");
    }
}
int main(int argc, const char *argv[])
{
    //int ret;
    int flags = 0;
    //struct pollfd fds;


    fd = open("/dev/MY_DEV",O_RDONLY);
    if(fd<0){
        printf("device open error\n");
        return -1;
    }
    printf("aaa\n");
/*
    fds.fd = fd;
    fds.events = POLLIN;
    ret =  poll(&fds,fd,500);
    switch(ret){
        case 0:
            printf("timeout\n");
        case -1:
            printf("error\n");
        default:
            read(fd,&status,sizeof(status));
            if(status == 0){
                printf("key press\n");
            }
            if(status == 1){
                printf("key release\n");
            }
        break;
    }
*/

    signal(SIGIO,async_func);
    fcntl(fd,F_SETOWN,getpid());
    flags = fcntl(fd, F_GETFD);
    fcntl(fd, F_SETFL, flags | FASYNC);

/*
    while(1){
        read(fd,&status,sizeof(status));

        if(status == 0){
            printf("key press\n");
        }
        if(status == 1){
            printf("key release\n");
        }
    }
*/

    for(;;);

    close(fd);
    if(fd<0){
        printf("device close error\n");
        return -1;
    }
    return 0;
}