#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <semaphore.h>
#include <signal.h>
#include "../headers/constants.h"
#include "../headers/shared_segment.h"

// Handles done signal
int done = 0;
sem_t *transactionSemaphore;
void signal_handler(int signum) {
    if (signum == SIGUSR2) {
      sem_post(transactionSemaphore);
      done = 1;
    }
}

int main(int argc, char const *argv[])
{
  // Register stop signal
  signal(SIGUSR2,signal_handler);
  // Not enough arguments, so specify the correct usage and exit
  if (argc != 3) {
    fprintf(stderr,"Usage: ./station-manager -s shmid\n");
    exit(0);
  }
  FILE *logfile;
  // Read the arguments
  int shmid;
  // Error
  if (strcmp(argv[1],"-s")) {
    fprintf(stderr,"Usage: ./station-manager -s shmid\n");
    exit(0);
  } else {
    // Read shared segment id
    shmid = atoi(argv[2]);
  }

  // Attach shared memory segment
  Shared_segment *sm;
  if ((sm = shmat(shmid,NULL,0)) == (void*)-1) {
    perror("Error attaching shared memory segment to station-manager:");
    exit(1);
  }

  transactionSemaphore = &(sm->vehicle_transaction);

  // Start simulation
  sem_wait(&(sm->output));
  printf("Started station_manager with pid:%d\n",getpid());
  sem_post(&(sm->output));

  while (1) {
    // Wait for request from incoming or outcoming bus
    sem_wait(&(sm->logfile));
    logfile = fopen("logfile.txt","a+");
    fprintf(logfile,"Station_manager waiting for bus transaction...\n");
    fclose(logfile);
    sem_post(&(sm->logfile));

    sem_wait(&(sm->vehicle_transaction));
    // Check if we received done signal
    if (done) 
      break;

    sem_wait(&(sm->logfile));
    logfile = fopen("logfile.txt","a+");
    fprintf(logfile,"Station_manager received bus signal.\n");
    fclose(logfile);
    sem_post(&(sm->logfile));

    // Depending on the transaction type wait for notification from the bus
    char transaction_type;
    int bus_id;
    int bus_type;
    char bus_plate[BUS_PLATE_SIZE+1];
    sem_wait(&(sm->IPC_mutex));
    transaction_type = sm->bus_transaction_type;
    switch (transaction_type)
    {
      case INBOUND_VEHICLE:
        bus_id = sm->inbound_bus_id;
        bus_type = sm->inbound_bus_type;
        strcpy(bus_plate,sm->inbound_bus_plate);
        break;
      case OUTBOUND_VEHICLE:
        bus_id = sm->outbound_bus_id;
        bus_type = sm->outbound_bus_type;
        break;
      default:
        break;
    }
    sem_post(&(sm->IPC_mutex));

    int final_bus_type;
    switch (transaction_type)
    {
      case INBOUND_VEHICLE: {
          sem_wait(&(sm->logfile));
          logfile = fopen("logfile.txt","a+");
          fprintf(logfile,"Station_manager: Bus of type %s with id %d and plate number %s waiting out of the station to park...\n",busTypes[bus_type],bus_id,bus_plate);
          fclose(logfile);
          sem_post(&(sm->logfile));
          // If the bus is of type VOR or ASK and has not enough space on it's bay, choose 1 from PEL
          if ((bus_type == VOR || bus_type == ASK) && sm->bayBuses[bus_type] >= sm->bayCap[bus_type])
            final_bus_type = PEL;
          else 
            final_bus_type = bus_type; 
          // Give the incoming bus it's final parking bay
          sem_wait(&(sm->IPC_mutex));
          sm->inbound_bus_final_bay = final_bus_type;
          sem_post(&(sm->IPC_mutex));
          // Notify bus to park
          sem_post(&(sm->station_manager_inbound_notification));
          sem_wait(&(sm->logfile));
          logfile = fopen("logfile.txt","a+");
          fprintf(logfile,"Station_manager: Bus of type %s with id %d and plate number %s just arrived at the station and will park in %s bay.\n",busTypes[bus_type],bus_id,bus_plate,busTypes[final_bus_type]);
          fclose(logfile);
          sem_post(&(sm->logfile));
        }
        break;
      case OUTBOUND_VEHICLE:
        // Update ledger
        sem_wait(&(sm->ledger_mutex));
        sm->total_in_station_buses--;
        sm->bayBuses[bus_type]--;
        sem_post(&(sm->ledger_mutex));
        // Free 1 parking slot for departed bus bay
        sem_post(&(sm->bay[bus_type]));
        // Notify bus to leave
        sem_post(&(sm->station_manager_outbound_notification));
        sem_wait(&(sm->logfile));
        logfile = fopen("logfile.txt","a+");
        fprintf(logfile,"Station_manager: Bus with id %d just left the staion.\n",bus_id);
        fclose(logfile);
        sem_post(&(sm->logfile));
        break;
      default:
        break;
    }
  }
  
  sem_wait(&(sm->output));
  printf("Station_manager with pid %d stopped working.\n",getpid());
  sem_post(&(sm->output));
  return 0;
}
