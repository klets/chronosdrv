#ifndef SAVE_C_
#define SAVE_C_

#include "chronos.h"

static FILE* chronos_open_file(chronos_t* chronos, char* prefix, char full_path[], struct tm* save_time)
{
	time_t rawtime;
	struct tm * timeinfo;
	FILE* fp;
	
	time ( &rawtime );
	timeinfo = localtime_r(&rawtime, save_time);
	sprintf(full_path,"%s_%04d%02d%02d.csv",
	        prefix,
	        timeinfo->tm_year + 1900,
	        timeinfo->tm_mon + 1,
	        timeinfo->tm_mday
	        );

	fp = fopen(full_path, "a+");
	if (!fp) {
		log_error(chronos->ctx, "Failed to open %s to save results! Reason %d: %s", full_path, errno, strerror(errno));
		return NULL;
	}
		
	log_info(chronos->ctx, "Opened new file for saving results: %s", full_path);
	
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

static char* get_event_name(uint16_t event)
{
	switch (event) {
	case CHRONOS_START_TIME:
		return "START";
	case CHRONOS_FINISH_TIME:  
		return "FINISH";
	case CHRONOS_INTERMEDIATE_TIME:
		return "INTER";
	case CHRONOS_DNS:
		return "DNS";
	case CHRONOS_DNF:
		return "DNF";
	case CHRONOS_DQ:   
		return "DQ";
	case CHRONOS_OK:   
		return "OK";
	case CHRONOS_FALSE_START:
		return "FALSESTART";
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

static int is_same_day(struct tm* saved_time)
{
	time_t rawtime;
	struct tm * timeinfo;
	
	time ( &rawtime );
	timeinfo = localtime(&rawtime);
	
	if ((timeinfo->tm_year != saved_time->tm_year) ||
	    (timeinfo->tm_mon != saved_time->tm_mon) ||
	    (timeinfo->tm_mday != saved_time->tm_mday)) {
		return FALSE;
	}
	return TRUE;
}

/**
   @brief Save heats to files of fixed size (10 MB)
 */
int chronos_save(chronos_t* chronos, char* prefix)
{
	int i;
	struct stat buf;
	
	if (!chronos->fp_csv) {
		chronos->fp_csv = chronos_open_file(chronos, prefix, chronos->csv_path, &chronos->csv_time);
		if (!chronos->fp_csv) return -1;
	} 
	
	if (stat(chronos->csv_path, &buf)) {
		/** Check that file is still exist  */
		log_error(chronos->ctx, "File %s was removed!", chronos->csv_path);
		if (!chronos->fp_csv) fclose(chronos->fp_csv);
		chronos->fp_csv = chronos_open_file(chronos, prefix, chronos->csv_path, &chronos->csv_time);
		if (!chronos->fp_csv) return -1;
	}
	
	if (!is_same_day(&chronos->csv_time)) {
		/** Close current fp and open new  */
		fclose(chronos->fp_csv);		
		chronos->fp_csv = chronos_open_file(chronos, prefix, chronos->csv_path, &chronos->csv_time);
		if (!chronos->fp_csv) return -1;
	}
	
	for (i = 0; i < MAX_HEATS; i++) {
		if (chronos->heats[i].is_ended) {
			log_debug(chronos->ctx, "Saving heat %d", chronos->heats[i].number);
			chronos_save_heat(chronos->fp_csv, &chronos->heats[i]);
			chronos->heats[i].is_ended = FALSE;
		}
	}

	return 0;
}


int chronos_event_save(chronos_t* chronos, chronos_event_t* event) 
{
	FILE* fp;
	struct stat buf;
	struct tm *ptm;
	struct timeb tb;

	if (!chronos->fp_csv_event) {
		chronos->fp_csv_event = chronos_open_file(chronos, chronos->csv_event_prefix, chronos->csv_event_path, &chronos->csv_event_time);
		if (!chronos->fp_csv_event) return -1;
	} 
	
	if (stat(chronos->csv_event_path, &buf)) {
		/** Check that file is still exist  */
		log_error(chronos->ctx, "File %s was removed!", chronos->csv_event_path);
		if (!chronos->fp_csv_event) fclose(chronos->fp_csv_event);
		chronos->fp_csv_event = chronos_open_file(chronos, chronos->csv_event_prefix, chronos->csv_event_path, &chronos->csv_event_time);
		if (!chronos->fp_csv_event) return -1;
	}
	
	if (!is_same_day(&chronos->csv_event_time)) {
		/** Close current fp and open new  */
		fclose(chronos->fp_csv_event);		
		chronos->fp_csv_event = chronos_open_file(chronos, chronos->csv_event_prefix, chronos->csv_event_path, &chronos->csv_event_time);
		if (!chronos->fp_csv_event) return -1;
	}
	
	log_debug(chronos->ctx, "Saving event %d", event->event);
	
	fp = chronos->fp_csv_event;
	
	ftime( &tb );
	ptm = localtime( &tb.time );

	fprintf(fp, "%04d/%02d/%02d %02d:%02d:%02d.%03d; ",
	        ptm->tm_year+1900, ptm->tm_mon+1,
	        ptm->tm_mday, ptm->tm_hour,
	        ptm->tm_min, ptm->tm_sec, tb.millitm);

	fprintf(fp, "%s; ", get_event_name(event->event));		
	fprintf(fp, "%s; ", get_competition_name(event->type));		
	fprintf(fp, "%u; ", event->heat);		
	fprintf(fp, "%u; ", event->number);		
	fprintf(fp, "%u; ", event->rank);		
	fprintf(fp, "%u; ", event->round);		
	fprintf(fp, "%u; ", event->pulse);		
	chronos_save_time(fp, &event->time_absolute);
	chronos_save_time(fp, &event->time);
	
	fprintf(fp, "\n");	
	fflush(fp);

	return 0;
}

#endif /* SAVE_C_ */
