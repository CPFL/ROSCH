#define _GNU_SOURCE
#include <cuda.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>
#include <linux/sched.h>

/* for resch */
#include <resch/api_gpu.h>
#include <resch/api.h>
#include "config.h"

#ifndef DETAIL_TIME
#define gettimeofday(x, y)
#else
#define gettimeofday(x, y) do_gettimeofday(x)
#endif
#define gettime(fmt) clock_gettime(CLOCK_MONOTONIC, fmt)

extern pthread_mutex_t mutex;
extern int prio;
extern struct timespec timeout, period, deadline, runtime;
static struct timespec ms_to_timespec(unsigned long ms)
{
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms - ts.tv_sec * 1000) * 1000000LL;
	return ts;
}
static unsigned long long timespec_to_ns_sub(struct timespec *ts1, struct timespec *ts2)
{
	unsigned long long ret, ret2;

	ret = ts1->tv_sec * 1000000000LL;
	ret2 = ts2->tv_sec * 1000000000LL;
	ret += ts1->tv_nsec;
	ret2 += ts2->tv_nsec;

	ret = ret2 - ret;
	return ret;
}


/* tvsub: ret = x - y. */
static inline void tvsub(struct timeval *x, 
	struct timeval *y, 
	struct timeval *ret)
{
    ret->tv_sec = x->tv_sec - y->tv_sec;
    ret->tv_usec = x->tv_usec - y->tv_usec;
    if (ret->tv_usec < 0) {
	ret->tv_sec--;
	ret->tv_usec += 1000000;
    }
}

