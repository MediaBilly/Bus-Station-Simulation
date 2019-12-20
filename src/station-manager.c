#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <semaphore.h>
#include "../headers/constants.h"
#include "../headers/shared_segment.h"

int main(int argc, char const *argv[])
{
  // Not enough arguments, so specify the correct usage and exit
  if (argc != 3) {
    fprintf(stderr,"Usage: ./station-manager -s shmid\n");
    exit(0);
  }
  // Read arguments
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

  // Start simulation
  printf("Started station-manager with pid:%d\n",getpid());

  int done = 0;
  while (1) {
    // Wait for request from incoming or outcoming bus
    printf("Station-manager waiting for bus transaction...\n");
    sem_wait(&(sm->vehicle_transaction));
    
    // Check if all buses finished their jobs. If so, exit
    sem_wait(&(sm->ledger_mutex));
    if (sm->bus_count == 0)
      done = 1;
    sem_post(&(sm->ledger_mutex));
    if (done)
      break;
    printf("Station-manager received bus signal.\n");

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
        printf("Station manager: Bus of type %s with id %d and plate number %s just arrived at the staion.\n",busTypes[bus_type],bus_id,bus_plate);
        // Notify bus to park
        sem_post(&(sm->station_manager_inbound_notification));
        break;
      case OUTBOUND_VEHICLE:
        printf("Station manager: Bus with id %d just left the staion.\n",bus_id);
        // Notify bus to park
        sem_post(&(sm->station_manager_outbound_notification));
        break;
      default:
        break;
    }
  }

  printf("Station-manager with pid %d stopped working.\n",getpid());
  return 0;
}
