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
    unsigned long value;

    fd = open("/dev/misc_beep",O_RDWR);
    if(fd<0){
        printf("device open error\n");
        return -1;
    }

    
    value = atoi(argv[1]);

    ret = write(fd,&value,sizeof(value));
    if(ret<0){
        printf("write fail\n");
        return -1;
    }

    close(fd);
    if(fd<0){
        printf("device close error\n");
        return -1;
    }
    return 0;
}