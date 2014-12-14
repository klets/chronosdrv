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
	int i, processed;
	uint8_t* byte_p;
	int pos = 0;
	
	do {
		byte_p = &input_buff.data[pos];
		pos++;
		if (DH_SOH == *byte_p) {
			processed = parse_msg(byte_p, pos, heats, cur_heat);
		}
		
		pos += processed;
		
	} while((pos < input_buff.size) & (processed > 0));
	
	return pos;
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
