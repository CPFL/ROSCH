#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <queue>
#include "api_ros_gpu.h"

pthread_mutex_t *launch_mutex;
pthread_mutex_t *value_mutex;
int *maxvalue;

int shm_launch_mutex_id;
int shm_value_mutex_id;
int shm_maxvalue_id;

int ros_gsched_init()
{
  pthread_mutexattr_t mat_l, mat_v;

  shm_launch_mutex_id = shmget(IPC_PRIVATE, sizeof(pthread_mutex_t), 0600);
  if (shm_launch_mutex_id < 0)
  {
    perror("shmget");
    return 1;
  }

  shm_value_mutex_id = shmget(IPC_PRIVATE, sizeof(pthread_mutex_t), 0600);
  if (shm_value_mutex_id < 0)
  {
    perror("shmget");
    return 1;
  }

  shm_maxvalue_id = shmget(IPC_PRIVATE, sizeof(int), 0666);
  if (shm_maxvalue_id < 0)
  {
    perror("shmget");
    return 1;
  }

  launch_mutex = (pthread_mutex_t *)shmat(shm_launch_mutex_id, NULL, 0);
  if (launch_mutex == (void *)-1)
  {
    perror("shmat");
    return 1;
  }

  value_mutex = (pthread_mutex_t *)shmat(shm_value_mutex_id, NULL, 0);
  if (value_mutex == (void *)-1)
  {
    perror("shmat");
    return 1;
  }

  maxvalue = (int *)shmat(shm_maxvalue_id, NULL, 0);
  if (maxvalue == (void *)-1)
  {
    perror("shmat");
    return 1;
  }

  pthread_mutexattr_init(&mat_l);
  if (pthread_mutexattr_setpshared(&mat_l, PTHREAD_PROCESS_SHARED) != 0)
  {
    perror("pthread_mutexattr_setpshared");
    return 1;
  }

  pthread_mutexattr_init(&mat_v);
  if (pthread_mutexattr_setpshared(&mat_v, PTHREAD_PROCESS_SHARED) != 0)
  {
    perror("pthread_mutexattr_setpshared");
    return 1;
  }

  pthread_mutex_init(launch_mutex, &mat_l);
  pthread_mutex_init(value_mutex, &mat_v);

  *maxvalue = 0;

  return 0;
}

int ros_gsched_enqueue()
{
  int my_prio;
  int ret;

  my_prio = getpriority(PRIO_PROCESS, 0);

  while (1)
  {
    if (my_prio > *maxvalue)
    {
      pthread_mutex_lock(value_mutex);
      *maxvalue = my_prio;
      pthread_mutex_unlock(value_mutex);
    }

    /* sleep for scheduling bloked launch requirement */
    usleep(10);

    if (*maxvalue == my_prio)
    {
      if (!(ret = pthread_mutex_trylock(launch_mutex)))
      {
        pthread_mutex_lock(value_mutex);
        *maxvalue = -1;
        pthread_mutex_unlock(value_mutex);
        break;
      }
    }
  }

  return 0;
}

int ros_gsched_dequeue()
{
  if (pthread_mutex_unlock(launch_mutex) != 0)
  {
    perror("pthread_mutex_unlock");
    return 1;
  }
  return 0;
}

int ros_gsched_exit(bool option)
{
  if (option == false)
  {
    shmdt(launch_mutex);
    shmdt(value_mutex);
    shmdt(maxvalue);
  }
  else
  {
    if (shmctl(shm_launch_mutex_id, IPC_RMID, NULL) != 0)
    {
      perror("shmctl");
      return 1;
    }
    if (shmctl(shm_value_mutex_id, IPC_RMID, NULL) != 0)
    {
      perror("shmctl");
      return 1;
    }
    if (shmctl(shm_maxvalue_id, IPC_RMID, NULL) != 0)
    {
      perror("shmctl");
      return 1;
    }
  }
  return 0;
}
