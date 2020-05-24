
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

//global vars for use in sig_handler function
void * shared_memory;
pid_t pid;

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

}Reference_Legder;


//handling of signal from mystation to terminate
void sig_handler(int signo)
{
  if (signo == SIGUSR2){

    printf("Station-manager Exiting\n" );
    kill(pid,SIGUSR2);
    shmdt((void *)shared_memory);
    exit(0);
    }
}

int main(int argc, char** argv)
{
    char message[4];
    int shared_id;
    int ask_spaces, pel_spaces, vor_spaces;
    time_t current_time;
    char* c_time_string;




    if (signal(SIGUSR2, sig_handler) == SIG_ERR){
            printf("\ncan't catch SIGUSR1\n");
    }

    //checking arguments
    if (argc!= 6) {
        printf("Wrong number of arguments");
        return -1;
    }
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-s")==0){
            shared_id = atoi(argv[i+1]);
            break;
        }
    }
    //how much space each type has, passed through mystation
    ask_spaces=atoi(argv[3]);
    pel_spaces=atoi(argv[4]);
    vor_spaces=atoi(argv[5]);

    printf("Hello there, station-manager is here with shared_id %d\n", shared_id );

    //attaching memory to station-manager
    void * shared_memory = (void *) shmat(shared_id, (void*)0, 0);


    Reference_Legder* pointer_for_Reference_Ledger = shared_memory;
    park_space* pointer_for_park_space = shared_memory + sizeof(Reference_Legder);

    pid=fork();
    if (pid>0){

    //here is where buses try to enter
    while (1) {
        //communication via messages and using semaphore write whenever a change in a shared var is about to happen
        sem_wait(&pointer_for_Reference_Ledger->receive_message_sem);
        strcpy(message,pointer_for_Reference_Ledger->message);
        printf("Bus with type %s trying to enter station\n",message );
        sem_post(&pointer_for_Reference_Ledger->read_message_sem);
        printf("Message is %s\n", message );
        //checking whether it's PEL ASK or VOR and doing what is instructed, freeing space, incrementing parked buses etc
        if (strcmp(message,"PEL")==0){
            if (pointer_for_Reference_Ledger->free_spaces[1]!=0){
                for (int i = ask_spaces; i < ask_spaces+pel_spaces; i++) {
                    if (pointer_for_park_space[i].occupied==0){
                        strcpy(pointer_for_Reference_Ledger->message,"ok");

                        sleep(1);
                        sem_post(&pointer_for_Reference_Ledger->receive_message_sem);
                        pointer_for_Reference_Ledger->chosen_space=i;
                        sem_wait(&pointer_for_Reference_Ledger->write);

                        /* Obtain current time. */
                        current_time = time(NULL);
                        pointer_for_park_space[i].arrival=current_time;
                        /* Convert to local time format. */
                        c_time_string = ctime(&current_time);
                        //trying to make up for not making logging file
                        (void) printf("Current time is %s", c_time_string);

                        pointer_for_Reference_Ledger->free_spaces[1]--;
                        pointer_for_Reference_Ledger->parked_buses++;
                        pointer_for_Reference_Ledger->passengers_left_now = pointer_for_Reference_Ledger->passengers_left_now - pointer_for_Reference_Ledger->incpassengers;
                        pointer_for_Reference_Ledger->total_passengers_entered = pointer_for_Reference_Ledger->total_passengers_entered + pointer_for_Reference_Ledger->incpassengers;
                        pointer_for_park_space[i].incpassengers = pointer_for_Reference_Ledger->incpassengers;
                        pointer_for_park_space[i].occupied=1;
                        strcpy(pointer_for_park_space[i].type,"PEL");

                        printf("Entered and parked at PEL %d\n",pointer_for_Reference_Ledger->chosen_space-ask_spaces);

                        sem_post(&pointer_for_Reference_Ledger->write);
                        sem_wait(&pointer_for_Reference_Ledger->read_message_sem);
                        // sem_post(&pointer_for_Reference_Ledger->write);
                        break;
                    }
                }
            }
        }
            else if(strcmp(message,"ASK")==0){
                if (pointer_for_Reference_Ledger->free_spaces[0]!=0){
                    for (int i = 0; i < ask_spaces; i++) {
                        if (pointer_for_park_space[i].occupied==0){

                            strcpy(pointer_for_Reference_Ledger->message,"ok");

                            sleep(1);
                            sem_post(&pointer_for_Reference_Ledger->receive_message_sem);
                            pointer_for_Reference_Ledger->chosen_space=i;
                            sem_wait(&pointer_for_Reference_Ledger->write);
                            /* Obtain current time. */
                            current_time = time(NULL);
                            pointer_for_park_space[i].arrival=current_time;
                            /* Convert to local time format. */
                            c_time_string = ctime(&current_time);
                            (void) printf("Current time is %s", c_time_string);
                            pointer_for_Reference_Ledger->passengers_left_now = pointer_for_Reference_Ledger->passengers_left_now - pointer_for_Reference_Ledger->incpassengers;
                            pointer_for_Reference_Ledger->total_passengers_entered = pointer_for_Reference_Ledger->total_passengers_entered + pointer_for_Reference_Ledger->incpassengers;

                            pointer_for_Reference_Ledger->free_spaces[0]--;
                            pointer_for_Reference_Ledger->parked_buses++;
                            pointer_for_park_space[i].incpassengers = pointer_for_Reference_Ledger->incpassengers;
                            pointer_for_park_space[i].occupied=1;
                            strcpy(pointer_for_park_space[i].type,"ASK");
                            printf("Entered and parked at ASK %d\n",pointer_for_Reference_Ledger->chosen_space );
                            sem_post(&pointer_for_Reference_Ledger->write);
                            sem_wait(&pointer_for_Reference_Ledger->read_message_sem);

                            break;
                        }
                    }
                }
                else if (pointer_for_Reference_Ledger->free_spaces[1]!=0){
                    for (int i = ask_spaces; i < ask_spaces+pel_spaces; i++) {
                        if (pointer_for_park_space[i].occupied==0){

                            strcpy(pointer_for_Reference_Ledger->message,"ok");

                            sleep(1);
                            sem_post(&pointer_for_Reference_Ledger->receive_message_sem);
                            pointer_for_Reference_Ledger->chosen_space=i;

                            sem_wait(&pointer_for_Reference_Ledger->write);

                            /* Obtain current time. */
                            current_time = time(NULL);
                            pointer_for_park_space[i].arrival=current_time;
                            /* Convert to local time format. */
                            c_time_string = ctime(&current_time);
                            (void) printf("Current time is %s", c_time_string);
                            pointer_for_Reference_Ledger->passengers_left_now = pointer_for_Reference_Ledger->passengers_left_now - pointer_for_Reference_Ledger->incpassengers;
                            pointer_for_Reference_Ledger->total_passengers_entered = pointer_for_Reference_Ledger->total_passengers_entered + pointer_for_Reference_Ledger->incpassengers;


                            pointer_for_Reference_Ledger->free_spaces[1]--;
                            pointer_for_Reference_Ledger->parked_buses++;
                            pointer_for_park_space[i].incpassengers = pointer_for_Reference_Ledger->incpassengers;
                            pointer_for_park_space[i].occupied=1;
                            strcpy(pointer_for_park_space[i].type,"ASK");
                            printf("Entered and parked at PEL %d\n",pointer_for_Reference_Ledger->chosen_space-ask_spaces );

                            sem_post(&pointer_for_Reference_Ledger->write);

                            sem_wait(&pointer_for_Reference_Ledger->read_message_sem);
                            break;
                        }
                    }

                }
            }
            else if (strcmp(message,"VOR")==0){
                if (pointer_for_Reference_Ledger->free_spaces[2]!=0){
                    for (int i = ask_spaces+pel_spaces; i < ask_spaces+pel_spaces+vor_spaces; i++) {
                        if (pointer_for_park_space[i].occupied==0){

                            strcpy(pointer_for_Reference_Ledger->message,"ok");

                            sleep(1);
                            sem_post(&pointer_for_Reference_Ledger->receive_message_sem);
                            pointer_for_Reference_Ledger->chosen_space=i;

                            sem_wait(&pointer_for_Reference_Ledger->write);

                            /* Obtain current time. */
                            current_time = time(NULL);
                            pointer_for_park_space[i].arrival=current_time;
                            /* Convert to local time format. */
                            c_time_string = ctime(&current_time);
                            (void) printf("Current time is %s", c_time_string);
                            pointer_for_Reference_Ledger->passengers_left_now = pointer_for_Reference_Ledger->passengers_left_now - pointer_for_Reference_Ledger->incpassengers;
                            pointer_for_Reference_Ledger->total_passengers_entered = pointer_for_Reference_Ledger->total_passengers_entered + pointer_for_Reference_Ledger->incpassengers;


                            pointer_for_Reference_Ledger->free_spaces[2]--;
                            pointer_for_Reference_Ledger->parked_buses++;
                            pointer_for_park_space[i].incpassengers = pointer_for_Reference_Ledger->incpassengers;
                            pointer_for_park_space[i].occupied=1;
                            strcpy(pointer_for_park_space[i].type,"VOR");
                            printf("Entered and parked at VOR %d\n",pointer_for_Reference_Ledger->chosen_space-ask_spaces-pel_spaces );
                            sem_post(&pointer_for_Reference_Ledger->write);
                            sem_wait(&pointer_for_Reference_Ledger->read_message_sem);
                            break;
                        }
                    }
                }
                else if (pointer_for_Reference_Ledger->free_spaces[1]!=0){
                    for (int i = ask_spaces; i < ask_spaces+pel_spaces; i++) {
                        if (pointer_for_park_space[i].occupied==0){

                            strcpy(pointer_for_Reference_Ledger->message,"ok");

                            sleep(1);
                            sem_post(&pointer_for_Reference_Ledger->receive_message_sem);
                            pointer_for_Reference_Ledger->chosen_space=i;

                            sem_wait(&pointer_for_Reference_Ledger->write);

                            /* Obtain current time. */
                            current_time = time(NULL);
                            pointer_for_park_space[i].arrival=current_time;
                            /* Convert to local time format. */
                            c_time_string = ctime(&current_time);
                            (void) printf("Current time is %s", c_time_string);
                            pointer_for_Reference_Ledger->passengers_left_now = pointer_for_Reference_Ledger->passengers_left_now - pointer_for_Reference_Ledger->incpassengers;
                            pointer_for_Reference_Ledger->total_passengers_entered = pointer_for_Reference_Ledger->total_passengers_entered + pointer_for_Reference_Ledger->incpassengers;


                            pointer_for_Reference_Ledger->free_spaces[1]--;
                            pointer_for_Reference_Ledger->parked_buses++;
                            pointer_for_park_space[i].incpassengers = pointer_for_Reference_Ledger->incpassengers;
                            pointer_for_park_space[i].occupied=1;
                            strcpy(pointer_for_park_space[i].type,"VOR");
                            printf("Entered and parked at PEL %d\n",pointer_for_Reference_Ledger->chosen_space-ask_spaces );
                            sem_post(&pointer_for_Reference_Ledger->write);
                            sem_wait(&pointer_for_Reference_Ledger->read_message_sem);
                            break;
                        }
                    }
                }
            }

        }
    }
    //here is where buses try to exit
    if (pid==0){
        while (1) {

        //using different sempahores for messaging at exit
        sem_wait(&pointer_for_Reference_Ledger->receive_message_sem2);
        strcpy(message,pointer_for_Reference_Ledger->message);
        printf("Bus with type %s trying to enter station\n",message );
        sem_post(&pointer_for_Reference_Ledger->read_message_sem2);
        printf("Message is %s\n", message );
        printf("we are at child of station-manager\n" );
        //checking if message for exit was sent and then incrementing spaces, passengers that left etc
        if (strcmp(message,"ext")==0){
            printf("exit_space about to be deleted %d\n", pointer_for_Reference_Ledger->exit_space);
            if (pointer_for_Reference_Ledger->exit_space < pointer_for_Reference_Ledger->bay_capacity[0]){
                if (pointer_for_Reference_Ledger->free_spaces[0]== pointer_for_Reference_Ledger->bay_capacity[0]){

                continue;
            }
            }
            else if (pointer_for_Reference_Ledger->exit_space < (pointer_for_Reference_Ledger->bay_capacity[0]+pointer_for_Reference_Ledger->bay_capacity[1])){
                if (pointer_for_Reference_Ledger->free_spaces[1]== pointer_for_Reference_Ledger->bay_capacity[1]){

                    continue;
                }
            }
            else {
                if (pointer_for_Reference_Ledger->free_spaces[2]== pointer_for_Reference_Ledger->bay_capacity[2]){

                    continue;
                }
            }

            sem_wait(&pointer_for_Reference_Ledger->write);
            pointer_for_park_space[pointer_for_Reference_Ledger->exit_space].occupied=0;
            pointer_for_Reference_Ledger->parked_buses--;

            //check which type you belong to change the correct values
            if (pointer_for_Reference_Ledger->exit_space < pointer_for_Reference_Ledger->bay_capacity[0]){
                pointer_for_Reference_Ledger->free_spaces[0]++;

                //if there are less passengers waiting than capacity of bus take remaining
                if (pointer_for_Reference_Ledger->passengers_waiting[0] < pointer_for_Reference_Ledger->bus_capacity[0]){
                    pointer_for_Reference_Ledger->total_passengers_exited = pointer_for_Reference_Ledger->total_passengers_exited + pointer_for_Reference_Ledger->passengers_waiting[0];
                    pointer_for_Reference_Ledger->passengers_waiting[0]=0;
                }
                //else take as much as you can
                else if (pointer_for_Reference_Ledger->passengers_waiting[0] >= pointer_for_Reference_Ledger->bus_capacity[0]) {
                    pointer_for_Reference_Ledger->total_passengers_exited = pointer_for_Reference_Ledger->total_passengers_exited + pointer_for_Reference_Ledger->bus_capacity[0];
                    pointer_for_Reference_Ledger->passengers_waiting[0]= pointer_for_Reference_Ledger->passengers_waiting[0] - pointer_for_Reference_Ledger->bus_capacity[0];
                }
            }

            else if (pointer_for_Reference_Ledger->exit_space < (pointer_for_Reference_Ledger->bay_capacity[0]+pointer_for_Reference_Ledger->bay_capacity[1])){
                pointer_for_Reference_Ledger->free_spaces[1]++;

                //if there are less passengers waiting than capacity of bus take remaining
                if (pointer_for_Reference_Ledger->passengers_waiting[1] < pointer_for_Reference_Ledger->bus_capacity[1]){
                    pointer_for_Reference_Ledger->total_passengers_exited = pointer_for_Reference_Ledger->total_passengers_exited + pointer_for_Reference_Ledger->passengers_waiting[1];
                    pointer_for_Reference_Ledger->passengers_waiting[1]=0;
                }

                //else take as much as you can
                else if (pointer_for_Reference_Ledger->passengers_waiting[1] >= pointer_for_Reference_Ledger->bus_capacity[1]) {
                    pointer_for_Reference_Ledger->total_passengers_exited = pointer_for_Reference_Ledger->total_passengers_exited + pointer_for_Reference_Ledger->bus_capacity[1];
                    pointer_for_Reference_Ledger->passengers_waiting[1]= pointer_for_Reference_Ledger->passengers_waiting[1] - pointer_for_Reference_Ledger->bus_capacity[1];
                }
            }
            else {

                //if there are less passengers waiting than capacity of bus take remaining
                if (pointer_for_Reference_Ledger->passengers_waiting[2] < pointer_for_Reference_Ledger->bus_capacity[2]){
                    pointer_for_Reference_Ledger->total_passengers_exited = pointer_for_Reference_Ledger->total_passengers_exited + pointer_for_Reference_Ledger->passengers_waiting[2];
                    pointer_for_Reference_Ledger->passengers_waiting[2]=0;
                }
                //else take as much as you can
                else if (pointer_for_Reference_Ledger->passengers_waiting[2] < pointer_for_Reference_Ledger->bus_capacity[2]) {
                    pointer_for_Reference_Ledger->total_passengers_exited = pointer_for_Reference_Ledger->total_passengers_exited + pointer_for_Reference_Ledger->bus_capacity[2];
                    pointer_for_Reference_Ledger->passengers_waiting[2]= pointer_for_Reference_Ledger->passengers_waiting[2] - pointer_for_Reference_Ledger->bus_capacity[2];
                }
                pointer_for_Reference_Ledger->free_spaces[2]++;
            }
            printf("Total passengers exited are %d\n", pointer_for_Reference_Ledger->total_passengers_exited );
            sem_post(&pointer_for_Reference_Ledger->write);
            sem_post(&pointer_for_Reference_Ledger->read_message_sem2);


        }
        else{
            printf("Not accepting non PEL right now\n" );
        }


        }
    }

    printf("station manager ending\n" );

    //detaching shared mem


    shmdt((void *)shared_memory);
    return 0;
}
