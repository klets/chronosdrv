#ifndef SAVE_C_
#define SAVE_C_

#include "chronos.h"
#include "logger.h"
#include <sys/timeb.h>
#include <time.h>

extern log_context_t* ctx;
extern char csv_path[];

static FILE* chronos_open_file(char* prefix, char full_path[])
{
	time_t rawtime;
	struct tm * timeinfo;
	FILE* fp;
	
	time ( &rawtime );
	timeinfo = localtime(&rawtime);
	sprintf(full_path,"%s_%04d%02d%02d_%02d%02d%02d.csv",
	        prefix,
	        timeinfo->tm_year + 1900,
	        timeinfo->tm_mon + 1,
	        timeinfo->tm_mday,
	        timeinfo->tm_hour,
	        timeinfo->tm_min,
	        timeinfo->tm_sec);

	fp = fopen(full_path, "w+");
	if (!fp) {
		log_error(ctx, "Failed to open %s to save results! Reason %d: %s", full_path, errno, strerror(errno));
		return NULL;
	}
		
	log_info(ctx, "Opened new file for saving results: %s", full_path);
	
	return fp;
}


static char* get_competition_name(uint16_t type)
{
	switch (type) {
	case INDIVIDUAL_PURSUIT:
		return "INDIVIDUAL_PURSUIT";
	case TEAM_PURSUIT:  
		return "TEAM_PURSUIT";
	case INDIVIDUAL_TIME_TRIAL:
		return "INDIVIDUAL_TIME_TRIAL";
	case TEAM_TIME_TRIAL:
		return "TEAM_TIME_TRIAL";
	case INDIVIDUAL_SPRINT:
		return "INDIVIDUAL_SPRINT";
	case TEAM_SPRINT:   
		return "TEAM_SPRINT";
	case POINT_RACE:
		return "POINT_RACE";
	default:
		return "UNKNOWN";
	}
	return "UNKNOWN";
}

static void chronos_save_time(FILE *fp, chronos_time_t* tm)
{
	fprintf(fp, "%02u:%02u:%02u.%03u; ", tm->hh, tm->mm, tm->ss, tm->dcm);		
}

/**
   TEMPLATE: heat;type;racer;round;start_time;finish_time;heat_time;rank;pulses_num;<intermediate_times>
 */
static void chronos_save_heat(FILE *fp, heat_t* heat)
{
	int i, j;
	for (i = 0; i < heat->racers_num; i++) {
		
		fprintf(fp, "%u; ", heat->number);		
		fprintf(fp, "%s; ", get_competition_name(heat->type));		
		fprintf(fp, "%u; ", heat->results[i].number);		
		fprintf(fp, "%u; ", heat->results[i].round);		
		chronos_save_time(fp, &heat->start_time);
		chronos_save_time(fp, &heat->results[i].finish_time);
		chronos_save_time(fp, &heat->results[i].heat_time);
		fprintf(fp, "%u; ", heat->results[i].rank);
		fprintf(fp, "%d; ", heat->results[i].inter_number);
		for (j = 0; j < heat->results[i].inter_number; j++) {
			chronos_save_time(fp, &heat->results[i].intermediate[j].time_absolute);
			chronos_save_time(fp, &heat->results[i].intermediate[j].time);
		}
		fprintf(fp, "\n");
		fflush(fp);
	}
}

/**
   @brief Save heats to files of fixed size (10 MB)
 */
int chronos_save(FILE **fp, char* prefix, long max_size, heat_t* heats)
{
	long size = 0;
	int i;
	struct stat buf;
	
	if (!*fp) {
		*fp = chronos_open_file(prefix, csv_path);
		if (!*fp) return -1;
	} 
	
	if (stat(csv_path, &buf)) {
		/** Check that file is still exist  */
		log_error(ctx, "File %s was removed!", csv_path);
		if (!*fp) fclose(*fp);
		*fp = chronos_open_file(prefix, csv_path);
		if (!*fp) return -1;
	}
	
	size = ftell(*fp);

	if (size > max_size) {
		/** Close current fp and open new  */
		fclose(*fp);
		
		*fp = chronos_open_file(prefix, csv_path);
		if (!*fp) return -1;
	}
	
	for (i = 0; i < MAX_HEATS; i++) {
		if (heats[i].is_ended) {
			log_debug(ctx, "Saving heat %d", heats[i].number);
			chronos_save_heat(*fp, &heats[i]);
			heats[i].is_ended = FALSE;
		}
	}

	return 0;
}

#endif /* SAVE_C_ */
