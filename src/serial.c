#include "chronos.h"
#include "logger.h"

#define BUFFER_SIZE (4096)
#define MSG_LEN (256)

typedef struct read_buffer_s {
	uint8_t data[BUFFER_SIZE];
	uint8_t msg[MSG_LEN];
	int size;
	int pos;
} read_buffer_t;

read_buffer_t input_buff;

extern int chronos_connected;
extern log_context_t* ctx;

static int parse_string(heat_t* heats, uint8_t* start, size_t len)
{
	char str[1024];

	memset(str, 0, sizeof(str));
	
	memcpy(str, start, len);	
	
	if (strcmp(str, "TP"))
		log_debug(ctx, "string: %s", str);

	if (chronos_dh(str, heats)) {
		log_error(ctx, "Error while process %s", str);
	} else
		chronos_connected = TRUE;
		
	return 0;
}

int chronos_parse(heat_t* heats)
{
	int processed = 0;
	uint8_t* byte_p;
	int pos = 0;
	uint8_t* start;
	size_t len;
	
	do {
		byte_p = &input_buff.data[pos];

		if (DH_SOH == *byte_p) {
			if (input_buff.size - pos > DH_MIN_LEN - 1) {
				byte_p++;
				pos++;
				
				if ((*byte_p == DH_DC3) ||
				    (*byte_p == DH_DC4)) {
					start = byte_p;
					
					/* Find end */
					pos++;
					byte_p++;
					while (pos < input_buff.size) {
						if (*byte_p == DH_EOT) {
							start++;
							len = byte_p - start;
							
							/** Parse ASCII string */
							parse_string(heats, start, len);
							
							processed = pos + 1;
							break;
						}
						if (*byte_p == DH_SOH) {
							byte_p--;
							pos--;
							processed = pos + 1;
							break;
						}

						byte_p++;
						pos++;
					}
				} else {
					pos--;
				}
			} else {
				return processed;
			}
		} else {
			processed = pos + 1;
		}
		
		pos++;		
	} while((pos < input_buff.size));
	
	return processed;
}

int chronos_read(int fd, heat_t* heats)
{
	size_t req_count;
	size_t count;
	size_t processed;
	
	do {
		/* Requested number of bytes */
		req_count = BUFFER_SIZE - input_buff.size;
		if (!req_count) {
			log_error(ctx, "Buffer overflowed");
			return -1;
		}

		count = read(fd,
		             input_buff.data + input_buff.size,
		             req_count);
		if (count <= 0) {
			if ((errno != EAGAIN) && (errno)) {
				log_error(ctx, "chronos_read() errno %s", strerror(errno));
				return -1;
			}
			return 0;
		}
		
		input_buff.size += count;
		
		/** Process data  */
		processed = chronos_parse(heats);

		if (processed < 0) {
			log_error(ctx, "chronos_parse() returns -1");
			return -1;			
		}
		
		memmove(input_buff.data, input_buff.data + processed, processed);
		input_buff.size -= processed;
		
	} while (count == req_count);

	return 0;
}
