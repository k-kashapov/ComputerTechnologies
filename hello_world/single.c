#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define CHECK_RET(expr)                                                  \
    do{                                                                  \
        int res = expr;                                                  \
        if (res < 0)                                                     \
        {                                                                \
            fprintf(stderr, "single: " #expr ": %s\n", strerror(errno)); \
            return -1;                                                   \
        }                                                                \
    } while(0);

#define SEM_KEY 0xC0DEDEAD

union semun {
    int val;               /* Value for SETVAL */
    struct semid_ds *buf;  /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* Array for GETALL, SETALL */
    struct seminfo *__buf; /* Buffer for IPC_INFO
                              (Linux-specific) */
};

int semopSingle(int semid, short unsigned int semn, short int op) {
    struct sembuf operation = { semn, op, SEM_UNDO };
    return semop(semid, &operation, 1);
}

int semZero(int semid, short unsigned int semn) {
    struct sembuf operation = { semn, 0, IPC_NOWAIT };
    int res = semop(semid, &operation, 1);

    if (res == -1) {
        if (errno == EAGAIN) {
            return 0;
        }
    }

    return 1;
}

int main(int argc, char const *argv[])
{
    (void) argc;
    (void) argv;

    const char hw[] = "Hello world\n"; 
    const char gw[] = "Goodbye world\n"; 

    const int hw_len = sizeof(hw) / sizeof(hw[0]);
    const int gw_len = sizeof(gw) / sizeof(gw[0]);

    int im_first = 0;
    int semid = semget(SEM_KEY, 1, IPC_CREAT | IPC_EXCL | 0600);

    if (semid >= 0)
    {
        // semaphore didn't exist
        im_first = 1;
    }
    else
    {
        semid = semget(SEM_KEY, 1, 0600);
     
        // printf("Raising semaphore\n");
        semopSingle(semid, 0, 1);
    }

    int len = hw_len;
    const char *str = hw;

    if (!im_first)
    {
        len = gw_len;
        str = gw;
    }

    for (int byte = 0; byte < len; byte++)
    {
        sleep(1);
        putc(str[byte], stdout);
        fflush(stdout);
    }

    if (semZero(semid, 0))
    {
        // printf("semaphore is zero\n");
        semctl(semid, 1, IPC_RMID, NULL);
    }

    return 0;
}
