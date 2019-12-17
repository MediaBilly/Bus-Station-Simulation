#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <semaphore.h>
#include "../headers/constants.h"

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
  void *sm;
  if ((sm = shmat(shmid,NULL,0)) == (void*)-1) {
    perror("Error attaching shared memory segment to station-manager:");
    exit(1);
  }
  // Get pointers to needed shared memory variables and semaphores
  // Semaphores
  sem_t* vehicle_transaction = (sem_t*)sm;

  // Station status variables
  int *bayCap = (int*)(sm + 8*sizeof(sem_t));
  int *bayBuses = (int*)(sm + 8*sizeof(sem_t) + BAYS*sizeof(int));
  int totalCap = 0;
  int i;
  for (i = 0;i < BAYS;i++)
    totalCap += bayCap[i];
  int *busDepartedPassengers = (int*)(sm + 8*sizeof(sem_t) + 2*BAYS*sizeof(int));
  int *bus = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int));

  // Interest statistics variables
  int *total_passengers = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*sizeof(int));
  int *total_departed_passengers = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*sizeof(int) + sizeof(int));
  int *total_boarded_passengers = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*sizeof(int) + 2*sizeof(int));
  int *total_in_station_buses = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*sizeof(int) + 3*sizeof(int));
  int *total_completely_served_buses = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*sizeof(int) + 4*sizeof(int));
  double *average_bus_turnaround_time = (double*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*sizeof(int) + 5*sizeof(int));
  double *average_bus_park_time = (double*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*sizeof(int) + 5*sizeof(int) + sizeof(double));
  double *average_bus_type_turnaround_time = (double*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*sizeof(int) + 5*sizeof(int) + 2*sizeof(double));

  // IPC 
  char* bus_transaction_type = (char*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*sizeof(int) + 5*sizeof(int) + 5*sizeof(double));
  int* bus_to_park_id = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*sizeof(int) + 5*sizeof(int) + 5*sizeof(double) + sizeof(char));
  int* bus_to_park_category = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*sizeof(int) + 6*sizeof(int) + 5*sizeof(double) + sizeof(char));
  int* bus_to_leave_id = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*sizeof(int) + 7*sizeof(int) + 5*sizeof(double) + sizeof(char));
  double* bus_to_leave_waiting_time = (double*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*sizeof(int) + 8*sizeof(int) + 5*sizeof(double) + sizeof(char));
  int* bus_count = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*sizeof(int) + 8*sizeof(int) + 6*sizeof(double) + sizeof(char));

  // Start simulation
  printf("Started station-manager with pid:%d\n",getpid());

  /*
  printf("Station-manager waiting for bus transaction...\n");
  sem_wait(vehicle_transaction);
  
  printf("Station-manager received bus signal.\n");
  */

  printf("Station-manager with pid %d stopped working.\n",getpid());
  return 0;
}
