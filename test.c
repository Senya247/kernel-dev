#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_WRT_SZE 256

const char *dev_name = "/dev/ssa1";

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "The second argument must be r(read) or w(write)\n");
        return 1;
    }
    int ret = 0;
    char *buffer = NULL;

    int fd = open(dev_name, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "Error in opening %s\n", dev_name);
        goto fin;
    }

    size_t num = 64, copied;
    switch (argv[1][0]) {
    case 'r':
        if (argc >= 3) {
            if (!(num = atoi(argv[3]))) {
                fprintf(stderr, "Invalid size\n");
                goto fin;
            }
        }
        buffer = malloc(num * sizeof(char));
        copied = read(fd, buffer, num);

        fwrite(buffer, 1, num, stdout);
        fprintf(stdout, "\nread %ld bytes\n", copied);
        ret = 0;
        break;
    case 'w':
        if (argc < 3) {
            fprintf(stderr, "Provide buffer to write as argument 3\n");
            goto fin;
        }
        num = strnlen(argv[2], MAX_WRT_SZE);
        buffer = malloc(num);

        memcpy(buffer, argv[2], num);
        copied = write(fd, buffer, num);

        fprintf(stdout, "written %ld bytes\n", copied);
        ret = 0;
        break;
    default:
        fprintf(stderr, "Invalid operation\n");
    }
fin:
    free(buffer);
    return ret;
}
