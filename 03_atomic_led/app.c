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
    unsigned char databuf[1];

    fd = open("/dev/ato_led",O_RDWR);
    if(fd<0){
        printf("device open error\n");
        return -1;
    }

    databuf[0] = atoi(argv[1]);
    ret = write(fd,databuf,sizeof(databuf));
    if(ret<0){
        printf("fail write argv\n");
        close(fd);
        return -1;
    }

    printf("app is running\n");
    sleep(25);
    

    printf("app running finish\n");

    close(fd);
    if(fd<0){
        printf("device close error\n");
        return -1;
    }
    return 0;
}