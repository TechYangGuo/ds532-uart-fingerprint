#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DS532_IOC_MAGIC 'F'
#define DS532_IOC_INIT  _IO(DS532_IOC_MAGIC, 0)

int main() {
    int fd = open("/dev/ds532_fp", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    
    printf("Device opened successfully\n");
    
    int ret = ioctl(fd, DS532_IOC_INIT);
    printf("IOCTL returned: %d\n", ret);
    
    close(fd);
    printf("Device closed\n");
    return 0;
}