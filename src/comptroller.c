#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include "../headers/constants.h"

int main(int argc, char const *argv[])
{
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
  void *sm;
  if ((sm = shmat(shmid,NULL,0)) == (void*)-1) {
    perror("Error attaching shared memory segment to comptroller:");
    exit(1);
  }

  // Get pointers to needed shared memory variables and semaphores

  // Semaphores
  sem_t *ledger_mutex = (sem_t*)(sm + 7*sizeof(sem_t));

  // Station status variables
  int *bayCap = (int*)(sm + 8*sizeof(sem_t));
  int *bayBuses = (int*)(sm + 8*sizeof(sem_t) + BAYS*sizeof(int));
  int totalCap = 0;
  for (i = 0;i < BAYS;i++)
    totalCap += bayCap[i];
  int *busDepartedPassengers = (int*)(sm + 8*sizeof(sem_t) + 2*BAYS*sizeof(int));
  char (*bus)[totalCap][BAY_NAME_SIZE] = (char*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int));

  // Interest statistics variables
  int *total_passengers = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*BAY_NAME_SIZE*sizeof(char));
  int *total_departed_passengers = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*BAY_NAME_SIZE*sizeof(char) + sizeof(int));
  int *total_boarded_passengers = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*BAY_NAME_SIZE*sizeof(char) + 2*sizeof(int));
  int *total_in_station_buses = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*BAY_NAME_SIZE*sizeof(char) + 3*sizeof(int));
  int *total_completely_served_buses = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*BAY_NAME_SIZE*sizeof(char) + 4*sizeof(int));
  double *average_bus_turnaround_time = (double*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*BAY_NAME_SIZE*sizeof(char) + 5*sizeof(int));
  double *average_bus_park_time = (double*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*BAY_NAME_SIZE*sizeof(char) + 5*sizeof(int) + sizeof(double));
  double *average_bus_type_turnaround_time = (double*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*BAY_NAME_SIZE*sizeof(char) + 5*sizeof(int) + 2*sizeof(double));

  // Start simulation
  printf("Started comptroller with pid:%d\n",getpid());
  printf("Total cap is:%d\n",totalCap);
  clock_t start = clock();
  int seconds_elapsed = 0;
  while (1) {
    // Time to print station staus
    if (seconds_elapsed % time == 0) {
      // Lock ledger mutex
      sem_wait(ledger_mutex);

      // Print stuff
      printf("%d Station status:\n",seconds_elapsed);
      int parked_buses=0,departed_passengers = 0;
      for (i = 0;i < BAYS;i++) {
        printf("\t-%s:\n",busTypes[i]);
        printf("\t\t* Parked buses:%d\n",bayBuses[i]);
        parked_buses += bayBuses[i];
        printf("\t\t* Free bus parking positions:%d\n",bayCap[i] - bayBuses[i]);
        printf("\t\t* Departed passengers:%d\n",busDepartedPassengers[i]);
        departed_passengers += busDepartedPassengers[i];
      }
      printf("\t Total parked buses:%d\n",parked_buses);
      printf("\t Total departed passengers:%d\n",departed_passengers);
      printf("\n");

      // Unlock ledger mutex
      sem_post(ledger_mutex);
    }

    // Time to print statistics
    if (seconds_elapsed % stattimes == 0) {
      // Lock ledger mutex
      sem_wait(ledger_mutex);

      // Print stuff
      printf("%d Statistics:\n",seconds_elapsed);
      printf("\tTotal passengers served in station:%d\n",*total_passengers);
      printf("\tTotal passengers departed to station:%d\n",*total_departed_passengers);
      printf("\tTotal passengers boarded from the station:%d\n",*total_boarded_passengers);
      printf("\tTotal buses entered the station:%d\n",*total_in_station_buses);
      printf("\tTotal buses served completely from the station:%d\n",*total_completely_served_buses);
      printf("\tAverage in-station bus time:%lf\n",*average_bus_turnaround_time);
      printf("\tAverage bus park time:%lf\n",*average_bus_park_time);
      printf("\tAverage in-station bus time per category:\n");
      for(i = 0;i < BAYS;i++) {
        printf("\t\t- %s:%lf\n",busTypes[i],average_bus_type_turnaround_time[i]);
      }
      printf("\n");

      // Unlock ledger mutex
      sem_post(ledger_mutex);
    }

    sleep(1);
    seconds_elapsed++;
  }
  
  printf("Comptroller with pid %d stopped working.\n",getpid());
  return 0;
}
