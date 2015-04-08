#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <getopt.h>
#include <CL/cl.h>

#define MAX_SOURCE_SIZE (0x100000)
#define NB_MEASURE 1000

int main(int argc, char *argv[])
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

    // Host input vectors
    float *h_a;
    // Host output vector
    float *h_c;

    // device type CPU or GPU
    int device_type = CL_DEVICE_TYPE_DEFAULT;


    // Device input buffers
    cl_mem d_a;
    // Device output buffer
    cl_mem d_c;

    cl_platform_id cpPlatform;	// OpenCL platform
    cl_device_id device_id;	// device ID
    cl_context context;		// context
    cl_command_queue queue;	// command queue
    cl_program program;		// program
    cl_kernel kernel;		// kernel

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

    if (3 <= argc) {
	int pos = nbopt + 1;
	if (!strcasecmp("cpu", argv[pos])) {
	    device_type = CL_DEVICE_TYPE_CPU;
	    printf("SET Device to CPU\n");
	} else {
	    //device_type = CL_DEVICE_TYPE_GPU;
	    printf("SET Device to GPU\n");
	}

	pos++;
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
    // Allocate memory for each vector on host
    h_a = (float *) malloc(size);
    h_c = (float *) malloc(size);

    // Initialize vectors on host
    for (i = 0; i < size / sizeof(float); i++) {
	h_a[i] = (float) i;
	h_c[i] = 0.0;
    }

    size_t globalSize, localSize;
    cl_int err;

    FILE *fp;
    char *source_str;
    size_t source_size;

    /* Load the source code containing the kernel*/
    if (1 == flops) {
	fp = fopen("./flops.cl", "r");
    } else {
	fp = fopen("./vector_copy.cl", "r");
    }
    if (!fp) {
	fprintf(stderr, "Failed to load kernel.\n");
	exit(1);
    }
    source_str = (char *) malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);


    // Number of work items in each local work group
    localSize = 256;

    // Number of total work items - localSize must be devisor
    globalSize =
	ceil(size / sizeof(float) / (float) localSize) * localSize;

    // Bind to platform
    err = clGetPlatformIDs(1, &cpPlatform, NULL);

    // Get ID for the device
    err = clGetDeviceIDs(cpPlatform, device_type, 1, &device_id, NULL);

    // Create a context 
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);

    // Create a command queue
    queue =
	clCreateCommandQueueWithProperties(context, device_id, 0, &err);

    // Create the compute program from the source buffer
    program = clCreateProgramWithSource(context, 1,
					(const char **) &source_str, NULL,
					&err);

    // Build the program executable
    clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

    // Create the compute kernel in the program we wish to run
    if (1 == flops) {
	kernel = clCreateKernel(program, "flops_3", &err);
    } else {
	kernel = clCreateKernel(program, "vcopy", &err);
    }

    // Create the input and output arrays in device memory for our calculation
    d_a = clCreateBuffer(context, CL_MEM_READ_ONLY, size, NULL, NULL);
    d_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY, size, NULL, NULL);


    //Fill Caches
    if (prefillingcache) {
	for (i = 0; i < 64; i++) {
	    // Write our data set into the input array in device memory
	    err = clEnqueueWriteBuffer(queue, d_a, CL_TRUE, 0,
				       size, h_a, 0, NULL, NULL);

	    // Set the arguments to our compute kernel
	    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_a);
	    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_c);

	    // Execute the kernel over the entire range of the data set 
	    err =
		clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize,
				       &localSize, 0, NULL, NULL);

	    // Wait for the command queue to get serviced before reading back results
	    clFinish(queue);

	    // Read the results from the device
	    clEnqueueReadBuffer(queue, d_c, CL_TRUE, 0, size, h_c, 0, NULL,
				NULL);
	}
    }

    for (i = 0; i < NB_MEASURE; i++) {
	gettimeofday(&tstart, NULL);

	// Write our data set into the input array in device memory
	err = clEnqueueWriteBuffer(queue, d_a, CL_TRUE, 0,
				   size, h_a, 0, NULL, NULL);

	// Set the arguments to our compute kernel
	err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_a);
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_c);

	// Execute the kernel over the entire range of the data set 
	err =
	    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize,
				   &localSize, 0, NULL, NULL);

	// Wait for the command queue to get serviced before reading back results
	clFinish(queue);

	// Read the results from the device
	clEnqueueReadBuffer(queue, d_c, CL_TRUE, 0, size, h_c, 0, NULL,
			    NULL);

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
    if (1 == flops) {
	printf
	    ("OpenCL: Vector of %lu float 3 flops each takes %lu usec [+/-var %lu] => Speed = %.3f GFlops\n",
	     (size / sizeof(float)), duration, var,
	     (3.0 * (size / sizeof(float)) / (float) duration / 1000.0));
    } else {
	printf
	    ("OpenCL: Vector of %lu float of %d-bytes = %lu Bytes takes %lu usec [+/-var %lu]\n",
	     (size / sizeof(float)), (int) sizeof(float), size, duration,
	     var);
    }
    int valid = 1;
    for (i = 0; i < size / sizeof(float); i++) {
	if (verbose && (i < 10))
	    printf("in[%d]=%.2f, out[%d]=%.2f, ", i, h_a[i], i, h_c[i]);
	if (i != h_c[i]) {
	    valid = 0;
	    break;
	}
    }
    if (verbose) {
	if (valid)
	    printf("\nValidation Passed: out[%d]=%.2f\n", i - 1,
		   h_c[i - 1]);
	else
	    printf("\nValidation Failed\n");
    }
    // release OpenCL resources
    clReleaseMemObject(d_a);
    clReleaseMemObject(d_c);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    //release host memory
    free(h_a);
    free(h_c);

    return 0;
}
