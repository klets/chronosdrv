#ifnfef _CHRONOS_H_
#define _CHRONOS_H_

#include <stdint.h>

#define INDIVIDUAL_PURSUIT    (0)
#define TEAM_PURSUIT          (1)
#define INDIVIDUAL_TIME_TRIAL (2)
#define TEAM_TIME_TRIAL       (3)
#define INDIVIDUAL_SPRINT     (5)
#define TEAM_SPRINT           (6)
#define POINT_RACE            (8)

#define DH_SOH (0x01)
#define DH_DC3 (0x13)
#define DH_DC4 (0x14)
#define DH_EOT (0x04)

typedef chronos_time_s {
	uint16_t hh;
	uint16_t mm;
	uint16_t ss;
	uint16_t dcm;	
} chronos_time_t;

typedef intermediate_time_s {
	/** Pulse */
	uint16_t pulse;	
	/** Result time  */
	chronos_time_t time;
	/** Absolute time  */
	chronos_time_t time_absolute;
} intermediate_time_t;

typedef heat_results_s {
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
	/** Heat number  */
	uint16_t number;
	/** Competition type */
	uint16_t type;
	/** Racers number  */
	/** TODO: default 2, but may be more  */
	uint16_t racers_num;
	/** Start time  */
	chronos_time_t start_time;	
	/** Results */
	/** TODO: dynamic  */
	heat_results_t results[2];
} heat_t;


#endif	/* _CHRONOS_H_ */

