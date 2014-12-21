#include "chronos.h"


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
static int next_heat(char* str, heat_t* heats) 
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
		fprintf(stderr, "Too much of heats already\n");
		return -1;
	}
			
	heat = &heats[number];
	
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
static int arm_data(char* str, heat_t* heats) 
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
		fprintf(stderr, "Too much of heats already\n");
		return -1;
	}

	heat = &heats[number];
	
	if (heat->number != number) {
		fprintf(stderr, "Incorrect number for heat %u, received %u, but written %u\n", number, number, heat->number);
	}	
	if (heat->type != type) {
		fprintf(stderr, "Incorrect type for heat %u, received %u, but written %u\n", number, type, heat->type);
	}
	
	heat->results[0].number = red;
	heat->results[1].number = green;

	return 0;
}

/**
   Start time, use it to save start time :)
   Parse string like "DS| 5| 2|  0|  0|   56:08.8345"
 */
static int start_time(char* str, heat_t* heats) 
{
	char* token;
	char* saveptr;
	heat_t* heat;	
	uint32_t type;
	uint32_t number;
	uint32_t red;
	uint32_t green;
	chronos_time_t start_time;	
	
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
		fprintf(stderr, "Too much of heats already\n");
		return -1;
	}

	heat = &heats[number];
	
	if (heat->number != number) {
		fprintf(stderr, "Incorrect number for heat %u, received %u, but written %u\n", number, number, heat->number);
	}	
	
	if (heat->type != type) {
		fprintf(stderr, "Incorrect type for heat %u, received %u, but written %u\n", number, type, heat->type);
	}
	
	if (heat->type != INDIVIDUAL_SPRINT) {
		if (heat->results[0].number != red) {
			fprintf(stderr, "Incorrect red for heat %u, received %u, but written %u\n", number, red, heat->results[0].number);
		}

		if (heat->results[1].number != green) {
			fprintf(stderr, "Incorrect green for heat %u, received %u, but written %u\n", number, green, heat->results[1].number);
		}
	}
		
	heat->start_time = start_time;
		
	return 0;
}

/**
   Finish time, use it to save finish time :)
   Parse string like "DF| 0| 3|223|  0|    1:14.567|   35:39.7668|0"
 */
static int finish_time(char* str, heat_t* heats) 
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
		fprintf(stderr, "Too much of heats already\n");
		return -1;
	}

	heat = &heats[number];
	
	if (heat->number != number) {
		fprintf(stderr, "Incorrect number for heat %u, received %u, but written %u\n", number, number, heat->number);
	}	
	
	if (heat->type != type) {
		fprintf(stderr, "Incorrect type for heat %u, received %u, but written %u\n", number, type, heat->type);
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
		fprintf(stderr, "Incorrect racer number for heat %u, received %u, but written %u or %u\n", number, racer, heat->results[0].number, heat->results[1].number);
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
	    
	return 0;
}

/**
   Finish time, use it to save finish time :)
   Parse string like "DATAFINISH| 6| 3| 6|  8|  5|      30.672|   21:23.6836"
 */
static int finish_time2(char* str, heat_t* heats) 
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
		fprintf(stderr, "Too much of heats already\n");
		return -1;
	}

	heat = &heats[number];
	
	if (heat->number != number) {
		fprintf(stderr, "Incorrect number for heat %u, received %u, but written %u\n", number, number, heat->number);
	}	
	
	if (heat->type != type) {
		fprintf(stderr, "Incorrect type for heat %u, received %u, but written %u\n", number, type, heat->type);
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
		fprintf(stderr, "Incorrect racer number for heat %u, received %u, but written %u or %u\n", number, racer, heat->results[0].number, heat->results[1].number);
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
	    
	return 0;
}

/**
   Intermediate time, use it to save finish time :)
   Parse string like "DF| 0| 3|223|  0|    1:14.567|   35:39.7668|0"
 */
static int intermediate_time(char* str, heat_t* heats) 
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
		fprintf(stderr, "Too much of heats already\n");
		return -1;
	}

	heat = &heats[number];
	
	if (heat->number != number) {
		fprintf(stderr, "Incorrect number for heat %u, received %u, but written %u\n", number, number, heat->number);
	}	
	
	if (heat->type != type) {
		fprintf(stderr, "Incorrect type for heat %u, received %u, but written %u\n", number, type, heat->type);
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
		fprintf(stderr, "Incorrect racer number for heat %u, received %u, but written %u or %u\n", number, racer, heat->results[0].number, heat->results[1].number);
	}
	
	if (res->inter_number < pulse) {
		res->intermediate = (intermediate_time_t*) realloc(res->intermediate,
		                                                   pulse * sizeof(intermediate_time_t));
		res->inter_number = pulse;

		
	}
		
	res->intermediate[pulse - 1].pulse = pulse;
	res->intermediate[pulse - 1].time = intermediate_time;
	res->intermediate[pulse - 1].time_absolute = intermediate_time_abs;
		
	return 0;
}

int chronos_dh(char* str, heat_t* heats)
{
	size_t len;
	
	len = strlen(str);

	if (!len) {
		return -1;
	}
	
	if (!strncmp(str, "DN", strlen("DN"))) {		
		return next_heat(str, heats);
	} else if (!strncmp(str, "DATAFINISH", strlen("DATAFINISH"))) {
		return finish_time2(str, heats);
	} else if (!strncmp(str, "DA", strlen("DA"))) {
		return arm_data(str, heats);
	} else if (!strncmp(str, "DS", strlen("DS"))) {
		return start_time(str, heats);
	} else if (!strncmp(str, "DF", strlen("DF"))) {
		return finish_time(str, heats);
	} else if (!strncmp(str, "DI", strlen("DI"))) {		
		return intermediate_time(str, heats);
		/* TODO */
		/* } else if (strncmp(str, "DC", strlen("DC"))) { */
		/* 	return correction(str, heats); */
	} else if (!strncmp(str, "TP", strlen("TP"))) {
		return 0;
	} else {
		fprintf(stderr, "Unknown message\n");
		return -1;
	}
	
	return 0;
}


