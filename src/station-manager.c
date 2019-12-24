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
  printf("Started station-manager with pid:%d\n",getpid());
  sem_post(&(sm->output));

  while (1) {
    // Wait for request from incoming or outcoming bus
    sem_wait(&(sm->output));
    printf("Station-manager waiting for bus transaction...\n");
    sem_post(&(sm->output));

    sem_wait(&(sm->vehicle_transaction));
    // Check if we received done signal
    if (done) 
      break;

    sem_wait(&(sm->output));
    printf("Station-manager received bus signal.\n");
    sem_post(&(sm->output));

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
        break;
      default:
        break;
    }
    sem_post(&(sm->IPC_mutex));

    switch (transaction_type)
    {
      case INBOUND_VEHICLE:
        // TODO: if there is enough parking space do the following. (if not enough parking space, continue;)
        // For the moment, infinite parking places exist
        sem_wait(&(sm->output));
        printf("Station manager: Bus of type %s with id %d and plate number %s just arrived at the station.\n",busTypes[bus_type],bus_id,bus_plate);
        sem_post(&(sm->output));
        // Notify bus to park
        sem_post(&(sm->station_manager_inbound_notification));
        break;
      case OUTBOUND_VEHICLE:
        sem_wait(&(sm->output));
        printf("Station manager: Bus with id %d just left the staion.\n",bus_id);
        sem_post(&(sm->output));
        // Notify bus to park
        sem_post(&(sm->station_manager_outbound_notification));
        break;
      default:
        break;
    }
    // Make station manager available
    sem_post(&(sm->station_manager));
  }
  
  sem_wait(&(sm->output));
  printf("Station-manager with pid %d stopped working.\n",getpid());
  sem_post(&(sm->output));
  return 0;
}
