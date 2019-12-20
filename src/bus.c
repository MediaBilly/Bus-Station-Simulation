#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <semaphore.h>
#include "../headers/constants.h"
#include "../headers/shared_segment.h"

int main(int argc, char const *argv[])
{
  srand(time(NULL));
  // Not enough arguments, so specify the correct usage and exit
  if (argc != 15) {
    printf("Usage: ./bus -t type -n incpassengers -c capacity -p parkperiod -m mantime -l plate -s shmid\n");
    exit(0);
  }
  // Get bus id(primary key)
  int id = getpid();
  // Read the arguments
  char type[4],plate[BUS_PLATE_SIZE+1];
  int incpassengers,capacity,parkperiod,mantime,shmid,typeid;
  int i;
  for(i = 1;i < argc;i+=2) {
    // type
    if (!strcmp(argv[i],"-t")) {
      if (strlen(argv[i+1]) != 3 || (strcmp(argv[i+1],"ASK") && strcmp(argv[i+1],"PEL") && strcmp(argv[i+1],"VOR"))) {
        fprintf(stderr,"Bus type must be ASK, PEL or VOR.\n");
        exit(0);
      } else {
        strcpy(type,argv[i+1]);
        if (!strcmp(argv[i+1],"ASK"))
          typeid = 0;
        else if (!strcmp(argv[i+1],"PEL"))
          typeid = 1;
        else
          typeid = 2;
      }
    }
    // incpassengers
    else if (!strcmp(argv[i],"-n")) {
      incpassengers = atoi(argv[i+1]);
    }
    // capacity
    else if (!strcmp(argv[i],"-c")) {
      capacity = atoi(argv[i+1]);
    }
    // parkperiod
    else if (!strcmp(argv[i],"-p")) {
      parkperiod = atoi(argv[i+1]);
    }
    // mantime
    else if (!strcmp(argv[i],"-m")) {
      mantime = atoi(argv[i+1]);
    }
    // id
    else if (!strcmp(argv[i],"-l")) {
      strcpy(plate,argv[i+1]);
    }
    // shmid
    else if (!strcmp(argv[i],"-s")) {
      shmid = atoi(argv[i+1]);
    }
    // error
    else {
      printf("Usage: ./bus -t type -n incpassengers -c capacity -p parkperiod -m mantime -l plate -s shmid\n");
      exit(0);
    }
  }

  // Attach shared memory segment
  Shared_segment *sm;
  if ((sm = shmat(shmid,NULL,0)) == (void*)-1) {
    perror("Error attaching shared memory segment to bus:");
    exit(1);
  }

  // Increment shared memory bus count
  sem_wait(&(sm->ledger_mutex));
  sm->bus_count++;
  sem_post(&(sm->ledger_mutex));

  // Start simulation

  // Bus gets on the road after a certain time 
  sleep(rand() % 20);
  printf("Bus %d %s (%s) is on the road\n",id,plate,type);

  // Wait for station manager inbound vehicle availability
  sem_wait(&(sm->inbound_vehicle));

  // Notify station manager to park inside

  // Copy appropriate transaction variables to shared segment
  sem_wait(&(sm->IPC_mutex));
  sm->bus_transaction_type = INBOUND_VEHICLE;
  sm->inbound_bus_id = id;
  strcpy(sm->inbound_bus_plate,plate);
  sm->inbound_bus_type = typeid;
  sem_post(&(sm->IPC_mutex));

  // Send transaction request to station manager
  sem_post(&(sm->vehicle_transaction));

  // Wait for inbound notification from station manager
  sem_wait(&(sm->station_manager_inbound_notification));

  // Start parking process (waits for mantime seconds)
  printf("Bus %s (%s) starts parking process.\n",plate,type);
  sleep(mantime);

  // Arrive at the indicated bay and land passengers
  printf("Bus %s (%s) just parked.%d passengers landed\n",plate,type,incpassengers);

  // Update ledger for arrivals
  sem_wait(&(sm->ledger_mutex));
  sm->total_passengers += incpassengers;
  sm->total_landed_passengers += incpassengers;
  sm->total_in_station_buses++;
  sm->bayBuses[typeid]++;
  sm->bus_landed_passengers[typeid]+=incpassengers;
  sem_post(&(sm->ledger_mutex));

  // Finish parking process
  sem_post(&(sm->inbound_vehicle));

  // Waiting in station time
  sleep(parkperiod);

  // Board passengers
  int boardedPassengers = rand() % (capacity + 1);
  printf("Bus %s (%s) boarded %d passengers...\n",plate,type,boardedPassengers);

  // Update ledger for boarded passengers
  sem_wait(&(sm->ledger_mutex));
  sm->total_boarded_passengers += boardedPassengers;
  sem_post(&(sm->ledger_mutex));

  // Wait for station manager outbound vehicle availability
  sem_wait(&(sm->outbound_vehicle));

  // Copy appropriate transaction variables to shared segment
  sem_wait(&(sm->IPC_mutex));
  sm->bus_transaction_type = OUTBOUND_VEHICLE;
  sm->outbound_bus_id = id;
  sem_post(&(sm->IPC_mutex));

  // Send transaction request to station manager
  sem_post(&(sm->vehicle_transaction));

  // Wait for outbound notification from station manager
  sem_wait(&(sm->station_manager_outbound_notification));

  // Start departure process (waits for mantime seconds)
  printf("Bus %s (%s) departed.\n",plate,type);
  sleep(mantime);

  // Update ledger
  sem_wait(&(sm->ledger_mutex));
  sm->total_in_station_buses--;
  sm->bayBuses[typeid]--;
  sem_post(&(sm->ledger_mutex));

  // Finish departure process
  sem_post(&(sm->outbound_vehicle));

  // End of simulation

  // Decrement shared memory bus count
  sem_wait(&(sm->ledger_mutex));
  sm->bus_count--;
  // If it was the last bus, notify the station manager to check the bus_count and stop execution
  if (sm->bus_count == 0)
    sem_post(&(sm->vehicle_transaction));
  sem_post(&(sm->ledger_mutex));
  printf("Bus %s (%s) finished it's job.\n",plate,type);
  return 0;
}
