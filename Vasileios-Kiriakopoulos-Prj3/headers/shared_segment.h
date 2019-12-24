#ifndef SHARED_SEGMENT_H
#define SHARED_SEGMENT_H

#include <semaphore.h>
#include "../headers/constants.h"

#define TOTAL_SEMAPHORES 10 + BAYS

typedef struct shared_segment
{
  // Semaphores
  sem_t station_manager;
  sem_t vehicle_transaction;
  sem_t inbound_vehicle;
  sem_t outbound_vehicle;
  sem_t station_manager_inbound_notification;
  sem_t station_manager_outbound_notification;
  sem_t ledger_mutex;
  sem_t IPC_mutex;
  sem_t output;
  sem_t logfile;
  sem_t bay[BAYS];

  // Station_status_variables
  int bayCap[BAYS];
  int bayBuses[BAYS];
  int bus_landed_passengers[BAYS];

  // Interest statistics
  int total_passengers;
  int total_landed_passengers;
  int total_boarded_passengers;
  int total_in_station_buses;
  int total_completely_served_buses;
  double average_bus_turnaround_time;
  double average_bus_park_time;
  double average_bus_type_turnaround_time[BAYS];
  int completely_served_buses_per_type[BAYS];

  // IPC Helpers
  char bus_transaction_type;
  int inbound_bus_id;
  char inbound_bus_plate[BUS_PLATE_SIZE+1];
  int inbound_bus_type;
  int inbound_bus_final_bay;
  int outbound_bus_id;
  int outbound_bus_type;
  double outbound_bus_waiting_time;
  int bus_count;
} Shared_segment;

#endif