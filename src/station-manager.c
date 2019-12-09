#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../headers/station-manager.h"

int main(int argc, char const *argv[])
{
  // Not enough arguments, so specify the correct usage and exit
  if (argc != 3) {
    fprintf(stderr,"Usage: ./station-manager -s shmid\n");
    exit(0);
  }
  // Read arguments
  int i,shmid;
  // Error
  if (strcmp(argv[1],"-s")) {
    fprintf(stderr,"Usage: ./station-manager -s shmid\n");
    exit(0);
  } else {
    // Read shared segment id
    shmid = atoi(argv[2]);
  }
  // Attach shared memory segment
  // Start simulation
  printf("Started station-manager with pid:%d\n",getpid());
  sleep(5);
  printf("Station-manager with pid %d stopped working.\n",getpid());
  return 0;
}
