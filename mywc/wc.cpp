#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

// parent         child
// pipe
// fork
//                dup
//                close
//                exec
// close
// read
// count
// wait

#define WRIT_PIPE_IDX 1
#define READ_PIPE_IDX 0
#define BUFF_SIZE     10
#define FIELD_WIDTH   "8"

int main (int argc, char *const *argv)
{
    int pipefd[2];

    int pipe_res = pipe (pipefd);
    if (pipe_res)
    {
        perror ("my_wc:");
        return pipe_res;
    }

    pid_t pid = fork();

    if (!pid)
    {
        // Child
        int dup_res = dup2 (pipefd[WRIT_PIPE_IDX], 1);
        if (dup_res == -1)
        {
            perror ("dup:");
            return dup_res;
        }

        close (pipefd[READ_PIPE_IDX]);

        execvp (argv[1], argv + 1);
    }
    
    close (pipefd[WRIT_PIPE_IDX]);

    char pipe_buf[BUFF_SIZE];
    int read_ret;
    
    int bytes_num = 0,
        words_num = 0,
        lines_num = 0;

    char not_word = 1;

    do {
        read_ret = read (pipefd[READ_PIPE_IDX], 
                         pipe_buf, 
                         BUFF_SIZE);
        if (read_ret < 1) break;

        bytes_num += read_ret;

        for (int byte = 0; byte < read_ret; byte++)
        {
            #ifdef _DEBUG
            printf ("%d: \'%c\' (%d)\n", 
                     byte, 
                     pipe_buf[byte], 
                     pipe_buf[byte]);
            #endif //_DEBUG

            lines_num += (pipe_buf[byte] == '\n');

            if (isspace (pipe_buf[byte]))
            {
                not_word = 1;
            }
            else
            {
                words_num += not_word;
                not_word = 0;
            }
        }
    } while (1);

    printf ("bytes: %" FIELD_WIDTH "d\n"
            "lines: %" FIELD_WIDTH "d\n"
            "words: %" FIELD_WIDTH "d\n", 
             bytes_num,
             lines_num,
             words_num);

    return 0;
}