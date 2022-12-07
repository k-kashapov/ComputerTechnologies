#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

void usr_handler(int arg)
{
    printf("zalupa\n");
    printf("%s\n", arg == SIGUSR1 ? "JOJO" : "DIO");
}

int main (int argc, const char **argv)
{
    (void) argc;
    (void) argv;

    struct sigaction act = 
        { 
            .sa_handler = usr_handler, 
            .sa_mask    = {}, 
            .sa_flags   = 0, 
            NULL 
        };

    struct sigaction old;

    int sigres = sigaction(SIGUSR1, &act, &old);

    if (sigres)
    {
        perror("sigaction");
        return 0;
    }

    int pid = fork();

    if (!pid)
    {
        // Child

        printf("zalupa 0\n");

        sigset_t set = {};

        sigfillset(&set);
        sigdelset(&set, SIGUSR1);
        sigprocmask(SIG_BLOCK, &set, NULL);

        printf("zalupa 1\n");

        sigsuspend(&set);

        printf("zalupa 2\n");

        return 0;
    }

    sleep(1);

    kill (pid, SIGUSR1);

    while(wait(NULL) > 0) ;

    return 0;
}
