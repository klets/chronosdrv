#include <stdio.h>
#include <signal.h>

#define DEFAULT_PORT "/dev/ttyUSB0"
/**maximum events to wait*/
#define MAX_EVENTS 2

#define MAX_HEATS 1024

int cur_heat;
heat_t heats[MAX_HEATS];

/** TODO: options parsing  */

int stop = 0;

static void stop_running(int sig)
{
    if ( sig == SIGINT || sig == SIGTERM ) {
	    stop = 1;
    }
}

static void setup_signal_actions()
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
	
	fd = open(fname, O_RDWR, O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "Failed to open %s: %s", fname, strerror(errno));
		return -1;
	}
	
	efd = epoll_create(1);
	event.events = EPOLLIN;
	/* set marker of timer event */
    event.data.fd = fd;
    
    err = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
    if (err) {
	    fprintf(stderr, "epoll_ctl() returns %d: %s", err, strerror(errno));
	    return -1;
    }

    while(1) {				
	    n = epoll_wait(efd, events, MAX_EVENTS, -1);
	    
		if (n <= 0) {			
			fprintf(stderr, "epoll_wait() returns -1: %s", strerror(errno));
			return -1;
		}
		for (i = 0; i < n; i++) {
			struct epoll_event *ev = events + i;
			
			if ( ev->data.fd == fd ) {
				/** Process data from port  */
				
			}
		}

		if (stop)
			break;
	}
    
    /** Print results  */
    
	return 0;
}
