#include "chronos.h"
#include "logger.h"

heat_t heats[MAX_HEATS];
int chronos_connected = FALSE;

/** TODO: options parsing  */
int log_level = log_level_debug;
long log_size = 1024 * 1024 * 10;
int stop = 0;

char* log_name = "chronosdrv.log";

log_context_t *ctx;

static void stop_running(int sig)
{
    if ( sig == SIGINT || sig == SIGTERM ) {
	    printf("received sig %d\n", sig);
	    stop = 1;
    }
}

static void setup_signal_actions(void)
{
    struct sigaction actions;
    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = stop_running;
    /* Actions that terminate process */
    sigaction(SIGINT,&actions,NULL);
    sigaction(SIGTERM,&actions,NULL);
    /* Action that terminates specific thread */
    sigaction(SIGUSR1,&actions,NULL);
}

int main(int argc, char* argv[])
{
	int fd, efd, n, i, err;
	char* fname;
	struct epoll_event event;
	struct epoll_event events[MAX_EVENTS];
	
	memset(&event, 0, sizeof(event));
    
	fname = DEFAULT_PORT;
	
	if (argc > 1) {
		fname = argv[1];
	}
	
	ctx = file_log_new(log_name, TRUE, log_size);
	if (!ctx) {
		fprintf(stderr, "Failed to create log %s: %s\n", log_name, strerror(errno));
		return -1;
	}
	ctx->level = (log_level_t) log_level;
	
	fd = open(fname, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		log_error(ctx, "Failed to open %s: %s", fname, strerror(errno));
		return -1;
	}
	
	log_info(ctx, "Opened %s", fname);

	efd = epoll_create(1);
	event.events = EPOLLIN;
	/* set marker of timer event */
    event.data.fd = fd;
    
    err = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
    if (err) {
	    log_error(ctx, "epoll_ctl() returns %d: %s", err, strerror(errno));
	    return -1;
    }
    
    setup_signal_actions();
	
    while(1) {				
	    n = epoll_wait(efd, events, MAX_EVENTS, -1);
	    
		if (n <= 0) {			
			log_error(ctx, "epoll_wait() returns -1: %s", strerror(errno));
			return -1;
		}
		for (i = 0; i < n; i++) {
			struct epoll_event *ev = events + i;
			
			if ( ev->data.fd == fd ) {
				/** Process data from port  */
				if (chronos_read(fd, heats)) {
					log_error(ctx, "Error while procesing serial port");					
				}
			}
		}
		
		/** Dump heats  */
		for (i = 0; i < MAX_HEATS; i++) {
			if (heats[i].is_ended) {
				log_debug_raw(ctx, "HEAT %u, racer %u finish time ", 
				          heats[i].number, heats[i].results[0].number);
				log_debug_raw(ctx, "%u:%u:%u.%u ", 
				          heats[i].results[0].heat_time.hh,
				          heats[i].results[0].heat_time.mm,
				          heats[i].results[0].heat_time.ss,
				          heats[i].results[0].heat_time.dcm);
				
				log_debug_raw(ctx, "racer %u finish time ", 
				          heats[i].results[1].number);

				log_debug(ctx, "%u:%u:%u.%u ", 
				          heats[i].results[1].heat_time.hh,
				          heats[i].results[1].heat_time.mm,
				          heats[i].results[1].heat_time.ss,
				          heats[i].results[1].heat_time.dcm);				
				heats[i].is_ended = FALSE;
			} 
		}

		if (stop)
			break;
	}
    
    /** Print results  */
    
	return 0;
}
