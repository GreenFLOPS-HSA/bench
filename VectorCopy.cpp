#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <libelf.h>
#include <iostream>
#include <sys/time.h>
#include <getopt.h>
#include "vector_copy.h"

#define NB_MEASURE 1000

int main(int argc, char **argv)
{
    struct timeval tstart, tstop;
    unsigned long int meas[NB_MEASURE];
    unsigned long int meas2[NB_MEASURE];
    unsigned long int duration = 0;
    unsigned long int var = 0;
    unsigned int size = 1;
    int verbose = 0;
    int i = 0;
    int c;
    int prefillingcache = 0;
    int coef = 1;
    int nbopt = 0;
int flops = 0;

    while ((c = getopt(argc, argv, "cf")) != -1)
	switch (c) {
	case 'c':
	    prefillingcache = 1;
	    nbopt++;
	    break;
	case 'f':
	flops = 1;
	nbopt++;
	break;
	default:
	    abort();
	}

    if (2 <= argc) {
	int pos = nbopt + 1;

	if (NULL != strcasestr(argv[pos], "kB"))
	    coef = 1024;
	else if (NULL != strcasestr(argv[pos], "MB"))
	    coef = 1024 * 1024;
	else if (NULL != strcasestr(argv[pos], "GB"))
	    coef = 1024 * 1024 * 1024;

	size *= coef * atoi(argv[pos]);
    }
    if (1 == coef) {
	size *= sizeof(float);
    }
    //Setup kernel arguments
    float *in = (float *) malloc(size);
    float *out = (float *) malloc(size);
    memset(out, 0, size);
    memset(in, 0, size);
    for (i = 0; i < (size / sizeof(float)); i++)
	in[i] = i;

    // Mesure NBR_COPY copy vector
    //gettimeofday(&tstart, NULL);
    //for (int i = 0; i < LOOPS; i++) {
    SNK_INIT_LPARM(lparm, size / sizeof(float));
    //Fill Caches
    if (prefillingcache) {
	printf("Pre-filling cache option SET\n");
	for (i = 0; i < 64; i++) {
		if(flops) flops_3(in, out, lparm)
		else vcopy(in, out, lparm);
	}
    }

    for (i = 0; i < NB_MEASURE; i++) {
	gettimeofday(&tstart, NULL);
	vcopy(in, out, lparm);
	gettimeofday(&tstop, NULL);
	meas[i] =
	    ((tstop.tv_sec - tstart.tv_sec) * 1000000L + tstop.tv_usec) -
	    tstart.tv_usec;
	//meas2[i] = meas[i] * meas[i];
    }
    for (i = 0; i < NB_MEASURE; i++) {
	duration += meas[i];
	//var += meas2[i];
    }
    duration /= NB_MEASURE;
    for (i = 0; i < NB_MEASURE; i++) {
	var += ((meas[i] - duration) * (meas[i] - duration));
    }
    var /= NB_MEASURE;
    //var -= duration * duration;

	if(flops){
	printf
                ("HSA: Vector of %lu integer of %d-bytes = %lu Bytes takes %lu usec [+/-var %lu] => Speed = %.3f\n",
                (size / sizeof(float)), (int) sizeof(float), size, duration, var, (float)(3.0 * size/sizeof(float) / duration * 1000000));
	}
	else{
    	printf
		("HSA: Vector of %lu integer of %d-bytes = %lu Bytes takes %lu usec [+/-var %lu]\n",
	 	(size / sizeof(float)), (int) sizeof(float), size, duration, var);
	}
    //Validate
    bool valid = true;
    int failIndex = 0;
    for (i = 0; i < (size / sizeof(float)); i++) {
	if (verbose && i < 10)
	    printf("in[%d]=%d, out[%d]=%d, ", i, in[i], i, out[i]);
	if (out[i] != in[i]) {
	    failIndex = i;
	    valid = false;
	    break;
	}
    }
    if (valid) {
	if (verbose)
	    printf("passed validation\n");
    } else
	printf("VALIDATION FAILED!\nBad index: %d\n", failIndex);
    free(in);
    free(out);
    return 0;
}
