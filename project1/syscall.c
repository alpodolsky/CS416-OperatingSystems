/* syscall.c
 *
 * Group Members Names and NetIDs:
 *   1. Al Manrique, am2240
 *   2. Alex Podolsky, adp169
 *
 * ILab Machine Tested on:
 *  ilab1
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>                                                                                
#include <sys/syscall.h>


double avg_time = 0;

int main(int argc, char *argv[]) {
	
    /* Implement Code Here */
	
	int i;
	double time = 0;
	struct timeval start;
	struct timeval end;
	
	for(i = 0; i<5000000; i++){
	
		gettimeofday(&start, 0);
		syscall(getuid());
		gettimeofday(&end, 0);
		time += ((end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec-start.tv_usec));
	}
	avg_time = time/5000000;
	
	
    // Remember to place your final calculated average time
    // per system call into the avg_time variable
	
    printf("Average time per system call is %f microseconds\n", avg_time);

    return 0;
}
