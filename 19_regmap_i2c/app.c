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
    unsigned short databuf[3];
    unsigned short ir, als, ps;

    fd = open("/dev/AP3216C",O_RDWR);
    if(fd<0){
        printf("device open error\n");
        return -1;
    }

    while(1){
        ret = read(fd, databuf, sizeof(databuf));
        if(ret == 0) { /* 数据读取成功 */
            ir = databuf[0]; /* ir 传感器数据 */
            als = databuf[1]; /* als 传感器数据 */
            ps = databuf[2]; /* ps 传感器数据 */
            printf("ir = %d, als = %d, ps = %d\r\n", ir, als, ps);
        }
        usleep(200000); /*100ms */
    }

    close(fd);
    if(fd<0){
        printf("device close error\n");
        return -1;
    }
    return 0;
}