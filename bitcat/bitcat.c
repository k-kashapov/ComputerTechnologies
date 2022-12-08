#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define CHECK_RET(expr)                                                        \
    do {                                                                       \
        int res##__LINE__ = (expr);                                            \
        if (res##__LINE__ < 0) {                                               \
            perror(#expr);                                                     \
            return res##__LINE__;                                              \
        }                                                                      \
    } while (0);

static uint8_t curr_byte = 0;
static uint8_t transmission_end = 0;

void getter_handler(int arg) {
    static int bit_count = 0;

    // printf("CHILD: signal %d, bit = %d\n", arg, arg == SIGUSR1);

    curr_byte <<= 1;
    curr_byte |= (arg == SIGUSR1);
    bit_count++;

    if (bit_count > 7)
    {
        if (curr_byte == 0)
        {
            transmission_end = 1;
            return;
        }

        // printf("byte = [%c] (0x%x)\n", curr_byte, curr_byte);
        putc(curr_byte, stdout);
        // putc('\n', stdout);
        curr_byte = 0;
        bit_count = 0;
    }
}

void setter_handler(int arg) {
    // printf("PARENT: signal %d\n", arg);
}

int main(int argc, const char **argv) {
    (void)argc;
    (void)argv;

    struct sigaction act = {
        .sa_handler = getter_handler, .sa_mask = {}, .sa_flags = 0, NULL};

    struct sigaction old;

    sigset_t set = {};

    CHECK_RET(sigfillset(&set));
    CHECK_RET(sigdelset(&set, SIGINT));
    CHECK_RET(sigdelset(&set, SIGQUIT));

    CHECK_RET(sigprocmask(SIG_BLOCK, &set, NULL));

    CHECK_RET(sigdelset(&set, SIGUSR1));
    CHECK_RET(sigdelset(&set, SIGUSR2));

    int parent_id = getpid();
    int pid = fork();

    if (!pid) {
        // Child

        CHECK_RET(sigaction(SIGUSR1, &act, &old));
        CHECK_RET(sigaction(SIGUSR2, &act, &old));

        while (!transmission_end)
        {
            // printf("CHILD: recieving...\n");

            sigsuspend(&set);

            if (errno == EFAULT) {
                fprintf(stderr, "sigsuspend: %s\n", strerror(errno));
                exit(-1);
            }

            // printf("CHILD: sending...\n");
            kill(parent_id, SIGUSR1);
        }
        return 0;
    }
    else
    {
        // Parent

        act.sa_handler = setter_handler;

        CHECK_RET(sigaction(SIGUSR1, &act, &old));

        char *str = "JOJO";
        char *str_iter = str;
        char curr_byte = *str_iter;

        do {
            curr_byte = *str_iter;

            for (int bit = 7; bit >= 0; bit--)
            {
                uint8_t curr_bit = (curr_byte >> bit) & 1;
                // printf("PARENT: parent ready\n");
                // printf("PARENT: sending %d...\n", curr_bit);
                kill(pid, curr_bit ? SIGUSR1 : SIGUSR2);

                // printf("PARENT: waiting for responce...\n");
                sigsuspend(&set);

                // sleep(1);   
            }

        } while(*str_iter++);

        while (wait(NULL) > 0)
                ;
    }
    return 0;
}
