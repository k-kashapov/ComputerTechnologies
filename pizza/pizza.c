#include <ctype.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SHARED_MEMORY_OBJECT_SIZE 50

int Recipient(void) {
    printf("Recipient reporting!\n");
    return 1;
}

int MasterChief(void) {
    printf("MasterChief reporting!\n");
    return 0;
}

int SlaveChief(int slave_idx) {
    printf("Slave #%d reporting!\n", slave_idx);
    return 0;
}

int main(int argc, const char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: pizza.elf NUMBER\n");
        return -1;
    }

    int pizza_req = atoi(argv[1]);

    if (!pizza_req) {
        fprintf(stderr, "ERROR: insufficient pizza num\n");
        return -1;
    }

    int shm_id = shmget(IPC_PRIVATE, );
    if (shm_fd < 0) {
        perror("shm open");
        return shm;
    }

    printf("pizza fd = (%d)\n", shm);

    char *pizza_addr = (char *)mmap(0, SHARED_MEMORY_OBJECT_SIZE + 1,
                                    PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);

    printf("pizza addr = <%p>\n", pizza_addr);
    if (pizza_addr == (char *)(-1)) {
        perror("mmap");
        return -1;
    }

    // Create Master Chief
    int pid = fork();
    if (!pid) {
        return MasterChief();
    }

    // Create 4 Slaves
    for (int i = 0; i < 4; i++) {
        pid = fork();
        if (!pid) {
            return SlaveChief(i);
        }
    }

    for (int pizza_num = 0; pizza_num < pizza_req;) {
        pizza_num += Recipient();
    }

    shm_unlink("Shared_pizza");

    printf("All done!");

    return 0;
}
