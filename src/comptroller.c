#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

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
    // time
    if (!strcmp(argv[i],"-d")) {
      time = atoi(argv[i+1]);
    }
    // stattimes
    else if (!strcmp(argv[i],"-t")) {
      stattimes = atoi(argv[i+1]);
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

  // Start simulation
  printf("Started comptroller with pid:%d\n",getpid());
  sleep(3);
  printf("Comptroller with pid %d stopped working.\n",getpid());
  return 0;
}
