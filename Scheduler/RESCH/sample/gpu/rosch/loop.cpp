#include <cuda.h>
#include <linux/sched.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/resource.h>
#include <sys/time.h>

/* for rosch */
#include <resch/api_ros_gpu.h>

int cuda_test_madd(unsigned int n)
{
  int i, j, idx;
  CUresult res;
  CUdevice dev;
  CUcontext ctx;
  CUfunction function;
  CUmodule module;
  CUdeviceptr a_dev, b_dev, c_dev;
  unsigned int *a = (unsigned int *)malloc(n * n * sizeof(unsigned int));
  unsigned int *b = (unsigned int *)malloc(n * n * sizeof(unsigned int));
  unsigned int *c = (unsigned int *)malloc(n * n * sizeof(unsigned int));
  int block_x, block_y, grid_x, grid_y;
  char fname[256];
  int loop;
  unsigned int dummy_b, dummy_c;

  /* block_x * block_y should not exceed 512. */
  block_x = n < 16 ? n : 16;
  block_y = n < 16 ? n : 16;
  grid_x = n / block_x;
  if (n % block_x != 0)
    grid_x++;
  grid_y = n / block_y;
  if (n % block_y != 0)
    grid_y++;

  res = cuInit(0);
  if (res != CUDA_SUCCESS)
  {
    printf("cuInit failed: res = %lu\n", (unsigned long)res);
    return -1;
  }

  res = cuDeviceGet(&dev, 0);
  if (res != CUDA_SUCCESS)
  {
    printf("cuDeviceGet failed: res = %lu\n", (unsigned long)res);
    return -1;
  }

  res = cuCtxCreate(&ctx, 0, dev);
  if (res != CUDA_SUCCESS)
  {
    printf("cuCtxCreate failed: res = %lu\n", (unsigned long)res);
    return -1;
  }

  for (loop = 0; loop < 1 /*LOOP_COUNT*/; loop++)
  {
    sprintf(fname, "./loop_gpu.cubin");
    res = cuModuleLoad(&module, fname);
    if (res != CUDA_SUCCESS)
    {
      printf("cuModuleLoad() failed\n");
      return -1;
    }
    res = cuModuleGetFunction(&function, module, "_Z4loopPjS_S_j");
    if (res != CUDA_SUCCESS)
    {
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
    if (res != CUDA_SUCCESS)
    {
      printf("cuFuncSetBlockShape() failed\n");
      return -1;
    }

    /* a[] */
    res = cuMemAlloc(&a_dev, n * n * sizeof(unsigned int));
    if (res != CUDA_SUCCESS)
    {
      printf("cuMemAlloc (a) failed\n");
      return -1;
    }
    /* b[] */
    res = cuMemAlloc(&b_dev, n * n * sizeof(unsigned int));
    if (res != CUDA_SUCCESS)
    {
      printf("cuMemAlloc (b) failed\n");
      return -1;
    }
    /* c[] */
    res = cuMemAlloc(&c_dev, n * n * sizeof(unsigned int));
    if (res != CUDA_SUCCESS)
    {
      printf("cuMemAlloc (c) failed\n");
      return -1;
    }

    /* initialize A[] & B[] */
    for (i = 0; i < n; i++)
    {
      idx = i * n;
      for (j = 0; j < n; j++)
      {
        a[idx++] = i;
      }
    }
    for (i = 0; i < n; i++)
    {
      idx = i * n;
      for (j = 0; j < n; j++)
      {
        b[idx++] = i;
      }
    }

    /* upload a[] and b[] */
    res = cuMemcpyHtoD(a_dev, a, n * n * sizeof(unsigned int));
    if (res != CUDA_SUCCESS)
    {
      printf("cuMemcpyHtoD (a) failed: res = %lu\n", (unsigned long)res);
      return -1;
    }
    res = cuMemcpyHtoD(b_dev, b, n * n * sizeof(unsigned int));
    if (res != CUDA_SUCCESS)
    {
      printf("cuMemcpyHtoD (b) failed: res = %lu\n", (unsigned long)res);
      return -1;
    }

    /* set kernel parameters */
    res = cuParamSeti(function, 0, a_dev);
    if (res != CUDA_SUCCESS)
    {
      printf("cuParamSeti (a) failed: res = %lu\n", (unsigned long)res);
      return -1;
    }
    res = cuParamSeti(function, 4, a_dev >> 32);
    if (res != CUDA_SUCCESS)
    {
      printf("cuParamSeti (a) failed: res = %lu\n", (unsigned long)res);
      return -1;
    }
    res = cuParamSeti(function, 8, b_dev);
    if (res != CUDA_SUCCESS)
    {
      printf("cuParamSeti (b) failed: res = %lu\n", (unsigned long)res);
      return -1;
    }
    res = cuParamSeti(function, 12, b_dev >> 32);
    if (res != CUDA_SUCCESS)
    {
      printf("cuParamSeti (b) failed: res = %lu\n", (unsigned long)res);
      return -1;
    }
    res = cuParamSeti(function, 16, c_dev);
    if (res != CUDA_SUCCESS)
    {
      printf("cuParamSeti (c) failed: res = %lu\n", (unsigned long)res);
      return -1;
    }
    res = cuParamSeti(function, 20, c_dev >> 32);
    if (res != CUDA_SUCCESS)
    {
      printf("cuParamSeti (c) failed: res = %lu\n", (unsigned long)res);
      return -1;
    }
    res = cuParamSeti(function, 24, n);
    if (res != CUDA_SUCCESS)
    {
      printf("cuParamSeti (c) failed: res = %lu\n", (unsigned long)res);
      return -1;
    }
    res = cuParamSetSize(function, 28);
    if (res != CUDA_SUCCESS)
    {
      printf("cuParamSetSize failed: res = %lu\n", (unsigned long)res);
      return -1;
    }

    ros_gsched_enqueue();
    std::cout << "[launch]:"
              << " pid: " << getpid() << " prio: " << getpriority(PRIO_PROCESS, 0) << std::endl;

    /* launch the kernel */
    res = cuLaunchGrid(function, grid_x, grid_y);
    if (res != CUDA_SUCCESS)
    {
      printf("cuLaunchGrid failed: res = %lu\n", (unsigned long)res);
      return -1;
    }

    cuCtxSynchronize();

    ros_gsched_dequeue();

    std::cout << "[finish]:"
              << " pid: " << getpid() << " prio: " << getpriority(PRIO_PROCESS, 0) << "\n"
              << std::endl;

    /* download c[] */
    res = cuMemcpyDtoH(c, c_dev, n * n * sizeof(unsigned int));
    if (res != CUDA_SUCCESS)
    {
      printf("cuMemcpyDtoH (c) failed: res = %lu\n", (unsigned long)res);
      return -1;
    }

    /* Read back */
    for (i = 0; i < n; i++)
    {
      idx = i * n;
      for (j = 0; j < n; j++)
      {
        dummy_c = c[idx++];
      }
    }

    res = cuMemFree(a_dev);
    if (res != CUDA_SUCCESS)
    {
      printf("cuMemFree (a) failed: res = %lu\n", (unsigned long)res);
      return -1;
    }
    res = cuMemFree(b_dev);
    if (res != CUDA_SUCCESS)
    {
      printf("cuMemFree (b) failed: res = %lu\n", (unsigned long)res);
      return -1;
    }
    res = cuMemFree(c_dev);
    if (res != CUDA_SUCCESS)
    {
      printf("cuMemFree (c) failed: res = %lu\n", (unsigned long)res);
      return -1;
    }

    res = cuModuleUnload(module);
    if (res != CUDA_SUCCESS)
    {
      printf("cuModuleUnload failed: res = %lu\n", (unsigned long)res);
      return -1;
    }
  }
  ros_gsched_exit(false);

  res = cuCtxDestroy(ctx);
  if (res != CUDA_SUCCESS)
  {
    printf("cuCtxDestroy failed: res = %lu\n", (unsigned long)res);
    return -1;
  }

  free(a);
  free(b);
  free(c);
  return 0;
}
