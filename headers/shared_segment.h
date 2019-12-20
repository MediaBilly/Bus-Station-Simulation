#ifndef SHARED_SEGMENT_H
#define SHARED_SEGMENT_H

#include <semaphore.h>
#include "../headers/constants.h"

#define TOTAL_SEMAPHORES 9

typedef struct shared_segment
{
  // Semaphores
  //sem_t station_manager;
  sem_t vehicle_transaction;
  sem_t inbound_vehicle;
  sem_t outbound_vehicle;
  sem_t station_manager_inbound_notification;
  sem_t station_manager_outbound_notification;
  sem_t ledger_read;
  sem_t ledger_write;
  sem_t ledger_mutex;
  sem_t IPC_mutex;

  // Station_status_variables
  int bayCap[3];
  int bayBuses[3];
  int bus_landed_passengers[3];

  // Interest statistics
  int total_passengers;
  int total_landed_passengers;
  int total_boarded_passengers;
  int total_in_station_buses;
  int total_completely_served_buses;
  double average_bus_turnaround_time;
  double average_bus_park_time;
  double average_bus_type_turnaround_time[3];

  // IPC Helpers
  char bus_transaction_type;
  int inbound_bus_id;
  char inbound_bus_plate[BUS_PLATE_SIZE+1];
  int inbound_bus_type;
  int outbound_bus_id;
  double outbound_bus_waiting_time;
  int bus_count;
} Shared_segment;

//void Shared_Segment_Init(Shared_segment*);

#endif