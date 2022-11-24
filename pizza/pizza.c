#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
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

static const char Ingridients[] = {'P', 'I', 'Z', 'A'};

// 5 letters in "PIZZA"
#define PIZZA_LEN 5

// Number of different ingridients: ['P', 'I', 'Z', 'A']
#define INGR_NUM (sizeof(Ingridients) / sizeof(Ingridients[0]))

// Size of shared memory
#define PIZZA_SHM_SIZE 120

// Ingridient capacity
#define INGR_CAP 12

// Buffers for each ingridient
#define P_START 0
#define I_START (P_START + INGR_CAP)
#define Z_START (I_START + INGR_CAP)
#define A_START (Z_START + INGR_CAP)

// Memory for conveyor
#define CONVEYOR_START (A_START + INGR_CAP)
#define CONVEYOR_NUM 4 // 4 Full pizzas
#define CONVEYOR_CAP (PIZZA_LEN * CONVEYOR_NUM)

#define SHIFT_END_BYTE (CONVEYOR_START + CONVEYOR_CAP)

#if SHIFT_END_BYTE + 1 > PIZZA_SHM_SIZE
#error Not enough space for all ingridients and conveyor.
#endif

// Number of semaphores =
// (num of ingridients) + (number of conveyor parts)
#define SEMAPHORES_NUM (INGR_NUM + CONVEYOR_NUM)

union semun {
    int val;               /* Value for SETVAL */
    struct semid_ds *buf;  /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* Array for GETALL, SETALL */
    struct seminfo *__buf; /* Buffer for IPC_INFO
                              (Linux-specific) */
};

int semopSingle(int semid, short unsigned int semn, short int op) {
    struct sembuf operation = {semn, op, 0};
    return semop(semid, &operation, 1);
}

int semZero(int semid, short unsigned int semn) {
    struct sembuf operation = {semn, 0, IPC_NOWAIT};
    int res = semop(semid, &operation, 1);

    if (res == -1) {
        if (errno == EAGAIN) {
            return 0;
        }
    }

    return 1;
}

int semopArr(int semid, short unsigned int *semn, short int *op, int semn_len) {
    struct sembuf *ops =
        (struct sembuf *)malloc(sizeof(struct sembuf) * semn_len);

    for (int i = 0; i < semn_len; i++) {
        ops[i].sem_op = op[i];
        ops[i].sem_num = semn[i];
        ops[i].sem_flg = 0;
    }

    int opret = semop(semid, ops, semn_len);

    free(ops);
    return opret;
}

/*
 * Recipient waits for any conveyor semaphore to be lowered
 * it then checks if the pizza is correct, clears the conveyor
 * and returns 1
 */
int Recipient(char *shm, int semid) {
    printf("Recipient reporting!\n");

    for (size_t i = SEMAPHORES_NUM - CONVEYOR_NUM; i < SEMAPHORES_NUM; i++) {
        if (semZero(semid, i)) {
            printf("Should check convey #%ld\n", SEMAPHORES_NUM - i);
            break;
        }
    }

    shm[SHIFT_END_BYTE] = 1;
    printf("\nShift ended!\n");

    return 1;
}

/*
 * Master checks each ingridient semaphore. If it's zero,
 * refills the ingridients and sets the semaphore value to INGR_CAP.
 */
int MasterChief(char *shm, int semid) {
    printf("MasterChief reporting!\n");

    while (1) {
        printf("MASTER: Loop start\n");
        fflush(stdout);

        if (shm[SHIFT_END_BYTE]) {
            break;
        }

        for (size_t i = 0; i < INGR_NUM; i++) {
            if (!semZero(semid, i)) {
                printf("MASTER: No help at #%ld", i);

                continue;
            }

            printf("MASTER: Help requrired at #%ld. Refilling...\n", i);

            memset(shm + INGR_CAP * i, Ingridients[i], INGR_CAP);

            printf("MASTER: Refilled.\n");

            semopSingle(semid, i, INGR_CAP);
        }

        printf("MASTER: Loop end\n");
        sleep(3);
    }

    printf("MASTER: Shift end\n");
    fflush(stdout);

    return 0;
}

/*
 * Slave checks if the semaphore for the respective ingridient is zero.
 * If his semaphore is > 0, takes one ingridient and adds it to the
 * first available position in a conveyor belt.
 * If the Slave has used all available ingrients,
 * it waits for the Chief to put them back.
 */
