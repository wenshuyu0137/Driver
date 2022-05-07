#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

int main(int argc, const char *argv[])
{
    int fd,ret;
    int status;

    fd = open("/dev/MY_DEV",O_RDWR);
    if(fd<0){
        printf("device open error\n");
        return -1;
    }

    for(;;){
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
    

    close(fd);
    if(fd<0){
        printf("device close error\n");
        return -1;
    }
    return 0;
}