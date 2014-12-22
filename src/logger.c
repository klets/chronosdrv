#ifndef LOGGER_C_
#define LOGGER_C_

#include "logger.h"

#include <string.h>
#include <sys/timeb.h>
#include <time.h>
#include <time.h>
#include <strings.h>

#define stricmp strcasecmp

#include <stdarg.h>
#include <stdio.h>

log_context_t *log_context_new()
{
    return calloc(1,sizeof(log_context_t));
}

void log_context_free(log_context_t *ctx)
{
	if (!ctx)
		return;
    free(ctx);
}

void log_debug(log_context_t *ctx, const char *fmt,...)
{
	int offset = 0;
	va_list ap;
	char msg[MAX_LOG_MSG];

	if ( !ctx ) {
		return;
	}

	if ( !ctx->debug || ( log_level_debug > ctx->level ))
		return;

	if ( ctx->name ) {
		strcpy( msg, "[");
		strcat( msg, ctx->name );
		strcat( msg, "] " );
		offset = strlen( msg );
	}

	va_start(ap,fmt);
	vsnprintf(msg+offset,MAX_LOG_MSG,fmt,ap);
	va_end(ap);
	ctx->debug(ctx,msg);
}

void log_debug_raw(log_context_t *ctx, const char *fmt,...)
{
	int offset = 0;
	va_list ap;
	char msg[MAX_LOG_MSG];

	if ( !ctx ) {
		return;
	}

	if ( !ctx->debug || ( log_level_debug > ctx->level ))
		return;

	va_start(ap,fmt);
	vsnprintf(msg+offset,MAX_LOG_MSG,fmt,ap);
	va_end(ap);

	ctx->debug_raw(ctx,msg);
}

void log_info(log_context_t *ctx, const char *fmt,...)
{
	int offset = 0;
	va_list ap;
	char msg[MAX_LOG_MSG];
	if ( !ctx ) {
		return;
	}

	if ( !ctx->info || ( log_level_info > ctx->level ) )
		return;

	if ( ctx->name ) {
		strcpy( msg, "[");
		strcat( msg, ctx->name );
		strcat( msg, "] " );
		offset = strlen( msg );
	}
	va_start(ap,fmt);
	vsnprintf(msg+offset,MAX_LOG_MSG,fmt,ap);
	va_end(ap);

	ctx->info(ctx,msg);
}

void log_error(log_context_t *ctx, const char *fmt,...)
{
	int offset = 0;
	va_list ap;
	char msg[MAX_LOG_MSG];
	if ( !ctx ) {
		return;
	}

	if ( !ctx->error || ( log_level_error > ctx->level ) )
		return;

	if ( ctx->name ) {
		strcpy( msg, "[");
		strcat( msg, ctx->name );
		strcat( msg, "] " );
		offset = strlen( msg );
	}

	va_start(ap,fmt);
	vsnprintf(msg+offset,MAX_LOG_MSG,fmt,ap);
	va_end(ap);

	ctx->error(ctx,msg);
}

log_level_t str_to_loglevel(const char* str)
{
	log_level_t ll = log_level_error;
	if( 0 == stricmp( "debug", str ) )
		ll = log_level_debug;
	else if( 0 == stricmp( "info", str) )
		ll = log_level_info;
	else if ( 0 == stricmp( "off", str ) )
		ll = log_level_off;
	
	return ll;
}

/* ///////////// */
/* FILE LOGGER// */
/* ///////////// */
static void
file_log_print(file_log_info_t* fi, const char *level, const char *msg)
{

	/* /time_t t = 0; */
	struct tm *ptm;
	struct timeb tb;
	fpos_t curr_file_pos, end_file_pos;
	long curr_offset = 0;
	long size = 0;

	/* /time(&t); */
	ftime( &tb );
	ptm = localtime( &tb.time );

	fgetpos(fi->file_handle, &curr_file_pos);
	curr_offset = ftell(fi->file_handle);

	fseek(fi->file_handle, 0L, SEEK_END);
	fgetpos(fi->file_handle, &end_file_pos);
	size = ftell(fi->file_handle);
	
	if ( size > fi->max_size && curr_offset == size ) {
		rewind(fi->file_handle);
	} else {
		fsetpos(fi->file_handle, &curr_file_pos);
	}

	fprintf(fi->file_handle, "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s]: %s\n",
		ptm->tm_year+1900, ptm->tm_mon+1,
		ptm->tm_mday, ptm->tm_hour,
		ptm->tm_min, ptm->tm_sec, tb.millitm, level, msg );

	if( fi->flush ) 
		fflush(fi->file_handle);
}

static void
file_log_print_raw(file_log_info_t* fi, const char *msg)
{
	fpos_t curr_file_pos, end_file_pos;
	long curr_offset = 0;
	long size = 0;
	/* /time(&t); */
	fgetpos(fi->file_handle, &curr_file_pos);
	curr_offset = ftell(fi->file_handle);

	fseek(fi->file_handle, 0L, SEEK_END);
	fgetpos(fi->file_handle, &end_file_pos);
	size = ftell(fi->file_handle);
	
	if ( size > fi->max_size && curr_offset == size ) {
		rewind(fi->file_handle);
	} else {
		fsetpos(fi->file_handle, &curr_file_pos);
	}

	fprintf(fi->file_handle, "%s", msg );

	if( fi->flush ) 
		fflush(fi->file_handle);
}

static void
file_log_debug(struct log_context_s *ctx, const char *msg)
{
	file_log_info_t* fi = ctx->internal;
	file_log_print(fi,"debug",msg);
}

static void
file_log_debug_raw(struct log_context_s *ctx, const char *msg)
{
	file_log_info_t* fi = ctx->internal;
	file_log_print_raw(fi, msg);
}

static void
file_log_info(struct log_context_s *ctx, const char *msg)
{
	file_log_info_t* fi = ctx->internal;
	file_log_print(fi," info",msg);
}

static void
file_log_error(struct log_context_s *ctx, const char *msg)
{
	file_log_info_t* fi = ctx->internal;
	file_log_print(fi,"error",msg);
}

log_context_t *file_log_new(const char* filename, int flush, long size)
{
	FILE *f = 0;
    f = fopen(filename, "w");
    if ( !f ) {
        fprintf(stderr,"file_log_new: failed to open file %s\n",filename);
        return 0;
    }
    return file_log_new2(f,flush,size);
}

log_context_t *file_log_new2(FILE *f, int flush, long size)
{
	file_log_info_t* fi = 0;
	log_context_t *log = log_context_new();

    if (!log) {
        fclose(f);
		return 0;
	}

    fi = (file_log_info_t*) calloc(1,sizeof(file_log_info_t));
	fi->file_handle = f;
	fi->flush = flush;
	fi->max_size = size;

	log->level = log_level_error;
	log->internal = fi;
    log->info = file_log_info;
	log->debug = file_log_debug;
	log->debug_raw = file_log_debug_raw;
	log->error = file_log_error;

	return log;
}

void file_log_free(log_context_t *ctx)
{
	file_log_info_t* fi;

	if (!ctx) return;
	fi = (file_log_info_t*) ctx->internal;
	fflush(fi->file_handle);
	fclose(fi->file_handle);

    free(fi);
    free(ctx);
}

#endif /* LOGGER_C_ */
