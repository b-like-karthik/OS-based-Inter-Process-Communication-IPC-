#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>
#include <semaphore.h>
#include<fcntl.h>
#include <sys/types.h>

int MAX_NODES; // Maximum number of nodes in the graph
#define k1 1  
#define k2 2  
#define k3 3
#define k4 4
#define SEM_KEY 1234
#define SEM_KEY1 1235

// Define struct for semaphore operations
#define P(s) semop(s, &pop, 1)  
#define V(s) semop(s, &vop, 1)

int shmid_A;
int shmid_T;
int shmid_idx;
int sem_mtx;



int main() {
    struct sembuf pop, vop ;
    int n;  // Number of nodes/workers
    
    FILE *file = fopen("graph.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return 0;
    }

    // Read number of nodes
    if (fscanf(file, "%d", &n) != 1) {
        fprintf(stderr, "Error reading number of nodes\n");
        fclose(file);
        return 0;
    }

    shmid_A = shmget(k1, sizeof(int[n][n]), IPC_CREAT | 0777);
    int (*A)[n] = shmat(shmid_A,0,0);

    // printf("%d\n",shmid_A);

    // Read adjacency matrix
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (fscanf(file, "%d", &A[i][j]) != 1) {
                fprintf(stderr, "Error reading adjacency matrix\n");
                fclose(file);
                return 0;
            }
        }
    }

    fclose(file);

    // for(int i=0;i<n;i++){
    //     for(int j=0;j<n;j++){
    //         printf("%d ",A[i][j]);
    //     }
    //     printf("\n");
    // }

    shmid_T = shmget(k2, n*sizeof(int), IPC_CREAT | 0777);
    int *T = shmat(shmid_T, NULL, 0);
    // printf("%d\n",shmid_T);
    shmid_idx = shmget(k3, sizeof(int), IPC_CREAT | 0777);
    int *idx = shmat(shmid_idx, NULL, 0);
    *idx=0;


    sem_mtx = semget(SEM_KEY, 1, 0777|IPC_CREAT);
    semctl(sem_mtx, 0, SETVAL, 1);  // Initialize mutex semaphore
    int sync[n]; // Array of semaphores
    for (int i = 0; i < n; i++) {
        sync[i] = semget(SEM_KEY1 + i, 1, 0777 | IPC_CREAT); // Create semaphores
        semctl(sync[i], 0, SETVAL, 0); // Initialize semaphores with value 1
    }
    // printf("%d %d %d %d %d",shmid_A,shmid_T,shmid_idx,sem_mtx,sync);
    fflush(stdout);
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1 ; vop.sem_op = 1 ;

    int ntfy = semget(k4, 1, 0777|IPC_CREAT);
    semctl(ntfy,0,SETVAL,0);

    printf("+++Boss setup done...\n");
    for(int i=0;i<n;i++)
        P(ntfy);
    

    for(int i=0;i<n;i++)
        printf("%d ",T[i]-1);
    printf("\n");
    int a[n];
    for(int i=0;i<n;i++)
        a[i]=0;
    int o=0;
    for(int i=0;i<n;i++){
        a[T[i]-1]++;
        // printf("%d ",T[i]-1);
        for(int j=0;j<n;j++){
            if(A[j][T[i]-1]==1){
                if(!a[j]){
                    o++;
                    break;
                }
            }
        }
        if(o){
            printf("Something is wrong...\n");
            break;
        }
        
    }
    if(o==0){
        printf("+++Boss: Well done team...\n");
    }

    shmctl(shmid_A, IPC_RMID, NULL);
    shmctl(shmid_T, IPC_RMID, NULL);
    shmctl(shmid_idx, IPC_RMID, NULL);
    semctl(sem_mtx, 0, IPC_RMID);
    semctl(ntfy,0,IPC_RMID);
    for(int i=0;i<n;i++)
        semctl(sync[i],0,IPC_RMID);


    return 0;
}
