#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>          /* Definition of O_* and S_* constants */

#ifdef _DEBUG
#define DEBUG_(expr) { expr; }
#else
#define DEBUG_(expr) ;
#endif // _DEBUG

struct CpOpt_t
{
    unsigned force       : 1;
    unsigned interactive : 1;

    const char **files_list;
    int          files_num;
};

CpOpt_t ParseCpArgs (int argc, char *const *argv)
{
    const struct option options[] = 
    {
        {
            .name    = "interactive", 
            .has_arg = no_argument,
            .flag    = NULL,
            .val     = 'i'
        },

        {
            .name    = "verbose", 
            .has_arg = no_argument,
            .flag    = NULL,
            .val     = 'v'
        },
        {
            .name    = "force", 
            .has_arg = no_argument,
            .flag    = NULL,
            .val     = 'f'
        },

        { 0, 0, 0, 0 }
    };

    CpOpt_t cp_opt  = {};
    int     opt_ret = 0;

    cp_opt.files_list = (const char**) malloc (argc * sizeof (char *));

    while (opt_ret != -1)
    {
        opt_ret = getopt_long
                  (
                    argc, argv,
                    "-i-v-f--interactive--verbose--force",
                    options, NULL
                  );

        DEBUG_
        (
            printf ("\nopt_ret = %c (%d)\n", opt_ret, opt_ret);
        );

        switch (opt_ret)
        {
            case 'v':
                printf ("cp_my version 1.0\n");
                break;

            case 'f':
                DEBUG_ (printf ("force\n"));

                cp_opt.force = 1;
                break;

            case 'i':
                DEBUG_ (printf ("interactive\n"));                
                
                cp_opt.interactive = 1;
                break;

            default:
                DEBUG_ (printf ("default: %s\n", argv[optind - 1]));

                cp_opt.files_list[cp_opt.files_num++] = argv[optind - 1];
                break;
        }
    }

    cp_opt.files_num--;
    
    return cp_opt;
}

ssize_t safe_write (int fd_dest, const char *src, ssize_t num)
{
    ssize_t bytes_written = 0;

    while (bytes_written < num)
    {
        ssize_t write_ret = write (fd_dest, 
                                   src + bytes_written,
                                   num - bytes_written);
        if (write_ret < 0)
        {
            return write_ret;
        }

        bytes_written += write_ret;
    }

    return bytes_written;
}

ssize_t mycat (int fd_src, int fd_dest)
{
    const int BUFF_SIZE = 4096;
    char buff[BUFF_SIZE];

    while (true)
    {
        ssize_t read_ret = read (fd_src, buff, BUFF_SIZE);
        if (read_ret < 0)
        {
            perror ("Error reading from file descriptor");
            return read_ret;
        }

        if (read_ret == 0)
        {
            break;
        }

        ssize_t write_ret = safe_write (fd_dest, buff, read_ret);   
        if (write_ret < 0)
        {
            perror ("Error writing to file descriptor");
            return write_ret;
        }
    }

    return 0;
}

#define IS_REG_MASK 1
#define EXISTS_MASK 2

// Check if file is regular or does not exist
uint8_t IsNotDir (const char *file_name)
{
    struct stat last_file;
    int stat_ret = stat (file_name, &last_file);

    uint8_t status = S_ISREG (last_file.st_mode);
    
    status |= ((int) (stat_ret != -1)) << 1;

    return status;
}

bool CheckOverwrite (uint8_t status, const char *name)
{
    if (status & EXISTS_MASK)
    {
        DEBUG_
        (
            printf ("File Extsts\n");
        )

        printf ("cp_my: overwrite \'%s\'? ", name);
        
        int ans = getchar(); 
        
        if (ans != 'y' && ans != '\n')
        {
            return 0;
        }
    }

    return 1;
}

static const mode_t All_Permissions = S_IRWXU | S_IRWXG | S_IRWXO;

ssize_t CP_file (const char *src, const char *dest, int force)
{
    int src_fd = open (src, O_RDONLY);

    if (src_fd == -1)
    {
        perror ("Could not open src file");
    }
    
    int dest_fd = creat (dest, All_Permissions);

    if (dest_fd == -1)
    {
        if (force)
        {
            unlink (dest);
            dest_fd = creat (dest, All_Permissions);
        }

        if (dest_fd == -1)
        {
            perror ("Could not open or create dest file");
            return 1;
        }
    }

    ssize_t caterr = mycat (src_fd, dest_fd);

    if (caterr)
    {
        perror ("cp");
    }

    close (src_fd);
    close (dest_fd);

    return caterr;
}

ssize_t TwoArgs (CpOpt_t *options)
{
    DEBUG_
    (
        printf ("2 files! Checking if |%s| is a dir\n",
                options->files_list[1]);
    )

    const char *src  = options->files_list[0];
    const char *dest = options->files_list[1];

    uint8_t status = IsNotDir (dest);

    DEBUG_ (printf ("status = %x\n", status));

    if (status & IS_REG_MASK || !(status & EXISTS_MASK))
    {
        DEBUG_
        (
            printf ("Not a directory. Copying...\n");
        )

        if (options->interactive && !CheckOverwrite (status, dest))
        {
            return 0;
        }

        return CP_file (src, dest, options->force);
    }
    else
    {
        DEBUG_
        (
            printf ("A directory. Copying...\n");
        )

        int dest_dir_fd = open (dest, O_DIRECTORY);
        
        if (dest_dir_fd == -1)
        {
            perror ("cp");
            return 1;
        }

        int dest_fd = openat (dest_dir_fd, src, O_EXCL | O_CREAT, All_Permissions);

        close (dest_dir_fd);
        close (dest_fd);
    }

    return 1;
}

ssize_t cp (CpOpt_t *options)
{   
    #ifdef _DEBUG
    printf ("Modes: %s%s\n", 
            options->force       ? "force"         : "no-force", 
            options->interactive ? ", interactive" : "");

    printf ("files_list = <%p>\n"
            "files_num  = (%d)\n\n",
            options->files_list,
            options->files_num);
    #endif // _DEBUG

    const char *dest_dir_name = options->files_list[options->files_num - 1];

    if (options->files_num == 2)
    {
        return TwoArgs (options);
    }

    uint8_t status = IsNotDir (dest_dir_name);

    if (status & IS_REG_MASK || !(status & EXISTS_MASK))
    {
        fprintf (stderr, "cp_my: %s: No such directory!\n", 
                 dest_dir_name);

        return 1;
    }

    for (int idx = 0; idx < options->files_num - 1; idx++)
    {
        DEBUG_ (printf ("\nfile: %s\n", options->files_list[idx]));

        int curr_fd = open (options->files_list[idx], O_RDONLY);

        // DIR* d
        // opendir
        // readdir
        // open (dir->d_name, O_CREAT)

        ssize_t caterr = mycat (curr_fd, 1);

        close (curr_fd);

        if (caterr)
        {
            perror ("cp");
        }
    }

    return 0;
}

int main (int argc, char *const *argv)
{
    CpOpt_t cp_opt = {};
    
    int ret_val = 1;

    switch (argc)
    {
        case 1:
            fputs ("cp_my: missing file operand", stderr);
            break;

        case 2:
            fputs ("cp_my: missing destination operand", stderr);
            break;

        default:
            cp_opt = ParseCpArgs (argc, argv);
            ret_val = cp (&cp_opt);

            break;
    }
    
    free (cp_opt.files_list);

    return ret_val;
}
