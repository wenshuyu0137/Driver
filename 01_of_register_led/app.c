#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

int main(int argc, const char* argv[])
{
    int fd,retval;
    unsigned char databuff[1];

    /*open a device*/
    fd = open("/dev/my_led",O_RDWR);
    if(fd<0){
        printf("device open fail\n");
        return -1;
    }

    /*get the led status*/
    databuff[0] = atoi(argv[1]);
    printf("databuff[0] = %d\n",databuff[0]);

    /*write to device*/
    retval = write(fd,databuff,sizeof(databuff));
    if(retval<0){
        printf("write fail\n");
        close(fd);
        return -1;
    }

    /*close device*/
    retval = close(fd);
    if(retval<0){
        printf("device close fail\n");
        close(fd);
        return -1;
    }

    return 0;
}