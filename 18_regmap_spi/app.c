#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

static unsigned char data_to_hex(unsigned char data)
{   
    int i;
    unsigned char hex_data[16] = {
    [0] = 0x3F,
    [1] = 0x06,
    [2] = 0x5B,
    [3] = 0x4F,
    [4] = 0x66,
    [5] = 0x6D,
    [6] = 0x7D,
    [7] = 0x07,
    [8] = 0x7F,
    [9] = 0x6F,
    [10] = 0x77,
    [11] = 0x7C,
    [12] = 0x39,
    [13] = 0x5E,
    [14] = 0x79,
    [15] = 0x71,
    };
    if(data >= 48 && data <= 57){

        return hex_data[(data-48)];

    }else if(data >= 97 && data <= 102){

        return hex_data[(data-97+10)];

    }else if(data >= 65 && data <= 70){

        return hex_data[(data-65+10)];

    }else{
        printf("the args must be 1-9 or a-f or A-F\n");
        return -1;
    }
}

int main(int argc, const char *argv[])
{
    int fd,ret,len,i;
    char tmp;
    unsigned char res_data[8];
    unsigned char write_data[4];
    unsigned char read_data[4];
    
    if(argc <= 1 || argc >2 || strlen(argv[1])>4){
        printf("argc fail\n");
        return -1;
    }

    len = strlen(argv[1]);

    for(i=0;i<len;i++){
        tmp = argv[1][i];
        if(!((tmp >= 48 && tmp <= 57) || (tmp >= 97 && tmp <= 102) || (tmp >= 65 && tmp <= 70))){
            printf("args must be 1-9 or a-f or A-F\n");
            printf("please try agin\n");
            return -1;
        }
    }

    for(i=0;i<len;i++){
        write_data[i] = data_to_hex(argv[1][i]);
    }

    for(i=len;i<4;i++){
        write_data[i] = 0x3F;
    }
    fd = open("/dev/m74hc595",O_RDWR);
    if(fd<0){
        printf("device open error\n");
        return -1;
    }

    while(1){
        static int i = 0;
        ret = write(fd,write_data,sizeof(write_data));
        if(ret<0){
            printf("write data fail ret  = %d\n",ret);
            return -1;
        }
        if(i==0){
            printf("write_data = %#x %#x %#x %#x\n",write_data[0],write_data[1],write_data[2],write_data[3]);
        }
        i++;
    }

    close(fd);
    if(fd<0){
        printf("device close error\n");
        return -1;
    }
    
    return 0;
}