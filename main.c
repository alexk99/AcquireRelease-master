#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "assert.h"
#include "stdbool.h"

#define	atomic_cas_bool(p, o, n)__sync_bool_compare_and_swap(p, o, n)	

uint32_t flag = 0;
uint32_t shared_value = 0;

const int kArraySize = 65536;
int* g_randomValues;

typedef struct random_delay {
	unsigned int m_pos;
	unsigned int m_step;
	unsigned int m_wrap;
}
random_delay_t;

random_delay_t* randomDelay1;
random_delay_t* randomDelay2;

void 
random_delay_global_init(void)
{
	g_randomValues = (int*) malloc(sizeof(int) * kArraySize);
	
	int i;
	for (i=0; i<kArraySize; i++)
		g_randomValues[i] = rand();
}

random_delay_t*
random_delay_init(int step, int wrap)
{
	assert(step < kArraySize);
	assert(wrap < kArraySize);
	
	random_delay_t* rd = (random_delay_t*) malloc(sizeof(random_delay_t));
	rd->m_pos = 0;
	rd->m_step = step;
	rd->m_wrap = wrap;
	return rd;
}

void
random_delay_do_busy_work(random_delay_t* rd)
{
	int i;
	do {
		 i = g_randomValues[rd->m_pos];
		 rd->m_pos += rd->m_step;
		 if (rd->m_pos >= rd->m_wrap)
			  rd->m_pos -= rd->m_wrap;
	}
	while (i & 7);
}

void 
inc_shared_value(random_delay_t* rd)
{
	int count = 0;
	while (count < 10000000) {
		random_delay_do_busy_work(rd);
		
		if (atomic_cas_bool(&flag, 0, 1)) {
			/* Lock was successful */
			shared_value++;
			flag = 0;
			count++;
		}
	}
}

void* 
thread_func(void* prm)  
{
	inc_shared_value((random_delay_t*) prm);
}

int main(int argc, char** argv) 
{
	const int num_workers = 4;
	pthread_t workers[num_workers];
	int ret, i;
	random_delay_t* randomDelays[num_workers];

	random_delay_global_init();
	randomDelays[0] = random_delay_init(1, 60101);
	randomDelays[1] = random_delay_init(2, 65535);
	randomDelays[2] = random_delay_init(3, 62535);
	randomDelays[3] = random_delay_init(2, 62535);
	
	while (true) {
		shared_value = 0;
		
		for (i=0; i<num_workers; i++) {
			ret = pthread_create(&workers[i], NULL, thread_func, randomDelays[i]);
			assert(ret == 0);
		}

		/* wait workers to complete */
		for (i=0; i<num_workers; i++)
			pthread_join(workers[i], NULL);
			
		printf("shared value = %u\n", shared_value);
	}
   
	return (EXIT_SUCCESS);
}

