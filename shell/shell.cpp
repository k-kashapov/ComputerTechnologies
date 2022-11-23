#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <malloc.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef D
#define DBG(expr)                                                              \
    expr;                                                                      \
    fflush(stdout);
#else
#define DBG(expr) ;
#endif

#define WRIT_PIPE_IDX 1
#define READ_PIPE_IDX 0

char *GetEffIDName(uid_t id) {
    struct passwd *id_pwd;

    id_pwd = getpwuid(id);
    if (!id_pwd) {
        return NULL;
    } else {
        return strdup(id_pwd->pw_name);
    }
}

int ExecToken(char *tok, int pipe0, int pipe1) {
    DBG(printf("PARENT: Executing %s with pipe %d -> %d\n", tok, pipe0, pipe1));

    pid_t pid = fork();

    if (pid) {
        // Parent

        DBG(printf("PARENT: Created child |%s| with pid (%d)\n", tok, pid));

        return 0;
    }

    char *args[2048];
    char **args_ptr = args;

    for (char *arg = strtok(tok, " "); arg; arg = strtok(NULL, " ")) {
        *args_ptr++ = arg;
    }

    *args_ptr = NULL;

    DBG(printf("CHILD: Split\n"));

    if (pipe0 != STDIN_FILENO) {
        int dup_res = dup2(pipe0, STDIN_FILENO);
        if (dup_res == -1) {
            perror("dup");
            return dup_res;
        }
        
        close(pipe0);
    }

    if (pipe1 != STDOUT_FILENO) {
        int dup_res = dup2(pipe1, STDOUT_FILENO);
        if (dup_res == -1) {
            perror("dup");
            return dup_res;
        }

        close(pipe1);
    }

    DBG(printf("CHILD: Execution ready ready\n"));

    fflush(stdout);

    execvp(args[0], args);

    printf("%s: %s\n", tok, strerror(errno));

    return -1;
}

int ParseLine(char *cmd) {
    char *cmds[2];

    cmds[0] = strtok(cmd, "|");
    cmds[1] = strtok(NULL, "|");

    if (!cmds[1]) {
        DBG(printf("PARENT: Single command!\n"));

        ExecToken(cmds[0], 0, 1);

        while (wait(NULL) > 0)
            ;

        return 0;
    }

    int pipe_fd[2];
    int last_out = -1;

    int pipe_res = pipe(pipe_fd);
    if (pipe_res) {
        perror("shell:");
        return pipe_res;
    }

    last_out = pipe_fd[0];

    DBG(printf("PARENT: Created pipe: [%d, %d]\n", pipe_fd[0], pipe_fd[1]));

    DBG(printf("PARENT: Execution ready\n"));
    ExecToken(cmds[0], 0, pipe_fd[1]);
    close(pipe_fd[1]);

    char *token;

    for (token = strtok(NULL, "|"); token; token = strtok(NULL, "|")) {
        pipe_res = pipe(pipe_fd);
        if (pipe_res) {
            perror("shell:");
            return pipe_res;
        }

        DBG(printf("PARENT: Created pipe: [%d, %d]\n", pipe_fd[0], pipe_fd[1]));
        
        ExecToken(cmds[1], last_out, pipe_fd[1]);
        close(pipe_fd[1]);

        last_out = pipe_fd[0];

        cmds[0] = cmds[1];
        cmds[1] = token;
    }
    
    ExecToken(cmds[1], last_out, 1);

    close(last_out);

    DBG(printf("PARENT: Finished executing line\n"))

    return 0;
}

int main(int argc, char *const *argv) {
    char command[4096] = {};
    int command_len = 0;

    char *name = GetEffIDName(getuid());
    if (!name) {
        printf("Unknown username!\n");
        return 0;
    }

    while (1) {
        printf("%s: $ ", name);

        int read_num = scanf(" %4095[^\n]%n", command, &command_len);

        DBG(printf("read num = %d\n", read_num));

        if (read_num == -1) {
            break;
        }

        DBG(printf("Input len = %d\n", command_len));

        if (command_len == 0) {
            continue;
        }

        DBG(printf("READ line: #%s#\n", command));

        if (strstr(command, "exit")) {
            break;
        }

        ParseLine(command);

        DBG(printf("Waiting to finish jobs...\n"));

        int pid;

        while ((pid = wait(NULL)) > 0) {
            DBG(printf("finished pid = %d\n", pid));
        }

        DBG(printf("All jobs finished!\n"));
    }

    free(name);

    return 0;
}
