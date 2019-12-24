#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include "../headers/constants.h"
#include "../headers/shared_segment.h"

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
  // Read comptroller's time
  int comptroller_time;
  if (fscanf(configfile,"%d",&comptroller_time) != 1) {
    fprintf(stderr,"Configfile format error.\n");
    fclose(configfile);
    exit(0);
  }
  // Read comptroller's stattimes
  int comptroller_stattimes;
  if (fscanf(configfile,"%d",&comptroller_stattimes) != 1) {
    fprintf(stderr,"Configfile format error.\n");
    fclose(configfile);
    exit(0);
  }

  // Close configuration file
  fclose(configfile);

  // Create shared memory segment
  int shmid;
  if ((shmid = shmget(IPC_PRIVATE,sizeof(Shared_segment),0666)) == -1) {
    perror("Error creating shared memory segment:");
    exit(1);
  }
  // Attach shared memory segment
  Shared_segment *sm;
  if ((sm = shmat(shmid,NULL,0)) == (void*)-1) {
    perror("Error attaching shared memory segment to mystation:");
    exit(1);
  }

  // Create the semaphores

  // Create station_manager semaphore
  if (sem_init(&(sm->station_manager),1,1) != 0) {
    perror("Could not initialize station_manager semaphore:");
    exit(1);
  } 

  // Create vehicle_transaction semaphore
  if (sem_init(&(sm->vehicle_transaction),1,0) != 0) {
    perror("Could not initialize vehicle_transaction semaphore:");
    exit(1);
  } 

  // Create inbound_vehicle semaphore
  if (sem_init(&(sm->inbound_vehicle),1,1) != 0) {
    perror("Could not initialize inbound_vehicle semaphore:");
    exit(1);
  }

  // Create outbound_vehicle semaphore
  if (sem_init(&(sm->outbound_vehicle),1,1) != 0) {
    perror("Could not initialize outbound_vehicle semaphore:");
    exit(1);
  }

  // Create station_manager_inbound_notification semaphore
  if (sem_init(&(sm->station_manager_inbound_notification),1,0) != 0) {
    perror("Could not initialize station_manager_inbound_notification semaphore:");
    exit(1);
  }

  // Create station_manager_outbound_notification semaphore
  if (sem_init(&(sm->station_manager_outbound_notification),1,0) != 0) {
    perror("Could not initialize station_manager_outbound_notification semaphore:");
    exit(1);
  }

  // Create ledger_read semaphore
  if (sem_init(&(sm->ledger_read),1,1) != 0) {
    perror("Could not initialize ledger_read semaphore:");
    exit(1);
  }

  // Create ledger_write semaphore
  if (sem_init(&(sm->ledger_write),1,1) != 0) {
    perror("Could not initialize ledger_write semaphore:");
    exit(1);
  }

  // Create ledger_mutex semaphore
  if (sem_init(&(sm->ledger_mutex),1,1) != 0) {
    perror("Could not initialize ledger_mutex semaphore:");
    exit(1);
  }

  // Create IPC_mutex semaphore
  if (sem_init(&(sm->IPC_mutex),1,1) != 0) {
    perror("Could not initialize IPC_mutex semaphore:");
    exit(1);
  }

  // Create output semaphore
  if (sem_init(&(sm->output),1,1) != 0) {
    perror("Could not initialize output semaphore:");
    exit(1);
  }

  // Initialize bay caps in shared memory
  for (i = 0;i < bays;i++) {
    sm->bayCap[i] = bayCap[i];
  }

  // Spawn the other processes

  // Spawn the station manager
  pid_t station_manager;
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
  } else {
    station_manager = pid;
  }
  // Parent continues spawning the other processes

  // Spawn the comptroller
  pid_t comptroller;
  // Error
  if ((pid = fork()) == -1) {
    perror("Comptroller creation error:");
    exit(1);
  }
  // Child(comptroller)
  else if (pid == 0) {
    char Shmid[10];
    char time[10];
    char stattimes[10];
    sprintf(Shmid,"%d",shmid);
    sprintf(time,"%d",comptroller_time);
    sprintf(stattimes,"%d",comptroller_stattimes);
    execl("./comptroller","comptroller","-d",time,"-t",stattimes,"-s",Shmid,NULL);
    perror("Comptroller execution error:");
    exit(1);
  } else {
    comptroller = pid;
  }
  // Parent continues spawning the other processes
  
  pid_t busPid[totalVehicles];
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
      char incpassengers[10],capacity[10],parkperiod[10],mantime[10],Shmid[10],waitingtime[10];
      sprintf(incpassengers,"%d",rand() % busCap);
      sprintf(capacity,"%d",busCap);
      sprintf(parkperiod,"%d",rand() % maxParkPeriod);
      sprintf(mantime,"%d",rand() % maxManTime);
      sprintf(Shmid,"%d",shmid);
      sprintf(waitingtime,"%d",rand() % 10);
      // Create id(plate number)
      char plate[BUS_PLATE_SIZE + 1];
      for (i = 0;i <= 2;i++) {
        char letter = rand() % ('Z' - 'A') + 'A';
        plate[i] = letter;
      }
      plate[3] = '-';
      for (i = 4;i < BUS_PLATE_SIZE;i++) {
        char numb = rand() % ('9' - '0') + '0';
        plate[i]= numb;
      }
      plate[BUS_PLATE_SIZE] = '\0';
      execl("./bus","bus","-t",busTypes[typeIndex],"-n",incpassengers,"-c",capacity,"-p",parkperiod,"-m",mantime,"-s",Shmid,"-l",plate,"-w",waitingtime,NULL);
      perror("Bus execution error:");
      exit(1);
    } else {
      busPid[i] = pid;
    }
  }

  int exit_status;
  // Wait for the buses to finish execution
  for(i = 0;i < totalVehicles;i++) {
    while (waitpid(busPid[i],&exit_status,0) == 0) ;
  }

  // Tell the comptroller to stop execution
  kill(comptroller,SIGUSR2);

  // Wait for him to finish execution
  while (waitpid(comptroller,&exit_status,0) == 0) ;

  // Tell the station_manager to stop execution
  kill(station_manager,SIGUSR2);

  // Wait for him to completely finish
  while (waitpid(station_manager,&exit_status,0) == 0) ;

  // Destroy all the semaphores
  for(i = 0;i < TOTAL_SEMAPHORES;i++) {
    sem_destroy((void*)sm + i*sizeof(sem_t));
  }
  
  // Remove shared memory segment
  if (shmctl(shmid,IPC_RMID,0) == -1) {
    perror("Error removing shared segment:");
    exit(1);
  }
  return 0;
}
