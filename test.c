#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define BUF_SZE 2048
#define RD_SZE 200

const char *dev_name = "/dev/koolchardrv0";

int main(int argc, char *argv[]) {
    char buff[BUF_SZE];
    int fd;
    ssize_t num_read;

    fd = open(dev_name, O_RDONLY);
    num_read = read(fd, buff, RD_SZE);

    buff[num_read] = '\0';
    printf("%s\nnum_read: %ld\n", buff, num_read);
}
