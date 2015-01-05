#include "chronos.h"
#include "logger.h"


#define NEXT_TOKEN(t, s, sv) \
	t = strtok_r(s, " |", sv);\
	if (!t) \
		return -1

#define PARSE_UINT(u, t)	  \
	u = strtoul(t, NULL, 10); \
	if (u == ULONG_MAX)\
		return -1

static int parse_chronos_time(chronos_time_t* t, char* str)
{
	char* token;
	char* saveptr;
	uint32_t u;
	
	memset(t, 0, sizeof(chronos_time_t));
	
	token = strtok_r(str, ":.", &saveptr);
	if (!token) {
		return -1;
	}	
	u = strtoul(token, NULL, 10); 	
	if (u == ULONG_MAX)
		return -1;
	
	t->hh = u;

	token = strtok_r(NULL, ":.", &saveptr);
	if (!token) {
		return -1;
	}	
	u = strtoul(token, NULL, 10); 	
	if (u == ULONG_MAX)
		return -1;
	
	t->mm = u;
	
	token = strtok_r(NULL, ":.", &saveptr);
	if (!token) {
		t->dcm = t->mm;
		t->ss = t->hh;
		t->mm = 0;
		t->hh = 0;
		return 0;
	}	
	
	u = strtoul(token, NULL, 10); 	
	if (u == ULONG_MAX)
		return -1;
	
	t->ss = u;
	
	token = strtok_r(NULL, ":.", &saveptr);
	if (!token) {
		t->dcm = t->ss;
		t->ss = t->mm;
		t->mm = t->hh;
		t->hh = 0;
		return 0;
	}	
	
	u = strtoul(token, NULL, 10); 	
	if (u == ULONG_MAX)
		return -1;
	
	t->dcm = u;

	return 0;
}

static int32_t get_racers_number(uint16_t type)
{
	switch (type) {
	case INDIVIDUAL_PURSUIT:
		return 2;
	case TEAM_PURSUIT:  
		return 2;
	case INDIVIDUAL_TIME_TRIAL:
		return 2;
	case TEAM_TIME_TRIAL:
		return 2;
	case INDIVIDUAL_SPRINT:
		return 1;
	case TEAM_SPRINT:   
		return 2;
	case POINT_RACE:
		return 2;
	default:
		return 2;
	}
	return 2;
}

/**
   Next heat, use it as erasing heat command
   Parse string like "DN| 5| 2|1"
 */
