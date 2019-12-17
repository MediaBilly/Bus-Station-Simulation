#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <unistd.h>
#include "../headers/constants.h"

// Shared memory total variables by type

const int SM_TOTAL_SEMAPHORES = 8;
const int SM_TOTAL_INTEGERS = 7;
const int SM_TOTAL_DOUBLES = 3;
const int SM_STRING_BYTES = 22; // Extra bytes for the strings

int main(int argc, char const *argv[])
{
  srand(time(NULL));
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
  int i,totalCap = 0;
  for(i = 0;i < bays;i++) {
    if (fscanf(configfile,"%d",bayCap + i) != 1) {
      fprintf(stderr,"Configfile format error.\n");
      fclose(configfile);
      exit(0);
    } else {
      totalCap += bayCap[i];
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
  // Raed vehicle's max mantime time
  int maxManTime;
  if (fscanf(configfile,"%d",&maxManTime) != 1) {
    fprintf(stderr,"Configfile format error.\n");
    fclose(configfile);
    exit(0);
  }
  // Read total vehicles to be created
  int totalVehicles;
  if (fscanf(configfile,"%d",&totalVehicles) != 1) {
    fprintf(stderr,"Configfile format error.\n");
    fclose(configfile);
    exit(0);
  }

  // Close configuration file
  fclose(configfile);

  // Create shared memory segment
  int shmid;
  if ((shmid = shmget(IPC_PRIVATE,SM_TOTAL_SEMAPHORES*sizeof(sem_t) + SM_TOTAL_INTEGERS*sizeof(int) + 3*BAYS * sizeof(int) + SM_TOTAL_DOUBLES*sizeof(double) + SM_STRING_BYTES + totalCap * BAY_NAME_SIZE,0666)) == -1) {
    perror("Error creating shared memory segment:");
    exit(1);
  }
  // Attach shared memory segment
  void *sm;
  if ((sm = shmat(shmid,NULL,0)) == (void*)-1) {
    perror("Error attaching shared memory segment to mystation:");
    exit(1);
  }

  // Create the semaphores

  // Create vehicle_transaction semaphore
  if (sem_init(sm,1,0) != 0) {
    perror("Could not initialize vehicle_transaction semaphore:");
    exit(1);
  }

  // Create inbound_vehicle semaphore
  if (sem_init(sm + sizeof(sem_t),1,1) != 0) {
    perror("Could not initialize inbound_vehicle semaphore:");
    exit(1);
  }

  // Create outbound_vehicle semaphore
  if (sem_init(sm + 2*sizeof(sem_t),1,1) != 0) {
    perror("Could not initialize outbound_vehicle semaphore:");
    exit(1);
  }

  // Create station_manager_inbound_notification semaphore
  if (sem_init(sm + 3*sizeof(sem_t),1,0) != 0) {
    perror("Could not initialize station_manager_inbound_notification semaphore:");
    exit(1);
  }

  // Create station_manager_outbound_notification semaphore
  if (sem_init(sm + 4*sizeof(sem_t),1,0) != 0) {
    perror("Could not initialize station_manager_outbound_notification semaphore:");
    exit(1);
  }

  // Create ledger_read semaphore
  if (sem_init(sm + 5*sizeof(sem_t),1,1) != 0) {
    perror("Could not initialize ledger_read semaphore:");
    exit(1);
  }

  // Create ledger_write semaphore
  if (sem_init(sm + 6*sizeof(sem_t),1,1) != 0) {
    perror("Could not initialize ledger_write semaphore:");
    exit(1);
  }

  // Create ledger_mutex semaphore
  if (sem_init(sm + 7*sizeof(sem_t),1,1) != 0) {
    perror("Could not initialize ledger_mutex semaphore:");
    exit(1);
  }

  // Initialize other variables to 0
  memset(sm + 8*sizeof(sem_t),0,SM_TOTAL_INTEGERS*sizeof(int) + 3*BAYS * sizeof(int) + SM_TOTAL_DOUBLES*sizeof(double) + SM_STRING_BYTES + totalCap * BAY_NAME_SIZE);

  // Initialize bay caps in shared memory
  int *sm_bayCap = (int*)(sm + 8*sizeof(sem_t));
  for (i = 0;i < bays;i++) {
    sm_bayCap[i] = bayCap[i];
  }

  // Spawn the other processes

  // Spawn the station manager
  pid_t pid;
  // Error
  if ((pid = fork()) == -1) {
    perror("Station-manager creation error:");
    exit(1);
  }
  // Child(station_manager)
  else if (pid == 0) {
    char Shmid[10];
    sprintf(Shmid,"%d",shmid);
    execl("./station-manager","station-manager","-s",Shmid,NULL);
    perror("Station-manager execution error:");
    exit(1);
  }
  // Parent continues spawning the other processes

  // Spawn the comtroller
  // Error
  if ((pid = fork()) == -1) {
    perror("Comptroller creation error:");
    exit(1);
  }
  // Child(comptroller)
  else if (pid == 0) {
    char Shmid[10];
    sprintf(Shmid,"%d",shmid);
    execl("./comptroller","comptroller","-d","1","-t","2","-s",Shmid,NULL);
    perror("Comptroller execution error:");
    exit(1);
  }
  // Parent continues spawning the other processes
  
  // Spawn the buses
  for(i = 0;i < totalVehicles;i++) {
    int typeIndex = rand() % bays;
    // Error
    if ((pid = fork()) == -1) {
      perror("Bus creation error:");
      exit(1);
    }
    // Child(bus)
    else if (pid == 0) {
      char incpassengers[10],capacity[10],parkperiod[10],mantime[10],Shmid[10];
      sprintf(incpassengers,"%d",rand() % busCap);
      sprintf(capacity,"%d",busCap);
      sprintf(parkperiod,"%d",maxParkPeriod);
      sprintf(mantime,"%d",rand() % maxManTime);
      sprintf(Shmid,"%d",shmid);
      // Create id(plate number)
      char plate[ID_PLATE_SIZE + 1];
      for (i = 0;i <= 2;i++) {
        char letter = rand() % ('Z' - 'A') + 'A';
        plate[i] = letter;
      }
      plate[3] = '-';
      for (i = 4;i < ID_PLATE_SIZE;i++) {
        char numb = rand() % ('9' - '0') + '0';
        plate[i]= numb;
      }
      plate[ID_PLATE_SIZE] = '\0';
      execl("./bus","bus","-t",busTypes[typeIndex],"-n",incpassengers,"-c",capacity,"-p",parkperiod,"-m",mantime,"-s",Shmid,"-l",plate,NULL);
      perror("Bus execution error:");
      exit(1);
    }
  }

  // Wait for the processes to finish execution
  for(i = 0;i < totalVehicles + 2;i++) {
    int exit_status;
    if (wait(&exit_status) == -1) {
      perror("Wait failed");
      exit(1);
    }
  }

  // Destroy all the semaphores
  for(i = 0;i < SM_TOTAL_SEMAPHORES;i++) {
    sem_destroy(sm + i*sizeof(sem_t));
  }
  
  // Remove shared memory segment
  if (shmctl(shmid,IPC_RMID,0) == -1) {
    perror("Error removing shared segment:");
    exit(1);
  }
  return 0;
}
