#ifndef SHARED_SEGMENT_H
#define SHARED_SEGMENT_H

#include <semaphore.h>

typedef struct shared_segment
{
  // Semaphores
  sem_t vehicle_transaction;
  sem_t inbound_vehicle;
  sem_t outbound_vehicle;
  sem_t station_manager_inbound_notification;
  sem_t station_manager_outbound_notification;
  sem_t ledger_read;
  sem_t ledger_write;
  sem_t ledger_mutex;

  // Station_status_variables
  int bayCAp[3];
  int bayBuses[3];

  // Interest statistics
  int total_passengers;
  int total_departed_passengers;
  int total_boarded_passengers;
  int total_in_station_buses;
  int total_completely_served_buses;
  double average_bus_turnaround_time;
  double average_bus_park_time;
  double average_bus_type_turnaround_time[3];

  // IPC Helpers
  char bus_transaction_type;
  int bus_to_park_id;
  int bus_to_park_category;
  int bus_to_leave_id;
  double bus_to_leave_waiting_time;
  int bus_count;
} Shared_segment;

#endif