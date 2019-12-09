#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>

int main(int argc, char const *argv[])
{
  // Not enough arguments, so specify the correct usage and exit
  if (argc != 3) {
    fprintf(stderr,"Usage: ./mystation -l configfile\n");
    exit(0);
  }
  // Check for correctness and exit if not
  if (strcmp(argv[1],"-l")) {
    fprintf(stderr,"Usage: ./mystation -l configfile\n");
    exit(0);
  }
  // Open configuration file
  FILE *configfile;
  // File does not exist.
  if ((configfile = fopen(argv[2],"r")) == NULL) {
    fprintf(stderr,"configfile does not exist.\n");
    exit(0);
  }
  // Read data from configuration file
  int bays;
  // Read num of bays (must be 3)
  if (fscanf(configfile,"%d",&bays) != 1) {
    fprintf(stderr,"Configfile format error.\n");
    fclose(configfile);
    exit(0);
  }
  if (bays != 3) {
    fprintf(stderr,"Only 3 bays are allowed.\n");
    fclose(configfile);
    exit(0);
  }
  // Read capacity for each bay
  int bayCap[bays];
  int i;
  for(i = 0;i < bays;i++) {
    if (fscanf(configfile,"%d",bayCap + i) != 1) {
      fprintf(stderr,"Configfile format error.\n");
      fclose(configfile);
      exit(0);
    }
  }
  // Read bus capacity
  int busCap;
  if (fscanf(configfile,"%d",&busCap) != 1) {
    fprintf(stderr,"Configfile format error.\n");
    fclose(configfile);
    exit(0);
  }
  // Raed vehicle's max parking time
  int maxParkPeriod;
  if (fscanf(configfile,"%d",&maxParkPeriod) != 1) {
    fprintf(stderr,"Configfile format error.\n");
    fclose(configfile);
    exit(0);
  }
  // Create shared memory segment
  int shmid;
  if ((shmid = shmget(IPC_PRIVATE,5*sizeof(sem_t) + 4*sizeof(int) + 2*sizeof(double),0666)) == -1) {
    perror("Error creating shared memory segment:");
    exit(1);
  }
  // Attach shared memory segment
  void *sm;
  if (((void*)shmat(shmid,NULL,0)) == (void*)-1) {
    perror("Error attaching shared memory segment:");
    exit(1);
  }
  // Create semaphores
  // Create rw semaphore
  if (sem_init(sm,1,1) != 0) {
    perror("Could not initialize rw semaphore:");
    exit(1);
  }
  // Create station-manager semaphore
  if (sem_init(sm + 1,1,2) != 0) {
    perror("Could not initialize station-manager semaphore:");
    exit(1);
  }
  // Spawn the other processes
  // Close configuration file
  fclose(configfile);
  // Remove shared memory segment
  if (shmctl(shmid,IPC_RMID,0) == -1) {
    perror("Error removing shared segment:");
    exit(1);
  }
  return 0;
}
