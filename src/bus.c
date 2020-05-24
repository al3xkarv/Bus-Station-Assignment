
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

#define DESTINATIONS 3


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
    int chosen_space;
    int exit_space;
    char message[4];
    sem_t receive_message_sem;
    sem_t read_message_sem;
    sem_t receive_message_sem2;
    sem_t read_message_sem2;
    sem_t write;
    sem_t in;
    sem_t out;
    // tm
    //time
}Reference_Legder;

int main(int argc, char** argv)
{
    char type[4];
    char message[10];
    int incpassengers, capacity, parkperiod, mantime, shared_id, chosen_space;

    if (argc!= 13) {
        printf("Wrong number of arguments");
        return -1;
    }
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-t")==0){
            strcpy(type,argv[i+1]);
        }
        if (strcmp(argv[i],"-n")==0){
            incpassengers = atoi(argv[i+1]);
        }
        if (strcmp(argv[i],"-c")==0){
            capacity = atoi(argv[i+1]);
        }
        if (strcmp(argv[i],"-p")==0){
            parkperiod = atoi(argv[i+1]);
        }
        if (strcmp(argv[i],"-m")==0){
            mantime = atoi(argv[i+1]);
        }
        if (strcmp(argv[i],"-s")==0){
            shared_id = atoi(argv[i+1]);
        }
    }
    // printf("Hello there, bus is here with %s %d %d %d %d %d\n",type,incpassengers, capacity, parkperiod, mantime, shared_id );

    //attaching memory to bus
    void * shared_memory = (void *) shmat(shared_id, (void*)0, 0);


    Reference_Legder* pointer_for_Reference_Ledger = shared_memory;
    park_space* pointer_for_park_space = shared_memory + sizeof(Reference_Legder);


    //entry section
    sem_wait(&pointer_for_Reference_Ledger->in);
    printf("No incoming bus in station, checking availability for type %s and passengers %d .\n",type, incpassengers );
    strcpy(pointer_for_Reference_Ledger->message,type);
    pointer_for_Reference_Ledger->incpassengers=incpassengers;

    sem_post(&pointer_for_Reference_Ledger->receive_message_sem);
    // sem_post(&pointer_for_Reference_Ledger->write);
    sem_wait(&pointer_for_Reference_Ledger->read_message_sem);
    printf("Ok, bus sent his type to station manager\n" );

    sem_wait(&pointer_for_Reference_Ledger->receive_message_sem);
    strcpy(message,pointer_for_Reference_Ledger->message);
    // printf("%s\n", message );
    if (strcmp(message,"ok")==0){
        sem_post(&pointer_for_Reference_Ledger->read_message_sem);
    // }

    printf("Bus is entering station\n");
    //doing the maneuver
    sleep(mantime);

    if (strcmp(type,"PEL")==0){
        printf("Bus finished maneuver and parks in space %d of PEL bay\n",pointer_for_Reference_Ledger->chosen_space );
        // pointer_for_park_space[pointer_for_Reference_Ledger->bay_capacity[1]+pointer_for_Reference_Ledger->chosen_space].occupied=1;
    }

    else if (strcmp(type,"ASK")==0){
            printf("Bus finished maneuver and parks in space %d of ASK bay\n",pointer_for_Reference_Ledger->chosen_space );
            // pointer_for_park_space[pointer_for_Reference_Ledger->chosen_space].occupied=1;
        }

    else if (strcmp(type,"VOR")==0){
            printf("Bus finished maneuver and parks in space %d of VOR bay\n",pointer_for_Reference_Ledger->chosen_space );
            // pointer_for_park_space[pointer_for_Reference_Ledger->bay_capacity[1]+pointer_for_Reference_Ledger->bay_capacity[1]+pointer_for_Reference_Ledger->chosen_space].occupied=1;
        }
    // sem_wait(&pointer_for_Reference_Ledger->write);
    chosen_space=pointer_for_Reference_Ledger->chosen_space;

    // pointer_for_Reference_Ledger->parked_buses++;
    // sem_post(&pointer_for_Reference_Ledger->write);
    pointer_for_Reference_Ledger->total_buses_entered++;
    sem_post(&pointer_for_Reference_Ledger->in);
    //staying parked for parkperiod
    sleep(parkperiod);


    }
    //exit section

    sem_wait(&pointer_for_Reference_Ledger->out);
    printf("No exiting bus in station, checking station-manager for ok to leave.\n" );

    strcpy(pointer_for_Reference_Ledger->message,"ext");
    pointer_for_Reference_Ledger->exit_space=chosen_space;
    sem_post(&pointer_for_Reference_Ledger->receive_message_sem2);
    sem_wait(&pointer_for_Reference_Ledger->read_message_sem2);
    printf("Ok, station-manager let bus exit. \n" );
    sleep(mantime);
    //maybe bad idea to do here or not?
    // sem_wait(&pointer_for_Reference_Ledger->write);
    // pointer_for_Reference_Ledger->parked_buses--;
    // pointer_for_park_space[chosen_space].occupied=0;
    // if (chosen_space < pointer_for_Reference_Ledger->bay_capacity[0]){
    //     pointer_for_Reference_Ledger->free_spaces[0]++;
    //
    // }
    // else if (chosen_space < (pointer_for_Reference_Ledger->bay_capacity[0]+pointer_for_Reference_Ledger->bay_capacity[1])){
    //     pointer_for_Reference_Ledger->free_spaces[1]++;
    // }
    // else {
    //     pointer_for_Reference_Ledger->free_spaces[2]++;
    // }
    // sem_post(&pointer_for_Reference_Ledger->write);
    pointer_for_Reference_Ledger->total_buses_exited++;
    sem_post(&pointer_for_Reference_Ledger->out);


    //detaching
    //TODO USE err
    printf("bus with type %s and passengers %d finished \n",type, incpassengers );
    printf("Total buses entered %d , total buses exited %d\n",pointer_for_Reference_Ledger->total_buses_entered, pointer_for_Reference_Ledger->total_buses_exited );
    // printf("Parked buses are %d\n",pointer_for_Reference_Ledger->parked_buses );
    // printf("Free spaces are %d %d %d\n",pointer_for_Reference_Ledger->free_spaces[0], pointer_for_Reference_Ledger->free_spaces[1], pointer_for_Reference_Ledger->free_spaces[2] );
    shmdt((void *)shared_memory);

    return 0;
}
