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
  // Not enough arguments, so specify the correct usage and exit
  if (argc != 17) {
    printf("Usage: ./bus -t type -n incpassengers -c capacity -p parkperiod -m mantime -l plate -s shmid -w wating time\n");
    exit(0);
  }
  FILE *logfile;
  // Get bus id(primary key)
  int id = getpid();
  // Read the arguments
  char type[4],plate[BUS_PLATE_SIZE+1];
  int incpassengers,capacity,parkperiod,mantime,shmid,typeid,waitingtime;
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
          typeid = ASK;
        else if (!strcmp(argv[i+1],"PEL"))
          typeid = PEL;
        else
          typeid = VOR;
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
    // waiting time
    else if (!strcmp(argv[i],"-w")) {
      waitingtime = atoi(argv[i+1]);
    }
    // error
    else {
      printf("Usage: ./bus -t type -n incpassengers -c capacity -p parkperiod -m mantime -l plate -s shmid -w waiting time\n");
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

  // Wait some seconds before getting on the road to make simulation more realistic
  sleep(waitingtime);

  // Seed the rand after waiting to avoid confusions(same random numbers with other buses due to same seed)
  srand(time(NULL));

  // Bus gets on the road after a certain time
  sem_wait(&(sm->logfile));
  logfile = fopen("logfile.txt","a+");
  fprintf(logfile,"Bus %d %s (%s) is on the road\n",id,plate,type);
  fclose(logfile);
  sem_post(&(sm->logfile));

  // Wait for station manager inbound vehicle availability
  sem_wait(&(sm->inbound_vehicle));

  // Wait until there is parking space available for the incoming bus depending on the type and it's priority
  if (typeid == VOR || typeid == ASK) {
    while (sem_trywait(&(sm->bay[typeid])) == -1 && sem_trywait(&(sm->bay[PEL])) == -1);
  } else if (typeid == PEL) {
    while (sem_trywait(&(sm->bay[typeid])) == -1);
  }

  // Wait for station manager availability
  sem_wait(&(sm->station_manager));

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

  int finalBay = typeid;
  // Get park bay
  sem_wait(&(sm->IPC_mutex));
  finalBay = sm->inbound_bus_final_bay;
  sem_post(&(sm->IPC_mutex));

  // Start parking process (waits for mantime seconds)
  sem_wait(&(sm->logfile));
  logfile = fopen("logfile.txt","a+");
  fprintf(logfile,"Bus %s (%s) starts parking process on bay %s.\n",plate,type,busTypes[finalBay]);
  fclose(logfile);
  sem_post(&(sm->logfile));
  sleep(mantime);

  // Arrive at the indicated bay and land passengers
  sem_wait(&(sm->logfile));
  logfile = fopen("logfile.txt","a+");
  fprintf(logfile,"Bus %s (%s) just parked.%d passengers landed\n",plate,type,incpassengers);
  fclose(logfile);
  sem_post(&(sm->logfile));

  // Update ledger for arrivals
  sem_wait(&(sm->ledger_mutex));
  sm->total_in_station_buses++;
  sm->bayBuses[finalBay]++;
  sm->total_passengers += incpassengers;
  sm->total_landed_passengers += incpassengers;
  sm->bus_landed_passengers[finalBay]+=incpassengers;
  sem_post(&(sm->ledger_mutex));

  // Make station manager available
  sem_post(&(sm->station_manager));

  // Finish parking process
  sem_post(&(sm->inbound_vehicle));

  // Waiting in station time
  sleep(parkperiod);

  // Board passengers
  int boardedPassengers = rand() % (capacity + 1);
  sem_wait(&(sm->logfile));
  logfile = fopen("logfile.txt","a+");
  fprintf(logfile,"Bus %s (%s) boarded %d passengers...\n",plate,type,boardedPassengers);
  fclose(logfile);
  sem_post(&(sm->logfile));

  // Update ledger for boarded passengers
  sem_wait(&(sm->ledger_mutex));
  sm->total_boarded_passengers += boardedPassengers;
  sem_post(&(sm->ledger_mutex));

  // Wait for station manager outbound vehicle availability
  sem_wait(&(sm->outbound_vehicle));

  // Wait for station manager availability
  sem_wait(&(sm->station_manager));

  // Copy appropriate transaction variables to shared segment
  sem_wait(&(sm->IPC_mutex));
  sm->bus_transaction_type = OUTBOUND_VEHICLE;
  sm->outbound_bus_id = id;
  sm->outbound_bus_type = finalBay;
  sem_post(&(sm->IPC_mutex));

  // Send transaction request to station manager
  sem_post(&(sm->vehicle_transaction));

  // Wait for outbound notification from station manager
  sem_wait(&(sm->station_manager_outbound_notification));

  // Start departure process (waits for mantime seconds)
  sem_wait(&(sm->logfile));
  logfile = fopen("logfile.txt","a+");
  fprintf(logfile,"Bus %s (%s) departed.\n",plate,type);
  fclose(logfile);
  sem_post(&(sm->logfile));
  sleep(mantime);

  // Make station manager available
  sem_post(&(sm->station_manager));

  // Finish departure process
  sem_post(&(sm->outbound_vehicle));

  // End of simulation

  // Decrement shared memory bus count, calculate new average times and increase total_completely_served_buses
  sem_wait(&(sm->ledger_mutex));
  sm->bus_count--;
  sm->average_bus_turnaround_time *= sm->total_completely_served_buses;
  sm->average_bus_turnaround_time += 2*mantime + parkperiod;
  sm->average_bus_park_time *= sm->total_completely_served_buses;
  sm->average_bus_park_time += parkperiod;
  sm->average_bus_type_turnaround_time[finalBay] *= sm->completely_served_buses_per_type[finalBay];
  sm->average_bus_type_turnaround_time[finalBay] += 2*mantime + parkperiod;
  sm->total_completely_served_buses++;
  sm->completely_served_buses_per_type[finalBay]++;
  sm->average_bus_turnaround_time /= sm->total_completely_served_buses;
  sm->average_bus_park_time /= sm->total_completely_served_buses;
  sm->average_bus_type_turnaround_time[finalBay] /= sm->completely_served_buses_per_type[finalBay];
  sem_post(&(sm->ledger_mutex));

  sem_wait(&(sm->logfile));
  logfile = fopen("logfile.txt","a+");
  fprintf(logfile,"Bus %d %s (%s) finished it's job.\n",id,plate,type);
  fclose(logfile);
  sem_post(&(sm->logfile));
  return 0;
}
