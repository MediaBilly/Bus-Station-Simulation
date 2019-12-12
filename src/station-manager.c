#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <semaphore.h>

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
  sem_t vehicle_transaction;
  if (sem_init(sm,1,0) != 0) {
    perror("Could not load vehicle_transaction semaphore in station-manager");
    exit(1);
  }

  // Start simulation
  printf("Started station-manager with pid:%d\n",getpid());

  printf("Station-manager waiting for bus transaction...\n");
  sem_wait(sm);
  printf("Station-manager received bus signal.\n");

  printf("Station-manager with pid %d stopped working.\n",getpid());
  return 0;
}
