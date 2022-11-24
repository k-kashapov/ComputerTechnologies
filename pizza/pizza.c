#include <ctype.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHECK_RET(val)                                                         \
    if (val < 0) {                                                             \
        perror(#val);                                                          \
        return val;                                                            \
    }

#define PIZZA_LEN 5 // 5 letters in "PIZZA"

// Size of shared memory
#define PIZZA_SHM_SIZE 50

// Ingridient capacity
#define INGR_CAP 4

// Buffers for each ingridient
#define P_START 0
#define I_START (P_START + INGR_CAP)
#define Z_START (I_START + INGR_CAP)
#define A_START (Z_START + INGR_CAP)

// Memory for conveyor
#define CONVEYOR_START (A_START + INGR_CAP)
#define CONVEYOR_NUM 4 // 4 Full pizzas
#define CONVEYOR_CAP (PIZZA_LEN * CONVEYOR_NUM)

#if (CONVEYOR_START + CONVEYOR_LEN) > PIZZA_SHM_SIZE
#error Not enough space for all ingridients and conveyor.
#endif

int Recipient(void) {
    printf("Recipient reporting!\n");
    return 1;
}

int MasterChief(char *shm) {
    printf("MasterChief reporting!\n");

    *(int *)shm = -1;

    return 0;
}

int SlaveChief(int slave_idx, char *shm) {
    printf("Slave #%d reporting!\n", slave_idx);

    printf("SLAVE #%d: (%d)\n", slave_idx, shm[slave_idx]);

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

    // Create shared memory for pizza
    int shm_id = shmget(IPC_PRIVATE, PIZZA_SHM_SIZE, IPC_CREAT | 0600);
    CHECK_RET(shm_id);

    printf("pizza id = (%d)\n", shm_id);

    char *pizza_addr = (char *)shmat(shm_id, NULL, SHM_RND);

    printf("pizza addr = <%p>\n", pizza_addr);
    if (pizza_addr == (char *)(-1)) {
        perror("shmat");
        return -1;
    }

    // Create semaphores for each ingridient
    int sem_id = semget(IPC_PRIVATE, 4, IPC_CREAT | 0600);
    CHECK_RET(sem_id);

    // Create Master Chief
    int pid = fork();
    if (!pid) {
        return MasterChief(pizza_addr);
    }

    // Create 4 Slaves
    for (int i = 0; i < 4; i++) {
        pid = fork();
        if (!pid) {
            return SlaveChief(i, pizza_addr);
        }
    }

    for (int pizza_num = 0; pizza_num < pizza_req;) {
        pizza_num += Recipient();
    }

    shmdt(pizza_addr);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 4, IPC_RMID, NULL);

    printf("All done!");

    return 0;
}