int cuda_test_madd(unsigned int n, char *path, int loop_count)
{
    struct rtxGhandle *handle;
 
    int i, j, idx;
    CUresult res;
    CUdevice dev;
    CUcontext ctx;
    CUfunction function;
    CUmodule module;
    CUdeviceptr a_dev, b_dev, c_dev;
    unsigned int *a = (unsigned int *) malloc (n*n * sizeof(unsigned int));
    unsigned int *b = (unsigned int *) malloc (n*n * sizeof(unsigned int));
    unsigned int *c = (unsigned int *) malloc (n*n * sizeof(unsigned int));
    int block_x, block_y, grid_x, grid_y;
    char fname[256];
    struct timeval tv;
    struct timeval tv_total_start, tv_total_end;
    float total;
    struct timeval tv_h2d_start, tv_h2d_end;
    float h2d;
    struct timeval tv_d2h_start, tv_d2h_end;
    float d2h;
    struct timeval tv_exec_start, tv_exec_end;
    struct timeval tv_mem_alloc_start;
    struct timeval tv_data_init_start;
    struct timeval tv_init_end;
    float data_init;
    struct timeval tv_conf_kern_start;
    struct timeval tv_close_start;
    float mem_alloc;
    float exec;
    float init_gpu;
    float configure_kernel;
    float close_gpu;
    float data_read;
    int loop;
    unsigned int dummy_b, dummy_c;
    
    struct timeval tv1, tv2, tv3;
    struct timespec ts1,ts2,ts3;
    /* block_x * block_y should not exceed 512. */
    block_x = n < 16 ? n : 16;
    block_y = n < 16 ? n : 16;
    grid_x = n / block_x;
    if (n % block_x != 0)
	grid_x++;
    grid_y = n / block_y;
    if (n % block_y != 0)
	grid_y++;

    gettimeofday(&tv_total_start, NULL);
    res = cuInit(0);
    if (res != CUDA_SUCCESS) {
	printf("cuInit failed: res = %lu\n", (unsigned long)res);
	return -1;
    }

    res = cuDeviceGet(&dev, 0);
    if (res != CUDA_SUCCESS) {
	printf("cuDeviceGet failed: res = %lu\n", (unsigned long)res);
	return -1;
    }

    res = cuCtxCreate(&ctx, 0, dev);
    if (res != CUDA_SUCCESS) {
	printf("cuCtxCreate failed: res = %lu\n", (unsigned long)res);
	return -1;
    }
#ifndef DISABLE_SCHED_GPU
    handle = NULL;
    if((res = rtx_gpu_open(&handle, 0, 0)) < 0){
	printf("rtx_gpu_open failed: res = %lu\n", (unsigned long)res);
    }
#endif
#ifndef DISABLE_SCHED_CPU
    rt_set_priority(loop_count);
    rt_set_scheduler(SCHED_FP); /* you can also set SCHED_EDF. */
    timeout = ms_to_timespec(1);
    rt_run(timeout);
#endif
    gettimeofday(&tv_init_end, NULL);
    for (loop = 0; loop<LOOP_COUNT; loop++){
	gettime(&ts1);
	sprintf(fname, "%s/loop_gpu.cubin", path);
	res = cuModuleLoad(&module, fname);
	if (res != CUDA_SUCCESS) {
	    printf("cuModuleLoad() failed\n");
	    return -1;
	}
	res = cuModuleGetFunction(&function, module, "_Z4loopPjS_S_j");
	if (res != CUDA_SUCCESS) {
	    printf("cuModuleGetFunction() failed\n");
	    return -1;
	}
	/*	res = cuFuncSetSharedSize(function, 0x40); /* just random 
		if (res != CUDA_SUCCESS) {
		printf("cuFuncSetSharedSize() failed\n");
		return -1;
		}
		*/


	res = cuFuncSetBlockShape(function, block_x, block_y, 1);
	if (res != CUDA_SUCCESS) {
	    printf("cuFuncSetBlockShape() failed\n");
	    return -1;
	}

	gettimeofday(&tv_mem_alloc_start, NULL);

	/* a[] */
	res = cuMemAlloc(&a_dev, n*n * sizeof(unsigned int));
	if (res != CUDA_SUCCESS) {
	    printf("cuMemAlloc (a) failed\n");
	    return -1;
	}
	/* b[] */
	res = cuMemAlloc(&b_dev, n*n * sizeof(unsigned int));
	if (res != CUDA_SUCCESS) {
	    printf("cuMemAlloc (b) failed\n");
	    return -1;
	}
	/* c[] */
	res = cuMemAlloc(&c_dev, n*n * sizeof(unsigned int));
	if (res != CUDA_SUCCESS) {
	    printf("cuMemAlloc (c) failed\n");
	    return -1;
	}

	gettimeofday(&tv_data_init_start, NULL);
	/* initialize A[] & B[] */
	for (i = 0; i < n; i++) {
	    idx = i*n;
	    for(j = 0; j < n; j++) {			
		a[idx++] = i;
	    }
	}
	for (i = 0; i < n; i++) {
	    idx = i*n;
	    for(j = 0; j < n; j++) {
		b[idx++] = i;
	    }
	}

	gettimeofday(&tv_h2d_start, NULL);
	/* upload a[] and b[] */
	res = cuMemcpyHtoD(a_dev, a, n*n * sizeof(unsigned int));
	if (res != CUDA_SUCCESS) {
	    printf("cuMemcpyHtoD (a) failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}
	res = cuMemcpyHtoD(b_dev, b, n*n * sizeof(unsigned int));
	if (res != CUDA_SUCCESS) {
	    printf("cuMemcpyHtoD (b) failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}
	gettimeofday(&tv_h2d_end, NULL);

	gettimeofday(&tv_conf_kern_start, NULL);
	/* set kernel parameters */
	res = cuParamSeti(function, 0, a_dev);	
	if (res != CUDA_SUCCESS) {
	    printf("cuParamSeti (a) failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}
	res = cuParamSeti(function, 4, a_dev >> 32);
	if (res != CUDA_SUCCESS) {
	    printf("cuParamSeti (a) failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}
	res = cuParamSeti(function, 8, b_dev);
	if (res != CUDA_SUCCESS) {
	    printf("cuParamSeti (b) failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}
	res = cuParamSeti(function, 12, b_dev >> 32);
	if (res != CUDA_SUCCESS) {
	    printf("cuParamSeti (b) failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}
	res = cuParamSeti(function, 16, c_dev);
	if (res != CUDA_SUCCESS) {
	    printf("cuParamSeti (c) failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}
	res = cuParamSeti(function, 20, c_dev >> 32);
	if (res != CUDA_SUCCESS) {
	    printf("cuParamSeti (c) failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}
	res = cuParamSeti(function, 24, n);
	if (res != CUDA_SUCCESS) {
	    printf("cuParamSeti (c) failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}
	res = cuParamSetSize(function, 28);
	if (res != CUDA_SUCCESS) {
	    printf("cuParamSetSize failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}
	gettimeofday(&tv_exec_start, NULL);
	//printf("kernel launch\n");
#ifndef DISABLE_SCHED_GPU
	rtx_gpu_launch(&handle);
#endif
#ifndef MUTEX_OFF
	pthread_mutex_lock(&mutex);
#endif
	/* launch the kernel */
	res = cuLaunchGrid(function, grid_x, grid_y);
	if (res != CUDA_SUCCESS) {
	    printf("cuLaunchGrid failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}
//printf("kernel sync\n");
#ifndef DISABLE_SCHED_GPU
	rtx_gpu_notify(&handle);
	rtx_gpu_sync(&handle);
#endif
#if 1
	cuCtxSynchronize();
#ifndef MUTEX_OFF
	pthread_mutex_unlock(&mutex);
#endif
	gettimeofday(&tv_exec_end, NULL);
	gettimeofday(&tv_d2h_start, NULL);
	/* download c[] */
	res = cuMemcpyDtoH(c, c_dev, n*n * sizeof(unsigned int));
	if (res != CUDA_SUCCESS) {
	    printf("cuMemcpyDtoH (c) failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}
	gettimeofday(&tv_d2h_end, NULL);

	/* Read back */
	for (i = 0; i < n; i++) {
	    idx = i*n;
	    for(j = 0; j < n; j++) {			
		dummy_c = c[idx++];
	    }
	}


	res = cuMemFree(a_dev);
	if (res != CUDA_SUCCESS) {
	    printf("cuMemFree (a) failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}
	res = cuMemFree(b_dev);
	if (res != CUDA_SUCCESS) {
	    printf("cuMemFree (b) failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}
	res = cuMemFree(c_dev);
	if (res != CUDA_SUCCESS) {
	    printf("cuMemFree (c) failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}

	res = cuModuleUnload(module);
	if (res != CUDA_SUCCESS) {
	    printf("cuModuleUnload failed: res = %lu\n", (unsigned long)res);
	    return -1;
	}

#ifdef DETAIL_TIME
	tvsub(&tv_data_init_start, &tv_mem_alloc_start, &tv);
	mem_alloc = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	tvsub(&tv_h2d_start, &tv_data_init_start, &tv);
	data_init = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	tvsub(&tv_h2d_end, &tv_h2d_start, &tv);
	h2d = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	tvsub(&tv_exec_start, &tv_conf_kern_start, &tv);
	configure_kernel = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	tvsub(&tv_exec_end, &tv_exec_start, &tv);
	exec = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	tvsub(&tv_d2h_end, &tv_d2h_start, &tv);
	d2h = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	fprintf(stdout,"-------===--------\n");
	fprintf(stdout,"MemAlloc: %f\n", mem_alloc);
	fprintf(stdout,"DataInit: %f\n", data_init);
	fprintf(stdout,"HtoD: %f\n", h2d);
	fprintf(stdout,"KernConf: %f\n", configure_kernel);
	fprintf(stdout,"Exec: %f\n", exec);
	fprintf(stdout,"DtoH: %f\n", d2h);
	fprintf(stdout,"-------===--------\n");
#endif
#endif
#ifndef DISABLE_SCHED_CPU
	gettime(&ts2);
	fprintf(stderr,"%llu\n",timespec_to_ns_sub(&ts1,&ts2));
	rt_wait_period();
#endif
    }

#ifndef DISABLE_SCHED_GPU
    rtx_gpu_close(&handle);
#endif
#ifndef DISABLE_SCHED_CPU
    rt_exit();
#endif
    res = cuCtxDestroy(ctx);
    if (res != CUDA_SUCCESS) {
	printf("cuCtxDestroy failed: res = %lu\n", (unsigned long)res);
	return -1;
    }

#ifdef DETAIL_TIME
    gettimeofday(&tv_total_end, NULL);
    tvsub(&tv_init_end, &tv_total_start, &tv);
    init_gpu = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

    tvsub(&tv_total_end, &tv_total_start, &tv);
    total = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;
    printf("Init: %f\n", init_gpu);
    printf("Total: %f\n", total);
#endif
    free(a);
    free(b);
    free(c);
    return 0;
}


