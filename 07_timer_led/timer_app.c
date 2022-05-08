#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define CLOSE_CMD           _IO(0xEF, 1)
#define OPEN_CMD            _IO(0xEF, 2)
#define SETCYCLE_CMD        _IOW(0xEF, 3, int)

int main(int argc, const char *argv[])
{
    int fd;
    unsigned int cmd;
    unsigned int arg;

    fd = open("/dev/LED",O_RDWR);
    if(fd<0){
        printf("device open error\n");
        return -1;
    }

    while(1){
        printf("input cmd:");
        scanf("%d",&cmd);
        //getchar();
        if(cmd == 1){
            cmd = CLOSE_CMD;

        }else if(cmd == 2){
            cmd = OPEN_CMD;

        }
        else if(cmd == 3){
            cmd = SETCYCLE_CMD;
            printf("input arg:");
            scanf("%d",&arg);
        //    getchar();
        }else{
            printf("cmd error\n");
            return -1;
        }
        ioctl(fd,cmd,&arg);
    }

    close(fd);
    if(fd<0){
        printf("device close error\n");
        return -1;
    }
    return 0;
}