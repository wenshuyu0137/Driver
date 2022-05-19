#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

static int get_data(const char *file,char *databuf)
{
    int ret;
    FILE *fd;
    fd = fopen(file,"r");
    if(fd == NULL){
        printf("device open error\n");
        return -1;
    }
    ret = fread(databuf,1,50,fd);
    
    fclose(fd);
    if(fd<0){
        printf("device close error\n");
        return -1;
    }
    return ret;
}
int main(int argc, const char *argv[])
{
    int ret,i;
    char databuf[50];
    unsigned short ir, als, ps;

    char *device[3] = {"/sys/bus/iio/devices/iio:device1/in_illuminance_raw",
                                "/sys/bus/iio/devices/iio:device1/in_intensity_raw",
                                "/sys/bus/iio/devices/iio:device1/in_proximity_raw"
                            };
    while(1){
        for(i=0;i<3;i++){
            get_data(device[i],databuf);
            printf("%s = %d\n", device[i],atoi(databuf));
        }
        sleep(1);
    }
    return 0;
}
