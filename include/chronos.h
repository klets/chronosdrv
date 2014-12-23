#ifndef _CHRONOS_H_
#define _CHRONOS_H_

#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <time.h>

#include "logger.h"

#define INDIVIDUAL_PURSUIT    (0)
#define TEAM_PURSUIT          (1)
#define INDIVIDUAL_TIME_TRIAL (2)
#define TEAM_TIME_TRIAL       (3)
#define INDIVIDUAL_SPRINT     (5)
#define TEAM_SPRINT           (6)
#define POINT_RACE            (8)

#define DH_MIN_LEN (5)
#define DH_SOH (0x01)
#define DH_DC3 (0x13)
#define DH_DC4 (0x14)
#define DH_EOT (0x04)

#define DEFAULT_PORT "/dev/ttyUSB0"
/**maximum events to wait*/
#define MAX_EVENTS 2

#define MAX_HEATS (9999 + 1)

#define TRUE (1)
#define FALSE (0)
#define QUESTIONABLE (3)

typedef struct chronos_time_s {
	uint16_t hh;
	uint16_t mm;
	uint16_t ss;
	uint16_t dcm;	
} chronos_time_t;

typedef struct intermediate_time_s {
	/** Pulse */
	uint16_t pulse;	
	/** Result time  */
	chronos_time_t time;
	/** Absolute time  */
	chronos_time_t time_absolute;
} intermediate_time_t;

typedef struct heat_results_s {
	/** Heat is ended  */
	int is_ended;
	/** Racer number  */
	uint16_t number;
	/** Pulse */
	uint16_t pulse;	
	/** Round (for sprint) */
	uint16_t round;	
	/** Rank */
	uint16_t rank;
	/** Racer result */
	chronos_time_t heat_time;
	/* Absolute time */
	chronos_time_t finish_time;	
	/** Intermediate results number  */
	int32_t inter_number;
	/** Intermediate results  */
	intermediate_time_t* intermediate;
} heat_results_t;

typedef struct heat_s {
	/** Heat is ended  */
	int is_ended;
	/** Heat number  */
	uint16_t number;
	/** Competition type */
	uint16_t type;
	/** Round (for sprint) */
	uint16_t round;	   
	/** Racers number  */
	/** TODO: default 2, but may be more  */
	uint16_t racers_num;
	/** Start time  */
	chronos_time_t start_time;	
	/** Results */
	/** TODO: dynamic  */
	heat_results_t results[2];
} heat_t;


#define CHRONOS_START_TIME (1)
#define CHRONOS_FINISH_TIME (2)
#define CHRONOS_INTERMEDIATE_TIME (3)
#define CHRONOS_DNS (4)
#define CHRONOS_DNF (5)
#define CHRONOS_DQ (6)
#define CHRONOS_OK (7)
#define CHRONOS_FALSE_START (8)

typedef struct chronos_event_s {
	/** Type of event -- START_TIME, FINISH_TIME, INTERMEDIATE_TIME, DNS, DNF, DQ, FALSE_START */
	uint16_t event;
	/** Type of competition  */
	uint16_t type;	
	/** Number of heat  */
	uint16_t heat;
	/** Number of racer  */
	uint16_t number;
	/** rank of racer  */
	uint16_t rank;
	/** round */
	uint16_t round;
	/** Pulse number for intermediate time */
	uint16_t pulse;	
	/** Result time  */
	chronos_time_t time;
	/** Absolute time  */
	chronos_time_t time_absolute;
} chronos_event_t;

typedef struct chronos_s {
	heat_t heats[MAX_HEATS];
	int chronos_connected;
	int chronos_connected_prev;

	log_context_t *ctx;

	char csv_path[2048];
	FILE *fp_csv;
	struct tm csv_time;

	char* csv_event_prefix;
	char csv_event_path[2048];
	FILE *fp_csv_event;
	struct tm csv_event_time;
} chronos_t;

int chronos_read(chronos_t* chronos, int fd);
int chronos_dh(chronos_t* chronos, char* str);
int chronos_save(chronos_t* chronos, char* prefix);
int chronos_event_save(chronos_t* chronos, chronos_event_t* event);

#endif	/* _CHRONOS_H_ */

