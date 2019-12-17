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
  if (argc != 15) {
    printf("Usage: ./bus -t type -n incpassengers -c capacity -p parkperiod -m mantime -l plate -s shmid\n");
    exit(0);
  }
  // Get bus id(primary key)
  int id = getpid();
  // Read the arguments
  char type[4],plate[ID_PLATE_SIZE+1];
  int incpassengers,capacity,parkperiod,mantime,shmid;
  int i;
  for(i = 1;i < argc;i+=2) {
    // type
    if (!strcmp(argv[i],"-t")) {
      if (strlen(argv[i+1]) != 3 || (strcmp(argv[i+1],"ASK") && strcmp(argv[i+1],"PEL") && strcmp(argv[i+1],"VOR"))) {
        fprintf(stderr,"Bus type must be ASK, PEL or VOR.\n");
        exit(0);
      } else {
        strcpy(type,argv[i+1]);
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
      parkperiod = atoi(argv[i+1]);
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
  void *sm;
  if ((sm = shmat(shmid,NULL,0)) == (void*)-1) {
    perror("Error attaching shared memory segment to bus:");
    exit(1);
  }

  // Get pointers to needed shared memory variables and semaphores

  // Semaphores
  sem_t* vehicle_transaction = (sem_t*)sm;
  sem_t *ledger_mutex = (sem_t*)(sm + 7*sizeof(sem_t));

  // ledger specific(only for offset calculations)
  int *bayCap = (int*)(sm + 8*sizeof(sem_t));
  int totalCap = 0;
  for (i = 0;i < BAYS;i++)
    totalCap += bayCap[i];

  // IPC helpers
  int* bus_count = (int*)(sm + 8*sizeof(sem_t) + 3*BAYS * sizeof(int) + totalCap*sizeof(int) + 8*sizeof(int) + 6*sizeof(double) + sizeof(char));

  // Increment shared memory bus count
  sem_wait(ledger_mutex);
  (*bus_count)++;
  sem_post(ledger_mutex);

  // Start simulation
  printf("Bus %d %s (%s) is on the road\n",id,plate,type);

  sleep(rand() % 20);

  // sem_post(vehicle_transaction);

  // End of simulation

  // Decrement shared memory bus count
  sem_wait(ledger_mutex);
  (*bus_count)--;
  sem_post(ledger_mutex);
  printf("Bus %s (%s) finished it's job.\n",plate,type);
  return 0;
}
