#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <resch/api_ros_gpu.h>

int cuda_test_madd(unsigned int n);

pid_t Fork()
{
  pid_t pid;

  pid = fork();
  if (-1 == pid)
  {
    warn("can not fork");
  }
  return (pid);
}

void child(int prio)
{
  sleep(1);
  setpriority(PRIO_PROCESS, 0, prio);
  cuda_test_madd(1000);
}

void multi_fork()
{
  int i = 0;
  int max = 10;

  for (i = 0; i < max; ++i)
  {
    pid_t pid = Fork();
    if (-1 == pid)
    {
      break;
    }
    else if (0 == pid)
    {
      child(i);
      exit(EXIT_SUCCESS);
    }
  }
}

void multi_wait()
{
  for (;;)
  {
    pid_t pid;
    int status = 0;

    pid = wait(&status);

    if (pid == -1)
    {
      if (ECHILD == errno)
      {
        break;
      }
      else if (EINTR == errno)
      {
        continue;
      }
      err(EXIT_FAILURE, "wait error");
    }
  }
}

int main(int argc, char *argv[])
{
  ros_gsched_init();
  
	multi_fork();
  sleep(3);
  multi_wait();

  ros_gsched_exit(true);

  exit(EXIT_SUCCESS);
}
