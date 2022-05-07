#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <poll.h>
#include <signal.h>


int main(int argc, const char *argv[])
{
    int fd,ret;
    int status;
//  fd_set read_set;
    int timeout = 500;
    struct pollfd fds;

    fd = open("/dev/MY_DEV",O_RDWR|O_NONBLOCK);
    if(fd<0){
        printf("device open error\n");
        return -1;
    }
    for(;;){
        fds.fd = fd;
        fds.events = POLLIN;
        ret  = poll(&fds, 1, timeout);
        switch(ret){
            case 0:
                printf("time out\n");
                return -1;
            case -1:
                printf("error\n");
                return -1;
            default:
                if(fds.revents | POLLIN){
                    ret = read(fd,&status,sizeof(int));
                    if(ret<0){
                        printf("can't read\n");;
                        return -1;
                    }
                    if(status == 0){
                        printf("can read\n");
                        printf("KEY PRESS\n");
                    }else if(status == 1){
                        printf("can read\n");
                        printf("KEY RELEASE\n");
                    }
                }
            break;
        }
    } 

#if 0
    for(;;){
        FD_ZERO(&read_set);
        FD_SET(fd, &read_set);
        timeout.tv_sec = 0.5;

        ret  = select (fd+1, &read_set,NULL,NULL,&timeout);
        switch(ret){
            case 0:
                printf("time out\n");
                return -1;
            case -1:
                printf("error\n");
                return -1;
            default:
                if(FD_ISSET(fd,&read_set)){
                    printf("can read\n");
                    ret = read(fd,&status,sizeof(int));
                    if(ret<0){
                        return -1;
                    }
                    if(status == 0){
                        printf("KEY PRESS\n");
                    }else if(status == 1){
                        printf("KEY RELEASE\n");
                    }
                }
            break;
        }
    }  
#endif

    close(fd);
    if(fd<0){
        printf("device close error\n");
        return -1;
    }
    return 0;
}