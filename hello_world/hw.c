#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define CHECK_RET(expr)                                                  \
    do{                                                                  \
        int res = expr;                                                  \
        if (res < 0)                                                     \
        {                                                                \
            fprintf(stderr, "single: " #expr ": %s\n", strerror(errno)); \
            return -1;                                                   \
        }                                                                \
    } while(0);

int main(int argc, char const *argv[])
{
    (void) argc;
    (void) argv;

    const char hw[] = "Hello world\n"; 
    const char gw[] = "Goodbye world\n"; 

    const int hw_len = sizeof(hw) / sizeof(hw[0]);
    const int gw_len = sizeof(gw) / sizeof(gw[0]);

    /*
        /proc/[pid]/comm
    */

    DIR *proc_dir = opendir("/proc");

    if (!proc_dir) {
        fprintf(stderr, "single: opendir: %s\n", strerror(errno));
        return -1;
    }

    int my_pid = getpid();

    char my_comm_path[256 + 7 + 5];
    sprintf(my_comm_path, "/proc/%d/comm", my_pid);

    char my_comm[256];

    int my_comm_fd = open (my_comm_path, O_RDONLY);
    CHECK_RET(read (my_comm_fd, my_comm, 256));

    char full_path[256 + 15] = "/proc/";
    char comm_buffer[256];

    int already_started = 0;

    for (struct dirent *dir = readdir(proc_dir); dir; dir = readdir(proc_dir)) {
        if (dir->d_type != DT_DIR)
        {
            continue;
        }

        int pid = atoi(dir->d_name);

        if (!pid || pid == my_pid)
        {
            continue;
        }

        // printf("name = |%s|\n", dir->d_name);

        sprintf(full_path + 5, "/%s/comm", dir->d_name);

        // printf("full_path = |%s|\n", full_path);

        int cmd_fd = open(full_path, O_RDONLY);

        if (cmd_fd < 0)
        {
            fprintf(stderr, "single: open: %s\n", strerror(errno));
            return -1;   
        }

        int read_bytes = read (cmd_fd, comm_buffer, 256);

        if (read_bytes < 0)
        {
            fprintf(stderr, "single: read: %s\n", strerror(errno));
            return -1;   
        }

        // printf("read /proc/%s/comm = |%.*s|\n", dir->d_name, read_bytes - 1, comm_buffer);

        if (!strncmp(my_comm, comm_buffer, read_bytes - 1))
        {
            // printf ("Found self\n");
            already_started = 1;
            break;
        }

        CHECK_RET(close(cmd_fd));
    }

    int len = hw_len;
    const char *str = hw;

    if (already_started)
    {
        len = gw_len;
        str = gw;
    }

    for (int byte = 0; byte < len; byte++)
    {
        putc(str[byte], stdout);
        fflush(stdout);
        sleep(1);
    }

    CHECK_RET(closedir(proc_dir));

    return 0;
}
