
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/times.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <signal.h>



#define DESTINATIONS 3

void * shared_memory;


typedef struct {
    char name[4];
    int parking_spaces;
    int buses;
}bay;

typedef struct{
    int occupied;
    char type[4];
    int incpassengers;
    time_t arrival;

}park_space;

typedef struct {
    int parked_buses;
    int total_buses_entered;
    int total_buses_exited;
    int total_passengers_entered;
    int total_passengers_exited;
    int passengers_left_now;
    int bay_capacity[DESTINATIONS];
    int bus_capacity[DESTINATIONS];
    int free_spaces[DESTINATIONS];
    int passengers_waiting[DESTINATIONS];
    int incpassengers;
    char message[4];
    int chosen_space;
    int exit_space;
    sem_t receive_message_sem;
    sem_t read_message_sem;
    sem_t receive_message_sem2;
    sem_t read_message_sem2;
    sem_t write;
    sem_t in;
    sem_t out;

}Reference_Legder;

//handling of signal from mystation to terminate
void sig_handler(int signo)
{
  if (signo == SIGUSR2){
    printf("Comptroller Exiting\n" );
    shmdt((void *)shared_memory);
    exit(0);
    }
}

int main(int argc, char** argv){


    int ttime, shared_id, passengers_left_now;
    double stattimes, seconds_since_start;
    time_t start;
    time(&start);


    if (signal(SIGUSR2, sig_handler) == SIG_ERR){
            printf("\ncan't catch SIGUSR1\n");
    }

    if (argc!= 7) {
        printf("Wrong number of arguments");
        return -1;
    }
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-d")==0){
            ttime=atoi(argv[i+1]);
        }
        if (strcmp(argv[i],"-t")==0){
            stattimes = atof(argv[i+1]);
        }
        if (strcmp(argv[i],"-s")==0){
            shared_id = atoi(argv[i+1]);
        }
    }
    // goal=stattimes;
    printf("Comptroller is here with time %d statimes %f and shared_id %d\n",ttime, stattimes, shared_id );

    //attaching memory to Comptroller
    void * shared_memory = (void *) shmat(shared_id, (void*)0, 0);


    Reference_Legder* pointer_for_Reference_Ledger = shared_memory;
    park_space* pointer_for_park_space = shared_memory + sizeof(Reference_Legder);

    while (1){
        //doing -d time
        sleep(ttime);
        for (int i = 0; i < pointer_for_Reference_Ledger->bay_capacity[0] + pointer_for_Reference_Ledger->bay_capacity[1] + pointer_for_Reference_Ledger->bay_capacity[2]; i++) {
            if (pointer_for_park_space[i].occupied==1){
            passengers_left_now = passengers_left_now + pointer_for_park_space[i].incpassengers;
            }
        }
        printf("Comptroller: Buses parked: %d, Spaces left in ASK %d in PEL %d in VOR %d .\
         Passengers left now are: %d.\n",pointer_for_Reference_Ledger->parked_buses,pointer_for_Reference_Ledger->free_spaces[0], pointer_for_Reference_Ledger->free_spaces[1], pointer_for_Reference_Ledger->free_spaces[2], passengers_left_now  );
         passengers_left_now=0;

        //checking for statimes
         seconds_since_start = difftime( time(0), start);
         if (seconds_since_start  >= stattimes){
         // sleep(stattimes-ttime);
         printf("Comptroller: total passengers that have entered: %d , total passengers that have exited %d , total buses that have entered station %d , total buses that have both entered and exited %d. \n",pointer_for_Reference_Ledger->total_passengers_entered, pointer_for_Reference_Ledger->total_passengers_exited, pointer_for_Reference_Ledger->total_buses_entered, pointer_for_Reference_Ledger->total_buses_exited  );
         time(&start);
        }
    }
}
