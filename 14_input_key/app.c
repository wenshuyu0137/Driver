#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/input.h>

int main(int argc, const char *argv[])
{
    int fd,ret;
    unsigned char databuf[1];
    struct input_event key_input;

    fd = open("/dev/input/event0",O_RDWR);
    if(fd<0){
        printf("device open error\n");
        return -1;
    }

    while(1){
        ret = read(fd,&key_input,sizeof(struct input_event));
        if(ret<0){
            printf("read status fail\n");
        }
        switch(key_input.type){
            case EV_KEY:
                if(key_input.code == KEY_0){
                    if(key_input.value == 0){
                        printf("key press\n");
                    }else if(key_input.value == 1){
                        printf("key release\n");
                    }
                }               
            default:
                break;
        }
    }

    close(fd);
    if(fd<0){
        printf("device close error\n");
        return -1;
    }
    return 0;
}