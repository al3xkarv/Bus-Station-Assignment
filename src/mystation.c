/*mystation reads data from config file, creates shared memory, initializes structs and semaphores,
fork and exec other processes*/

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

//bay struct
typedef struct {
    char name[4];
    int parking_spaces;
    int buses;
}bay;

//struct for each park space
typedef struct{
    int occupied;
    char type[4];
    int incpassengers;
    time_t arrival;

}park_space;

//most of the info wanted from comptroller is here
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

int main(int argc, char** argv){

    FILE* fp;
    char* token;
    int num_of_bays, retval, total_spaces=0, err=0;
    char buffer[15];
    char config_file[30];
    bay* bay_array;
    int parktime;
    pid_t pid, pid1, pid2;


    //checking flag
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i],"-l")==0){
            strcpy(config_file,argv[i+1]);
        }
    }
    //creating random incoming buses
    srand ( time(NULL) );
    int incoming_buses = rand() % 80 + 20;


     // opening file for reading config values
    fp = fopen(config_file , "r");
    if(fp == NULL) {
       perror("Error opening file");
       return(-1);
    }
    fgets (buffer, 15, fp );
    token = strtok(buffer, " ");
    if (strcmp(token , "bays")==0){
        token = strtok(NULL, " ");
        num_of_bays=atoi(token);
        printf("%d\n" , num_of_bays );
    }

    //creating an array of bay elements
    bay_array =malloc(sizeof(bay) * num_of_bays);

    for (int i = 0; i < num_of_bays; i++) {
        fgets (buffer, 15, fp );
        token = strtok(buffer, " ");
        strcpy(bay_array[i].name,token);
        token = strtok(NULL, " ");
        bay_array[i].parking_spaces=atoi(token);
        total_spaces=total_spaces + bay_array[i].parking_spaces;
        token = strtok(NULL, " ");
        bay_array[i].buses=atoi(token);
    }

    fgets (buffer, 15, fp );
    token = strtok(buffer, " ");
    if (strcmp(token , "max_wait")==0){
        token = strtok(NULL, " ");
        parktime=atoi(token);
        printf("%d\n" , parktime );
    }
    fclose(fp);

    //creating shared mem
    int shared_id = shmget(IPC_PRIVATE, (sizeof(Reference_Legder)+ total_spaces*sizeof(park_space)) ,0666);
    if (shared_id == -1)
        perror ("Creation");
    else
        printf("Allocated Shared Memory with ID: %d\n",(int)shared_id);

    //attaching memory to mystation
    void * shared_memory = (void *) shmat(shared_id, (void*)0, 0);


    Reference_Legder* pointer_for_Reference_Ledger = shared_memory;
    park_space* pointer_for_park_space = shared_memory + sizeof(Reference_Legder);

    //initializing starting values of shared mem vars
    pointer_for_Reference_Ledger->parked_buses=0;
    pointer_for_Reference_Ledger->total_passengers_entered = 0;
    pointer_for_Reference_Ledger->total_passengers_exited = 0;

    for (int i = 0; i < total_spaces; i++) {
        pointer_for_park_space[i].occupied =0;
    }

    for (int i = 0; i < DESTINATIONS; i++) {
        pointer_for_Reference_Ledger->free_spaces[i]=bay_array[i].parking_spaces;
        pointer_for_Reference_Ledger->bus_capacity[i]=80;
    }

    for (int i = 0; i < DESTINATIONS; i++) {
        pointer_for_Reference_Ledger->bay_capacity[i]=bay_array[i].parking_spaces;
    }

    for (int i = 0; i < DESTINATIONS; i++) {
        pointer_for_Reference_Ledger->passengers_waiting[i]=rand()%6000+600;
    }
    //initializing semaphores

   retval = sem_init(&pointer_for_Reference_Ledger->in , 1, 1 );
   if (retval != 0) {
       perror("Couldn't initialize.");
       exit(3);
   }

   retval = sem_init(&pointer_for_Reference_Ledger->out , 1, 1 );
   if (retval != 0) {
       perror("Couldn't initialize.");
       exit(3);
   }
   retval = sem_init(&pointer_for_Reference_Ledger->receive_message_sem , 1, 0 );
   if (retval != 0) {
       perror("Couldn't initialize.");
       exit(3);
   }
   retval = sem_init(&pointer_for_Reference_Ledger->read_message_sem , 1, 0 );
   if (retval != 0) {
       perror("Couldn't initialize.");
       exit(3);
   }
   retval = sem_init(&pointer_for_Reference_Ledger->receive_message_sem2 , 1, 0 );
   if (retval != 0) {
       perror("Couldn't initialize.");
       exit(3);
   }
   retval = sem_init(&pointer_for_Reference_Ledger->read_message_sem2 , 1, 0 );
   if (retval != 0) {
       perror("Couldn't initialize.");
       exit(3);
   }
   retval = sem_init(&pointer_for_Reference_Ledger->write , 1, 1 );
   if (retval != 0) {
       perror("Couldn't initialize.");
       exit(3);
   }



    //from here on starts execution of station-manager , comptroller and of buses
     char key[15];
     sprintf(key, "%d", shared_id);

     //starting station-manager.c
     if ((pid = fork()) < 0) {
         printf( "Failed to create station-manager process.\n");
                 // prerror(err));
         return 1;
     }

     char ask[10];
     char pel[10];
     char vor[10];

     sprintf(ask, "%d", bay_array[0].parking_spaces);
     sprintf(pel, "%d", bay_array[1].parking_spaces);
     sprintf(vor, "%d", bay_array[2].parking_spaces);
     if (!pid){
         char* args[] = {"./station-manager","-s", key, ask, pel, vor,  NULL};
         execv(args[0], args);
     }
     sleep(1);
     if ((pid2 = fork()) < 0) {
         printf( "Failed to create Comptroller process.\n");

         return 1;
     }
     //child process
     if (!pid2){
         char* args[] = {"./comptroller", "-d", "1", "-t", "10", "-s", key,  NULL};
         execv(args[0], args);
     }

     int k;
     int j;
    for (int i = 0; i < incoming_buses ; i++) {
        //random incpassengers and type for initialization
        k=rand()%81;
        j=rand()%3;
        if ((pid1 = fork()) < 0) {
            printf( "Failed to create process.\n");
                    // prerror(err));
            return 1;
        }
        //child process
        if (!pid1){

            //converting int to argument to parse in bus
            char incpassengers[15];
            sprintf(incpassengers, "%d", k);

            //executing bus and initializing random values for bus destination, passengers on board and the rest of values
			char* args[] = {"./bus", "-t", bay_array[j].name, "-n", incpassengers , "-c", "80", "-p", token, "-m", "1", "-s",key, NULL};
			execv(args[0], args);
		}


    }

    printf("Incoming buses are  %d\n", incoming_buses);



    //waiting for station-manager SET MANUALLY FOR DIFFERENT LENGTH OF PROGRAM
    sleep(190);

    //sending signals to stop station-manager and comptroller

    kill(pid,SIGUSR2);
    kill(pid2,SIGUSR2);

    //detaching share mem
    err = shmdt((void *)shared_memory);
    if ( err ==  -1 ) perror("Detachment");

    //removing shared mem
    err = shmctl(shared_id, IPC_RMID, 0);
    if (err == -1)
        perror ("Removal.");
    else
        printf("Just Removed Shared Segment. %d\n", (int)(err));

    return 0;
}
