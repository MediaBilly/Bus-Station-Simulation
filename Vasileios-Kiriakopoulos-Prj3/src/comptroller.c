#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <signal.h>
#include "../headers/constants.h"
#include "../headers/shared_segment.h"

// Handles done signal
int done = 0;
void signal_handler(int signum) {
    if (signum == SIGUSR2) {
      done = 1;
    }
}

int main(int argc, char const *argv[])
{
  // Register stop signal
  signal(SIGUSR2,signal_handler);
  // Not enough arguments, so specify the correct usage and exit
  if (argc != 7) {
    fprintf(stderr,"Usage: ./comptroller -d time -t stattimes -s shmid\n");
    exit(0);
  }
  // Read arguments
  int time,stattimes,shmid;
  int i;
  for(i = 1;i < argc;i+=2) {
    // Additional constraint: times must be >= 1
    // time
    if (!strcmp(argv[i],"-d")) {
      time = atoi(argv[i+1]);
      if (time < 1) {
        fprintf(stderr,"time must be >= 1\n");
        exit(0);
      }
    }
    // stattimes
    else if (!strcmp(argv[i],"-t")) {
      stattimes = atoi(argv[i+1]);
      if (stattimes < 1) {
        fprintf(stderr,"stattimes must be >= 1\n");
        exit(0);
      }
    }
    // shmid
    else if (!strcmp(argv[i],"-s")) {
      shmid = atoi(argv[i+1]);
    }
    // Error
    else {
      fprintf(stderr,"Usage: ./comptroller -d time -t stattimes -s shmid\n");
      exit(0);
    }
  }

  // Attach shared memory segment
  Shared_segment *sm;
  if ((sm = shmat(shmid,NULL,0)) == (void*)-1) {
    perror("Error attaching shared memory segment to comptroller:");
    exit(1);
  }

  int totalCap = 0;
  for (i = 0;i < BAYS;i++)
    totalCap += sm->bayCap[i];

  // Start simulation
  sem_wait(&(sm->output));
  printf("Started comptroller with pid:%d\n",getpid());
  sem_post(&(sm->output));
  int seconds_elapsed = 0;
  while (!done) {
    // Time to print station staus
    if (seconds_elapsed % time == 0) {
      // Lock ledger mutex
      sem_wait(&(sm->ledger_mutex));

      // Print stuff
      sem_wait(&(sm->output));
      printf("Station status:\n");
      int parked_buses=0,landed_passengers = 0;
      for (i = 0;i < BAYS;i++) {
        printf("\t-%s:\n",busTypes[i]);
        printf("\t\t* Parked buses:%d\n",sm->bayBuses[i]);
        parked_buses += sm->bayBuses[i];
        printf("\t\t* Free bus parking positions:%d\n",sm->bayCap[i] - sm->bayBuses[i]);
        printf("\t\t* Landed passengers:%d\n",sm->bus_landed_passengers[i]);
        landed_passengers += sm->bus_landed_passengers[i];
      }
      printf("\t Total parked buses:%d\n",parked_buses);
      printf("\t Total landed passengers:%d\n",landed_passengers);
      printf("\n");
      sem_post(&(sm->output));

      // Unlock ledger mutex
      sem_post(&(sm->ledger_mutex));
    }

    // Time to print statistics
    if (seconds_elapsed % stattimes == 0) {
      // Lock ledger mutex
      sem_wait(&(sm->ledger_mutex));

      sem_wait(&(sm->output));
      // Print stuff
      printf("Statistics:\n");
      printf("\tTotal passengers served in station:%d\n",sm->total_passengers);
      printf("\tTotal passengers departed to station:%d\n",sm->total_landed_passengers);
      printf("\tTotal passengers boarded from the station:%d\n",sm->total_boarded_passengers);
      printf("\tTotal buses entered the station:%d\n",sm->total_in_station_buses);
      printf("\tTotal buses served completely from the station:%d\n",sm->total_completely_served_buses);
      printf("\tAverage in-station bus time:%lf\n",sm->average_bus_turnaround_time);
      printf("\tAverage bus park time:%lf\n",sm->average_bus_park_time);
      printf("\tAverage in-station bus time per category:\n");
      for(i = 0;i < BAYS;i++) {
        printf("\t\t- %s:%lf\n",busTypes[i],sm->average_bus_type_turnaround_time[i]);
      }
      printf("\n");
      sem_post(&(sm->output));

      // Unlock ledger mutex
      sem_post(&(sm->ledger_mutex));
    }

    sleep(1);
    seconds_elapsed++;
  }
  
  sem_wait(&(sm->output));
  printf("Comptroller with pid %d stopped working.\n",getpid());
  sem_post(&(sm->output));
  return 0;
}
