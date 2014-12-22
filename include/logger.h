#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define strcat_safe( dst, n, src ) strncat( dst, src, (n > strlen(dst) + strlen(src) + 1) ? strlen(src) : n - strlen(dst))
#define strcpy_safe( dst, n, src ) strncpy( dst, src, (n > strlen(src)) ? strlen(src)+1 : n-1)

/** Размер файла по умолчанию */
#define LOG_FILE_MAX_SIZE (100*1024) /* 100 КБ */
/*Максимальная длина строки в логе*/
#define MAX_LOG_MSG 1024
/*Длина строки при кодировании*/
#define BUFFER_ROW_SIZE 16

struct log_context_s;

/**
 * Функция записи в лог
 * \param ctx контекст лога
 * \param msg сообщение
 */
typedef void (*log_write_f) (struct log_context_s *ctx,
		const char *msg);

typedef enum {
	log_level_off = 0,
	log_level_error = 1,
	log_level_info = 3,
	log_level_debug = 7
} log_level_t;

/**
 * Структура описывающая контекст лог приложения
 */
typedef struct log_context_s {
	/**имя контекста*/
	const char *name;
	/**функция записи информационного сообщения*/
	log_write_f info;
	/**функция записи оладочного сообщения*/
	log_write_f debug;
	/**функция записи отладочного сообщения*/
	log_write_f debug_raw;
	/**функция записи ошибочного сообщения*/
	log_write_f error;
	/**степень детализации лога*/
	log_level_t level;
	/**внутренние данные приложения*/
	void *internal;
} log_context_t;

log_context_t *log_context_new();
void log_context_free(log_context_t *ctx);
void log_debug(log_context_t *ctx, const char *fmt,...);
void log_debug_raw(log_context_t *ctx, const char *fmt,...);
void log_info(log_context_t *ctx, const char *fmt,...);
void log_error(log_context_t *ctx, const char *fmt,...);
log_level_t str_to_loglevel(const char* str);

typedef struct file_log_info_s {
	FILE* file_handle;
	int flush;
	long max_size;
} file_log_info_t;

log_context_t *file_log_new(const char* filename, int flush, long size);
log_context_t *file_log_new2(FILE *f, int flush, long size);
void file_log_free(log_context_t *ctx);

#ifdef DBG_KLS

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define err_log(CTX, M, ...) log_error(CTX, "[ERROR] (%s:%d: errno: %s) " M, __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define err_log_2(CTX, M, ...) log_error(CTX, "[ERROR] (%s:%d:) " M, __FILE__, __LINE__, ##__VA_ARGS__)

#define err_log_3(CTX, M) log_error(CTX, "[ERROR] (%s:%d:) " M, __FILE__, __LINE__)
	
#define dbg_log(CTX, M, ...) log_debug(CTX, "[DEBUG] (%s:%d: errno: %s) " M, __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define info_log(CTX, M, ...) log_info(CTX, "[INFO] (%s:%d) " M, __FILE__, __LINE__, ##__VA_ARGS__)

#else 

#define err_log log_error 
#define dbg_log log_debug
#define info_log log_info
#define err_log_2 log_error 
#define err_log_3 log_error 

#endif

#endif /* LOGGER_H_ */
