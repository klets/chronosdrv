#include "chronos.h"

#define BUFFER_SIZE (4096)
#define MSG_LEN (256)

typedef read_buffer_s {
	uint8_t data[BUFFER_SIZE];
	uint8_t msg[MSG_LEN];
	int size;
	int pos;
} read_buffer_t;

read_buffer_t input_buff;


int chronos_parse(heat_t* heats, int cur_heat)
{
	int i, processed = 0;
	uint8_t* byte_p;
	int pos = 0;
	uint8_t* start;
	uint8_t* end;

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
							end = byte_p - 1;

							/** Parse ASCII string */
							processed = pos;
							break;
						}
						if (*byte_p == DH_SOH) {
							byte_p--;
							pos--;
							processed = pos;
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
			processed = pos;
		}
		
		pos++;		
		
	} while((pos < input_buff.size));
	
	return processed;
}

int chronos_read(int fd, heat_t* heats, int cur_heat)
{
	heat_t* h;
	size_t req_count;
	size_t count;
	size_t processed;
	
	h = &heats[cur_heat];

	do {
		/* Requested number of bytes */
		req_count = BUFFER_SIZE - input_buff.size;
		
		if (!req_count) {
			fprintf(stderr, "Buffer overflowed\n");
			return -1;
		}

		count = read(fd,
		             input_buff.data + input_buff.size,
		             req_count);
		if ( count < 0 ) {
			if (errno != EAGAIN) {
				fprintf(stderr, "chronos_read() errno %s\n", strerror(errno));
				return -1;
			}

			return 0;
		}
		
		input_buff.size += count;
		
		/** Process data  */
		processed = chronos_parse(heats, cur_heat);
		
		if (processed < 0) {
			fprintf(stderr, "chronos_parse() returns -1\n");
			return -1;			
		}
		
		memmove(input_buff.data, input_buff.data + processed, processed);
		input_buff.size -= processed;

	} while ( count );

	return 0;
}