int SlaveChief(int slave_idx, char *shm, int semid) {
    const int ingr_take[INGR_NUM] = {1, 1, 2, 1};

    // Shm index to start reading from
    int slave_addr = slave_idx * INGR_CAP;

    // Shm idx of first conv belt to put ingridients to
    const int patch_addr[INGR_NUM] = {CONVEYOR_START, CONVEYOR_START + 1,
                                      CONVEYOR_START + 2, CONVEYOR_START + 4};

    printf("Slave #%d reporting!\n", slave_idx);

    int taken_count = 0;

    while (1) {
        printf("SLAVE #%d: Loop start\n", slave_idx);
        fflush(stdout);

        if (semZero(semid, slave_idx)) {
            if (shm[SHIFT_END_BYTE]) {
                break;
            }

            sleep(2);
            continue;
        }

        semopSingle(semid, slave_idx, -ingr_take[slave_idx]);

        for (int byte = 0;
             byte < INGR_CAP && taken_count < ingr_take[slave_idx]; byte++) {
            if (shm[slave_addr + byte]) {
                shm[slave_addr + byte] = 0;
                taken_count++;
            }
        }

        printf("SLAVE #%d: took ingridients\n", slave_idx);

        for (int conv_num = 0; conv_num < CONVEYOR_NUM; conv_num++) {
            for (int ingr_put = 0; ingr_put < ingr_take[slave_idx];
                 ingr_put++) {
                int ingr_addr =
                    patch_addr[slave_idx] + conv_num * PIZZA_LEN + ingr_put;

                if (shm[ingr_addr] == 0) {
                    shm[ingr_addr] = Ingridients[slave_idx];
                    printf("SLAVE #%d: put ingridient to shm[%d]\n", slave_idx,
                           ingr_addr);
                    taken_count--;
                }

                if (taken_count <= 0) {
                    goto done;
                }
            }
        }
    done:
        printf("SLAVE #%d: loop end\n", slave_idx);
    }

    printf("SLAVE #%d: Shift end\n", slave_idx);
    fflush(stdout);

    return 0;
}

int main(int argc, const char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: pizza.elf [NUMBER]\n");
        return -1;
    }

    int pizza_req = atoi(argv[1]);

    if (!pizza_req) {
        fprintf(stderr, "ERROR: insufficient pizza num\n");
        return -1;
    }

    // Create shared memory for pizza
    int shmid = shmget(IPC_PRIVATE, PIZZA_SHM_SIZE, IPC_CREAT | 0600);
    CHECK_RET(shmid);

    printf("pizza id = (%d)\n", shmid);

    char *pizza_addr = (char *)shmat(shmid, NULL, SHM_RND);

    printf("pizza addr = <%p>\n", pizza_addr);
    if (pizza_addr == (char *)(-1)) {
        perror("shmat");
        return -1;
    }

    // Create semaphores for each ingridient
    int semid = semget(IPC_PRIVATE, SEMAPHORES_NUM, IPC_CREAT | 0600);
    CHECK_RET(semid);

    union semun sem_arg;
    unsigned short sem_vals[SEMAPHORES_NUM] = {0};

    // All conveyors are blocked for recipient
    for (size_t pizza_idx = SEMAPHORES_NUM - CONVEYOR_NUM;
         pizza_idx < SEMAPHORES_NUM; pizza_idx++) {
        printf ("Sem #%ld = 1\n", pizza_idx);
        sem_vals[pizza_idx] = 1;
    }

    sem_arg.array = sem_vals;

    // Initialize semaphores with zeroes
    int set_ret = semctl(semid, SEMAPHORES_NUM, SETALL, sem_arg);
    CHECK_RET(set_ret);

    // Create Master Chief
    int pid = fork();
    CHECK_RET(pid);
    if (!pid) {
        return MasterChief(pizza_addr, semid);
    }

    printf("Master pid = %d\n", pid);

    // Create 4 Slaves
    for (int i = 0; i < 4; i++) {
        pid = fork();
        CHECK_RET(pid);
        if (!pid) {
            return SlaveChief(i, pizza_addr, semid);
        }
        printf("Slave #%d pid = %d\n", i, pid);
    }

    for (int pizza_num = 0; pizza_num < pizza_req;) {
        pizza_num += Recipient(pizza_addr, semid);
    }

    printf("Waiting for all children to finish...\n");

    while ((pid = wait(NULL)) > 0) {
        printf("pid = %d finished\n", pid);
    }

    shmdt(pizza_addr);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, SEMAPHORES_NUM, IPC_RMID, NULL);

    printf("All done!\n");

    return 0;
}