static int next_heat(chronos_t* chronos, char* str) 
{
	char* token;
	char* saveptr;
	heat_t* heat;
		
	uint32_t type;
	uint32_t number;
	uint32_t round;

	NEXT_TOKEN(token, str, &saveptr);	
	
	NEXT_TOKEN(token, NULL, &saveptr);	
	PARSE_UINT(type, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(number, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(round, token);
	
	
	if (number >= MAX_HEATS) {
		log_error(chronos->ctx, "Too much of heats already");
		return -1;
	}
			
	heat = &chronos->heats[number];
	
	if (heat->results[0].inter_number) {
		heat->results[0].inter_number = 0;
		free(heat->results[0].intermediate);
		heat->results[0].intermediate = NULL;
	}

	if (heat->results[1].inter_number) {
		heat->results[1].inter_number = 0;
		free(heat->results[1].intermediate);
		heat->results[1].intermediate = NULL;
	}
	
	heat->is_ended = FALSE;
	heat->results[0].is_ended = FALSE;
	heat->results[1].is_ended = FALSE;	
	heat->number = number;
	heat->type = type;
	heat->round = round;
	heat->racers_num = get_racers_number(type);
		
	return 0;
}

/**
   Arm data, use it to write numbers
   Parse string like "DA| 5| 2| 23|  0"
 */
static int arm_data(chronos_t* chronos, char* str) 
{
	char* token;
	char* saveptr;
	heat_t* heat;	
	uint32_t type;
	uint32_t number;
	uint32_t red;
	uint32_t green;

	NEXT_TOKEN(token, str, &saveptr);	
	
	NEXT_TOKEN(token, NULL, &saveptr);	
	PARSE_UINT(type, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(number, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(red, token);

	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(green, token);
	
	if (number >= MAX_HEATS) {
		log_error(chronos->ctx, "Too much of heats already");
		return -1;
	}

	heat = &chronos->heats[number];
	
	if (heat->number != number) {
		log_error(chronos->ctx, "Incorrect number for heat %u, received %u, but written %u", number, number, heat->number);
	}	
	if (heat->type != type) {
		log_error(chronos->ctx, "Incorrect type for heat %u, received %u, but written %u", number, type, heat->type);
	}
	
	heat->results[0].number = red;
	heat->results[1].number = green;

	return 0;
}

/**
   False start
   Parse string like "DCGG| 0| 5|S"
 */
static int false_start(chronos_t* chronos, char* str) 
{
	char* token;
	char* saveptr;
	heat_t* heat;	
	uint32_t type;
	uint32_t number;
	chronos_event_t event;
	
	memset(&event, 0, sizeof(chronos_event_t));

	NEXT_TOKEN(token, str, &saveptr);	
	
	NEXT_TOKEN(token, NULL, &saveptr);	
	PARSE_UINT(type, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(number, token);
	
	if (number >= MAX_HEATS) {
		log_error(chronos->ctx, "Too much of heats already");
		return -1;
	}

	heat = &chronos->heats[number];
	
	if (heat->number != number) {
		log_error(chronos->ctx, "Incorrect number for heat %u, received %u, but written %u", number, number, heat->number);
	}	
	if (heat->type != type) {
		log_error(chronos->ctx, "Incorrect type for heat %u, received %u, but written %u", number, type, heat->type);
	}
	
	event.event = CHRONOS_FALSE_START;
	event.type = type;
	event.heat = number;
	
	if (chronos_event_save(chronos, &event)) {
		log_info(chronos->ctx, "Failed to save event CHRONOS_FALSE_START");
	}
	
	return 0;
}

/**
   Correction
   Parse string like 
   "DC| 0| 5|I| 2|228" -- pulse
   "DC| 1| 2|Q|103" -- DQ
   "DC| 1| 2|F|103" -- pulse finish
   "DC| 1| 2|A|103" -- DNF
   "DC| 1| 2|R|103" -- OK
 */
static int correction(chronos_t* chronos, char* str) 
{
	char* token;
	char* saveptr;
	heat_t* heat;	
	uint32_t type;
	uint32_t number;
	uint32_t pulse;
	uint32_t racer;
	chronos_event_t event;
	
	memset(&event, 0, sizeof(chronos_event_t));

	NEXT_TOKEN(token, str, &saveptr);	
	
	NEXT_TOKEN(token, NULL, &saveptr);	
	PARSE_UINT(type, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(number, token);
	
	if (number >= MAX_HEATS) {
		log_error(chronos->ctx, "Too much of heats already");
		return -1;
	}

	heat = &chronos->heats[number];
	
	if (heat->number != number) {
		log_error(chronos->ctx, "Incorrect number for heat %u, received %u, but written %u", number, number, heat->number);
	}	
	if (heat->type != type) {
		log_error(chronos->ctx, "Incorrect type for heat %u, received %u, but written %u", number, type, heat->type);
	}
	
	NEXT_TOKEN(token, NULL, &saveptr);
	
	if (!strncmp(token, "I", strlen("I"))) {
		/* Pulse */
		/* TODO */
		NEXT_TOKEN(token, NULL, &saveptr);
		PARSE_UINT(pulse, token);
		
		NEXT_TOKEN(token, NULL, &saveptr);
		PARSE_UINT(racer, token);
	} else if (!strncmp(token, "Q", strlen("Q"))) {		
		/* DQ */
		/* TODO */

		NEXT_TOKEN(token, NULL, &saveptr);
		PARSE_UINT(racer, token);

	} else if (!strncmp(token, "F", strlen("F"))) {
		/* Pulse finish? */
		/* TODO */
		
		NEXT_TOKEN(token, NULL, &saveptr);
		PARSE_UINT(racer, token);

	} else if (!strncmp(token, "A", strlen("A"))) {
		/* Abort? */
		/* TODO */

		NEXT_TOKEN(token, NULL, &saveptr);
		PARSE_UINT(racer, token);

	} else if (!strncmp(token, "R", strlen("R"))) {
		/* Ok? */
		/* TODO */

		NEXT_TOKEN(token, NULL, &saveptr);
		PARSE_UINT(racer, token);		
	} else {
		log_error(chronos->ctx, "Unknown DC message: %s", str);
	}
	return 0;
}

/**
   Start time, use it to save start time :)
   Parse string like "DS| 5| 2|  0|  0|   56:08.8345"
 */
static int start_time(chronos_t* chronos, char* str) 
{
	char* token;
	char* saveptr;
	heat_t* heat;	
	uint32_t type;
	uint32_t number;
	uint32_t red;
	uint32_t green;
	chronos_time_t start_time;	
	chronos_event_t event;
	
	memset(&event, 0, sizeof(chronos_event_t));
	
	NEXT_TOKEN(token, str, &saveptr);	
	
	NEXT_TOKEN(token, NULL, &saveptr);	
	PARSE_UINT(type, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(number, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(red, token);

	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(green, token);

	NEXT_TOKEN(token, NULL, &saveptr);
	if (parse_chronos_time(&start_time, token)) {
		return -1;
	}		
	
	if (number >= MAX_HEATS) {
		log_error(chronos->ctx, "Too much of heats already");
		return -1;
	}

	heat = &chronos->heats[number];
	
	if (heat->number != number) {
		log_error(chronos->ctx, "Incorrect number for heat %u, received %u, but written %u", number, number, heat->number);
	}	
	
	if (heat->type != type) {
		log_error(chronos->ctx, "Incorrect type for heat %u, received %u, but written %u", number, type, heat->type);
	}
	
	if (heat->type != INDIVIDUAL_SPRINT) {
		if (heat->results[0].number != red) {
			log_error(chronos->ctx, "Incorrect red for heat %u, received %u, but written %u", number, red, heat->results[0].number);
		}

		if (heat->results[1].number != green) {
			log_error(chronos->ctx, "Incorrect green for heat %u, received %u, but written %u", number, green, heat->results[1].number);
		}
	}
		
	heat->start_time = start_time;
	
	event.event = CHRONOS_START_TIME;
	event.type = type;
	event.heat = number;
	event.time_absolute = start_time;
	event.number = red;
	
	if (chronos_event_save(chronos, &event)) {
		log_info(chronos->ctx, "Failed to save event CHRONOS_START_TIME");
	}
	
	if (heat->racers_num > 1) {
		event.number = green;
		if (chronos_event_save(chronos, &event)) {
			log_info(chronos->ctx, "Failed to save event CHRONOS_START_TIME");
		}
	}

	return 0;
}

/**
   Finish time, use it to save finish time :)
   Parse string like "DF| 0| 3|223|  0|    1:14.567|   35:39.7668|0"
 */
static int finish_time(chronos_t* chronos, char* str) 
{
	char* token;
	char* saveptr;
	heat_t* heat;	
	uint32_t type;
	uint32_t number;
	uint32_t racer;
	uint32_t rank;
	uint32_t round;

	chronos_time_t finish_time_abs;
	chronos_time_t finish_time;
	
	heat_results_t* res;
	
	chronos_event_t event;
	
	memset(&event, 0, sizeof(chronos_event_t));

	NEXT_TOKEN(token, str, &saveptr);	
	
	NEXT_TOKEN(token, NULL, &saveptr);	
	PARSE_UINT(type, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(number, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(racer, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(rank, token);

	NEXT_TOKEN(token, NULL, &saveptr);
	if (parse_chronos_time(&finish_time, token)) {
		return -1;
	}		

	NEXT_TOKEN(token, NULL, &saveptr);
	if (parse_chronos_time(&finish_time_abs, token)) {
		return -1;
	}		
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(round, token);

	if (number >= MAX_HEATS) {
		log_error(chronos->ctx, "Too much of heats already");
		return -1;
	}

	heat = &chronos->heats[number];
	
	if (heat->number != number) {
		log_error(chronos->ctx, "Incorrect number for heat %u, received %u, but written %u", number, number, heat->number);
	}	
	
	if (heat->type != type) {
		log_error(chronos->ctx, "Incorrect type for heat %u, received %u, but written %u", number, type, heat->type);
	}

	if (heat->results[0].number != racer) {
		if (heat->results[1].number != racer) {
			res = NULL;
		} else {
			res = &heat->results[1];
		}
	} else {
		res = &heat->results[0];
	} 
	
	if (!res) {
		log_error(chronos->ctx, "Incorrect racer number for heat %u, received %u, but written %u or %u", number, racer, heat->results[0].number, heat->results[1].number);
		return -1;
	}
		
	res->finish_time = finish_time_abs;
	res->heat_time = finish_time;
	res->round = round;
	res->rank = rank;
	res->is_ended = TRUE;
	
	if ((heat->racers_num == 1) ||
	    ((heat->racers_num == 2) && 
	     heat->results[0].is_ended &&
	     heat->results[1].is_ended)) {
		heat->is_ended = TRUE;
	}
	
	event.event = CHRONOS_FINISH_TIME;
	event.type = type;
	event.heat = number;
	event.number = racer;
	event.rank = rank;
	event.round = round;
	event.time_absolute = finish_time_abs;
	event.time = finish_time;
	
	if (chronos_event_save(chronos, &event)) {
		log_info(chronos->ctx, "Failed to save event CHRONOS_FINISH_TIME");
	}
	
	return 0;
}

/**
   Finish time, use it to save finish time :)
   Parse string like "DATAFINISH| 6| 3| 6|  8|  5|      30.672|   21:23.6836"
 */
static int finish_time2(chronos_t* chronos, char* str) 
{
	char* token;
	char* saveptr;
	heat_t* heat;	
	uint32_t type;
	uint32_t number;
	uint32_t racer;
	uint32_t rank;   
	uint32_t pulse;

	chronos_time_t finish_time_abs;
	chronos_time_t finish_time;
	
	heat_results_t* res;
	chronos_event_t event;
	
	memset(&event, 0, sizeof(chronos_event_t));

	NEXT_TOKEN(token, str, &saveptr);	
	
	NEXT_TOKEN(token, NULL, &saveptr);	
	PARSE_UINT(type, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(number, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(pulse, token);

	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(racer, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(rank, token);

	NEXT_TOKEN(token, NULL, &saveptr);
	if (parse_chronos_time(&finish_time, token)) {
		return -1;
	}		

	NEXT_TOKEN(token, NULL, &saveptr);
	if (parse_chronos_time(&finish_time_abs, token)) {
		return -1;
	}		

	if (number >= MAX_HEATS) {
		log_error(chronos->ctx, "Too much of heats already");
		return -1;
	}

	heat = &chronos->heats[number];
	
	if (heat->number != number) {
		log_error(chronos->ctx, "Incorrect number for heat %u, received %u, but written %u", number, number, heat->number);
	}	
	
	if (heat->type != type) {
		log_error(chronos->ctx, "Incorrect type for heat %u, received %u, but written %u", number, type, heat->type);
	}

	if (heat->results[0].number != racer) {
		if (heat->results[1].number != racer) {
			res = NULL;
		} else {
			res = &heat->results[1];
		}
	} else {
		res = &heat->results[0];
	} 
	
	if (!res) {
		log_error(chronos->ctx, "Incorrect racer number for heat %u, received %u, but written %u or %u", number, racer, heat->results[0].number, heat->results[1].number);
		return -1;
	}
		
	res->finish_time = finish_time_abs;
	res->heat_time = finish_time;
	res->rank = rank;
	
	res->is_ended = TRUE;
	
	if ((heat->racers_num == 1) ||
	    ((heat->racers_num == 2) && 
	     heat->results[0].is_ended &&
	     heat->results[1].is_ended)) {
		heat->is_ended = TRUE;
	}
	
	event.event = CHRONOS_FINISH_TIME;
	event.type = type;
	event.heat = number;
	event.number = racer;
	event.rank = rank;
	event.pulse = pulse;
	event.time_absolute = finish_time_abs;
	event.time = finish_time;
	
	if (chronos_event_save(chronos, &event)) {
		log_info(chronos->ctx, "Failed to save event CHRONOS_FINISH_TIME");
	}

	return 0;
}

/**
   Intermediate time, use it to save finish time :)
   Parse string like "DF| 0| 3|223|  0|    1:14.567|   35:39.7668|0"
 */
static int intermediate_time(chronos_t* chronos, char* str) 
{
	char* token;
	char* saveptr;
	heat_t* heat;
	uint32_t type;
	uint32_t number;
	uint32_t racer;
	uint32_t pulse;

	chronos_time_t intermediate_time_abs;
	chronos_time_t intermediate_time;
	
	heat_results_t* res;
	chronos_event_t event;
	
	memset(&event, 0, sizeof(chronos_event_t));

	NEXT_TOKEN(token, str, &saveptr);	
	
	NEXT_TOKEN(token, NULL, &saveptr);	
	PARSE_UINT(type, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(number, token);
	
	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(pulse, token);

	NEXT_TOKEN(token, NULL, &saveptr);
	PARSE_UINT(racer, token);
	

	NEXT_TOKEN(token, NULL, &saveptr);
	if (parse_chronos_time(&intermediate_time, token)) {
		return -1;
	}		

	NEXT_TOKEN(token, NULL, &saveptr);
	if (parse_chronos_time(&intermediate_time_abs, token)) {
		return -1;
	}		
	
	if (number >= MAX_HEATS) {
		log_error(chronos->ctx, "Too much of heats already");
		return -1;
	}

	heat = &chronos->heats[number];
	
	if (heat->number != number) {
		log_error(chronos->ctx, "Incorrect number for heat %u, received %u, but written %u", number, number, heat->number);
	}	
	
	if (heat->type != type) {
		log_error(chronos->ctx, "Incorrect type for heat %u, received %u, but written %u", number, type, heat->type);
	}

	if (heat->results[0].number != racer) {
		if (heat->results[1].number != racer) {
			res = NULL;
		} else {
			res = &heat->results[1];
		}
	} else {
		res = &heat->results[0];
	} 
	
	if (!res) {
		log_error(chronos->ctx, "Incorrect racer number for heat %u, received %u, but written %u or %u", number, racer, heat->results[0].number, heat->results[1].number);
	}
	
	if (res->inter_number < pulse) {
		res->intermediate = (intermediate_time_t*) realloc(res->intermediate,
		                                                   pulse * sizeof(intermediate_time_t));		
		memset(&res->intermediate[res->inter_number],
		       0,
		       sizeof(intermediate_time_t) * (pulse - res->inter_number));
		
		res->inter_number = pulse;		
	}
		
	res->intermediate[pulse - 1].pulse = pulse;
	res->intermediate[pulse - 1].time = intermediate_time;
	res->intermediate[pulse - 1].time_absolute = intermediate_time_abs;
	
	event.event = CHRONOS_INTERMEDIATE_TIME;
	event.type = type;
	event.heat = number;
	event.number = racer;
	event.pulse = pulse;
	event.time_absolute = intermediate_time_abs;
	event.time = intermediate_time;
	
	if (chronos_event_save(chronos, &event)) {
		log_info(chronos->ctx, "Failed to save event CHRONOS_INTERMEDIATE_TIME");
	}

	return 0;
}

int chronos_dh(chronos_t* chronos, char* str)
{
	size_t len;
	
	len = strlen(str);

	if (!len) {
		return -1;
	}
	
	if (!strncmp(str, "DN", strlen("DN"))) {		
		return next_heat(chronos, str);
	} else if (!strncmp(str, "DATAFINISH", strlen("DATAFINISH"))) {
		return finish_time2(chronos, str);
	} else if (!strncmp(str, "DA", strlen("DA"))) {
		return arm_data(chronos, str);
	} else if (!strncmp(str, "DS", strlen("DS"))) {
		return start_time(chronos, str);
	} else if (!strncmp(str, "DF", strlen("DF"))) {
		return finish_time(chronos, str);
	} else if (!strncmp(str, "DI", strlen("DI"))) {		
		return intermediate_time(chronos, str);		
	} else if (strncmp(str, "DCGG", strlen("DCGG"))) {
		return false_start(chronos, str);
	} else if (strncmp(str, "DC", strlen("DC"))) {
		return correction(chronos, str);
	} else if (!strncmp(str, "TP", strlen("TP"))) {
		return 0;
	} else {
		log_error(chronos->ctx, "Unknown message");
		return -1;
	}
	
	return 0;
}


