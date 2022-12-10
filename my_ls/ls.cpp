#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// readdir
// opendir
// rewinddir
// getpwuid
// getgrgid
// fstasat
// -R -l -a -i -n

#ifdef DEBUG
#define DEBUG_(code)                                                           \
    { code; }
#else
#define DEBUG_(code) ;
#endif

#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

#define WIDTH "-1" // field width for each file name

#define NAMESPERLINE 7

#define MAX_PATH_LEN 4096
#define PATH_LEN_PADDING 10

int colored_vprintf(const char *color, const char *fmt, ...) {
    fputs(color, stdout);

    va_list ptr;
    va_start(ptr, fmt);
    vprintf(fmt, ptr);
    va_end(ptr);

    printf(KNRM);
    return 0;
}

typedef struct LsOPT {
    int recursive : 1;
    int longList : 1;
    int all : 1;
    int inode : 1;
    int numeric : 1;
    char *dirname;
} LsOPT;

static LsOPT ParseArgs(int argc, char *const *argv) {
    int opt_ret = 0;
    LsOPT ls_opt = {};

    while (opt_ret != -1) {
        opt_ret = getopt(argc, argv, "-R-l-a-i-n");

        DEBUG_(printf("\nopt_ret = %c (%d)\n", opt_ret, opt_ret););

        switch (opt_ret) {
        case 'R':
            DEBUG_(printf("Recursive\n"));
            ls_opt.recursive = 1;
            break;

        case 'l':
            DEBUG_(printf("long\n"));
            ls_opt.longList = 1;
            break;

        case 'i':
            DEBUG_(printf("inode\n"));
            ls_opt.inode = 1;
            break;

        case 'a':
            DEBUG_(printf("all\n"));
            ls_opt.all = 1;
            break;

        case 'n':
            DEBUG_(printf("numeric\n"));
            ls_opt.numeric = 1;
            break;

        default:
            DEBUG_(printf("default: %s\n", argv[optind - 1]));
            ls_opt.dirname = argv[optind - 1];
            break;
        }
    }

    return ls_opt;
}

int my_strcpy(char *dest, const char *src) {
    char *start = dest;
    while ((*dest++ = *src++))
        ;

    return dest - start - 1;
}

#define RIGHTS_TRIPLET(mode_val, shift, who)                                   \
    {                                                                          \
        rights[shift] = (mode_val & S_IR##who) ? 'r' : '-';                    \
        rights[shift + 1] = (mode_val & S_IW##who) ? 'w' : '-';                \
        rights[shift + 2] = (mode_val & S_IX##who) ? 'x' : '-';                \
    }

static void PrintRights(mode_t mode) {
    char rights[10] = {0};

    RIGHTS_TRIPLET(mode, 0, USR);
    RIGHTS_TRIPLET(mode, 3, GRP);
    RIGHTS_TRIPLET(mode, 6, OTH);

    printf("%s ", rights);
    return;
}
#undef RIGHTS_TRIPLET

static void PrintIDs(id_t uid, id_t gid, char mode) {
    if (mode == 'n') {
        printf("%d ", uid);
        printf("%d ", gid);
        return;
    }

    struct passwd *pwd;
    pwd = getpwuid(uid);
    if (!pwd) {
        fprintf(stderr, "myid: \'%d\': no such user\n", uid);
        return;
    }

    printf("%s ", pwd->pw_name);
    printf("%s ", getpwuid(gid)->pw_name);

    return;
}

int ListDir(const char *name, LsOPT options) {
    DIR *cur_dir = opendir(name);

    if (!cur_dir) {
        fprintf(stderr, "mls: %s: %s\n", name, strerror(errno));
        return -1;
    }

    for (struct dirent *dir = readdir(cur_dir); dir; dir = readdir(cur_dir)) {
        DEBUG_(printf("name = |%s|, type = (%d)\n", dir->d_name, dir->d_type))

        if (*dir->d_name == '.' && !options.all) {
            continue;
        }

        if (options.inode) {
            struct stat file_stat;
            fstatat(dirfd(cur_dir), dir->d_name, &file_stat, 0);

            printf("%ld ", file_stat.st_ino);
        }

        if (options.longList || options.numeric) {
            printf("\n%c", dir->d_type == DT_DIR ? 'd' : '-');

            struct stat stat;

            if (fstatat(dirfd(cur_dir), dir->d_name, &stat, 0) == -1) {
                fprintf(stderr, "mls: %s: %s\n", dir->d_name, strerror(errno));
                return -1;
            }

            PrintRights(stat.st_mode);

            printf("%ld ", stat.st_nlink);

            PrintIDs(stat.st_uid, stat.st_gid, options.numeric ? 'n' : 'l');

            printf("%-4ld ", stat.st_size);

            struct timespec ts = stat.st_mtim;
            
            char* time_str = ctime(&ts.tv_sec);
            if (time_str == NULL)
            {
                fprintf(stderr, "ctime: %s \n", strerror(errno));
                return -errno;
            }

            time_str[strlen(time_str) - 1] = ' ';
            printf("%s", time_str);
        }

        if (dir->d_type == DT_DIR) {
            colored_vprintf(KGRN, "%" WIDTH "s  ", dir->d_name);
        } else {
            printf("%" WIDTH "s  ", dir->d_name);
        }
    }

    putc('\n', stdout);

    rewinddir(cur_dir);

    if (options.recursive) {
        for (struct dirent *dir = readdir(cur_dir); dir;
             dir = readdir(cur_dir)) {

            if (dir->d_type != DT_DIR) {
                continue;
            }

            if (*dir->d_name == '.') {
                if (!options.all) {
                    continue;
                } else {
                    if (dir->d_name[1] == '.' || dir->d_name[1] == '\0') {
                        continue;
                    }
                }
            }

            char full_path[MAX_PATH_LEN];
            int len = my_strcpy(full_path, name);

            if (len >= MAX_PATH_LEN - PATH_LEN_PADDING) {
                fprintf(stderr, "mls: path is too long: %s\n", full_path);
                closedir(cur_dir);
                return -1;
            }

            full_path[len] = '/';
            full_path[len + 1] = '\0';

            strcat(full_path + len + 1, dir->d_name);

            DEBUG_(printf("name = |%s|, dir = |%s|, new_path = |%s|\n", name,
                          dir->d_name, full_path));

            colored_vprintf(KBLU, "\n%s:\n\t", full_path);
            ListDir(full_path, options);
        }
    }

    closedir(cur_dir);

    return 0;
}

int main(int argc, char *const *argv) {

    LsOPT ls_opt = ParseArgs(argc, argv);

    return ListDir(ls_opt.dirname, ls_opt);
}
