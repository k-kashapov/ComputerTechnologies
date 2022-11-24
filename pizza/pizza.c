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

// 5 letters in "PIZZA"
#define PIZZA_LEN 5

// Number of different ingridients: ['P', 'I', 'Z', 'A']
#define INGR_NUM 4

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

// Number of semaphores =
// (num of ingridients) + (number of conveyor parts) + 1 (is pizza ready)
#define SEMAPHORES_NUM (INGR_NUM + CONVEYOR_NUM + 1)

union semun {
    int val;               /* Value for SETVAL */
    struct semid_ds *buf;  /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* Array for GETALL, SETALL */
    struct seminfo *__buf; /* Buffer for IPC_INFO
                              (Linux-specific) */
};

/*
 * Recipient waits for pizza is ready semaphore to be set
 * when set, it checks if pizza is OK and grabs it from the belt.
 * It then returns 1.
 */
int Recipient(void) {
    printf("Recipient reporting!\n");
    return 1;
}

/*
 * Master checks semaphores for ingridients. If any not set,
 * refills the ingridients and sets the semaphore value to 1.
 */
int MasterChief(char *shm) {
    printf("MasterChief reporting!\n");

    *(int *)shm = -1;

    return 0;
}

/*
 * Slave checks if the semaphore for the respective ingridient is set.
 * If his semaphore is set, takes one ingridient and adds it to the
 * first available position in a conveyor belt.
 * If the Slave has used all available ingrients, it lowers the semaphore
 * and waits for the Chief to put them back.
 */
int SlaveChief(int slave_idx, char *shm) {
    printf("Slave #%d reporting!\n", slave_idx);

    while (1) {
        printf("SLAVE #%d: (%d)\n", slave_idx, shm[slave_idx]);
    }

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
    int sem_id = semget(IPC_PRIVATE, SEMAPHORES_NUM, IPC_CREAT | 0600);
    CHECK_RET(sem_id);

    union semun sem_arg;
    unsigned short sem_vals[SEMAPHORES_NUM] = {0};
    sem_arg.array = sem_vals;

    // Initialize semaphores with zeroes
    int set_ret = semctl(sem_id, SEMAPHORES_NUM, SETALL, sem_arg);
    CHECK_RET(set_ret);

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

    printf("Waiting for all children to finish...\n");

    while (wait(NULL) > 0)
        ;

    shmdt(pizza_addr);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, INGR_NUM, IPC_RMID, NULL);

    printf("All done!\n");

    return 0;
}
