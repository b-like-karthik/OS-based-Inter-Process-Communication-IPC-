#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>



#define MAX_NODES 100  // Maximum number of nodes in the graph

#define k1 1  
#define k2 2  
#define k3 3
#define k4 4
#define SEM_KEY1 1235
#define SEM_KEY 1234

#define P(s) semop(s, &pop, 1)  
#define V(s) semop(s, &vop, 1)

int main(int argc, char *argv[]) {
    struct sembuf pop, vop ;
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <n> <id>\n", argv[0]);
        return 1;
    }
    

    int n = atoi(argv[1]);  // Number of nodes/workers
    int id = atoi(argv[2]); // ID of this worker process
    // printf("%d\n",id);
    // printf("%d %d\n",n,id);

    int shmid_A = shmget(k1, sizeof(int), 0777);
    // printf("%d\n",shmid_A);
    int (*A)[n] = shmat(shmid_A, NULL, 0);
    // for(int i=0;i<n;i++){
    //     for(int j=0;j<n;j++)
    //         printf("%d ",A[i][j]);
    //     printf("\n");
    // }

    int shmid_T = shmget(k2, sizeof(int), 0777);
    int *T = shmat(shmid_T, NULL, 0);
    // printf("%d\n",shmid_T);

    int shmid_idx = shmget(k3, sizeof(int), 0777);
    int *idx = shmat(shmid_idx, NULL, 0);

    // // Get semaphore set ID
    int sem_mtx = semget(SEM_KEY, 1, 0777);
    int sync[n]; // Array of semaphores
    for (int i = 0; i < n; i++) {
        sync[i] = semget(SEM_KEY1 + i, 1, 0777); // Create semaphores
        // semctl(sync[i], 0, SETVAL, 1); // Initialize semaphores with value 1
    }
    // printf("%d %d %d %d %d\n",shmid_A,shmid_T,shmid_idx,sem_mtx,sync);
    // sem_t *ntfy = sem_open("ntfy", O_CREAT, 0666, 0);

    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1 ; vop.sem_op = 1 ;

    // Wait for sync signals from incoming links
    for (int j = 0; j < n; j++) {
        if (A[j][id] == 1) {
            int o=0;
            for(int i=0;i<*idx;i++){
                if(T[i]==j+1){
                    o++;
                    break;
                }
            }
            if(o==0){
                semctl(sync[j], 0, SETVAL, 0);
                P(sync[j]);
            }
        }
    }
    P(sem_mtx);
    T[*idx] = id+1;
    (*idx)++;
    V(sem_mtx);
    // V(sync[id]);
    
    for (int j = 0; j < n; j++) {
        V(sync[id]);
    }

    // // Notify boss process that this worker is done
    int ntfy =semget(k4,1,0777);
    V(ntfy);

    // Detach from shared memory segments
    shmdt(A);
    shmdt(T);
    shmdt(idx);

    return 0;
}
