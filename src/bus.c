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
  if (argc != 13) {
    printf("Usage: ./bus -t type -n incpassengers -c capacity -p parkperiod -m mantime -s shmid\n");
    exit(0);
  }
  // Read the arguments
  char type[3];
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
    // shmid
    else if (!strcmp(argv[i],"-s")) {
      shmid = atoi(argv[i+1]);
    }
    // error
    else {
      printf("Usage: ./bus -t type -n incpassengers -c capacity -p parkperiod -m mantime -s shmid\n");
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

  sleep(5);

  sem_post(vehicle_transaction);

  // Start simulation
  printf("Started bus with pid:%d\n",getpid());
  printf("Bus with pid %d stopped working.\n",getpid());
  return 0;
}
