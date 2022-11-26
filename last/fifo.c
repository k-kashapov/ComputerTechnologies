#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Artificially shrink the buffer to check the multithreading
static const int BUFF_SIZE = 16;

ssize_t safe_write(int fd_dest, const char *src, size_t num) {
    ssize_t bytes_written = 0;

    while (bytes_written < num) {
        ssize_t write_ret =
            write(fd_dest, src + bytes_written, num - bytes_written);
        if (write_ret < 0) {
            return write_ret;
        }

        bytes_written += write_ret;
    }

    return bytes_written;
}

ssize_t mycat(int fd_src, int fd_dest) {
    char buff[BUFF_SIZE];

    while (1) {
        ssize_t read_ret = read(fd_src, buff, BUFF_SIZE);
        if (read_ret < 0) {
            perror("Error reading from file descriptor");
            return read_ret;
        }

        if (read_ret == 0) {
            break;
        }

        ssize_t write_ret = safe_write(fd_dest, buff, read_ret);
        if (write_ret < 0) {
            perror("Error writing to file descriptor");
            return write_ret;
        }
    }

    return 0;
}

int usage(void) {
    printf("Usage: fifo.elf [FILE]...\n");

    return 0;
}

void *reader(void *attr) {


    return NULL;
}

static const int FIFO_NUM = 16;

int main(int argc, const char **argv) {
    if (argc < 2) {
        return usage();
    }

    int fifo_ret = mkfifo("FIFO", O_EXCL | O_RDWR);
    if (fifo_ret < 0 && errno != EEXIST) {
        perror("mkfifo");
        return -1;
    }

    pthread_t read_id;

    int pthr = pthread_create(&read_id, NULL, reader, NULL);
    if (pthr) {
        perror("pthread");
        return pthr;
    }

    for (int iter = 1; iter < argc; iter++) {
        int curr_fd = open(argv[iter], O_RDONLY);

        printf("FILE: %s\n", argv[iter]);

        if (curr_fd < 0) {
            fprintf(stderr, "fifo: %s: %s\n", argv[iter], strerror(errno));
            return curr_fd;
        }

        ssize_t cat_val = mycat(curr_fd, 1);
        close(curr_fd);

        if (cat_val < 0) {
            return cat_val;
        }
    }

    return 0;
}
