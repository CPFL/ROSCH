/*
 *  libresch_gpu.c: the library for the RESCH gpu
 * 	
 */

#include "api_gpu.h"
#include "api.h"
#include <resch-api.h>
#include <resch-config.h>
#include <linux/sched.h>
#include <pthread.h>

#define discard_arg(arg)	asm("" : : "r"(arg))
#define gettid() syscall(SYS_gettid)

struct rtxGhandle **ghandler;

int fd = 0; /* module's file discripter */

/**
 * internal function for gpu scheduler core, using ioctl() system call.
 */
static inline int __rtx_gpu_ioctl(unsigned long cmd, unsigned long val)
{
    int ret;

    if(!fd){
	fd = open(RESCH_DEVNAME, O_RDWR);
	if (fd < 0) {
	    printf("Error: failed to access the module!\n");
	    return RES_FAULT;
	}
    }
    ret = ioctl(fd, cmd, val);

    return ret;
}

/* consideration of the interference with the implementation of the CPU cide */
static void gpu_kill_handler(int signum)
{
    discard_arg(signum);
    rtx_gpu_exit();
    kill(0, SIGINT);
}

int rtx_gpu_init(void)
{
    struct sigaction sa_kill;
    int ret;

    ret = rt_init();

    /* reregister the KILL signal. */
    memset(&sa_kill, 0, sizeof(sa_kill));
    sigemptyset(&sa_kill.sa_mask);
    sa_kill.sa_handler = gpu_kill_handler;
    sa_kill.sa_flags = 0;
    sigaction(SIGINT, &sa_kill, NULL); /* */

    return ret;
}

int rtx_gpu_exit(void)
{

    return rt_exit();
}

static int Ghandle_init(struct rtxGhandle **arg)
{
    if (!(*arg)) {
	*arg =(struct rtxGhandle*)malloc(sizeof(struct rtxGhandle));
	if (!(*arg))
	    return -ENOMEM;
	
	ghandler = arg;
    }

    return 1;
}


int rtx_gpu_device_advice(struct rtxGhandle **arg)
{
    int ret;
    struct rtxGhandle *handle = *arg; 

    if ( !(handle) )
	return -EINVAL;

    return __rtx_gpu_ioctl(GDEV_IOCTL_GETDEV, (unsigned long)handle);
}


#define DEV_NVIDIA_CTL "/dev/nvidiactl"
int rtx_gpu_open(struct rtxGhandle **arg, unsigned int dev_id, unsigned int vdev_id)
{
    int ret;
    struct rtxGhandle *handle;
    int fd_nv;
    
    /* module check*/
    fd = open(RESCH_DEVNAME, O_RDWR);
    if (fd < 0) {
	printf("Error: failed to access the module!: %s\n", strerror(fd));
	return -ENODEV;
    }

    if (Ghandle_init(arg)<0){
	return -ENOMEM;
    }

    handle = *arg;
    handle->dev_id = dev_id;
    handle->vdev_id = vdev_id;
    handle->sched_flag = GPU_SCHED_FLAG_OPEN;
    handle->nvdesc = NULL;
    handle->sync_flag = DEFAULT_GSYNC_FLAG;
    //handle->sync_flag =GSYNC_FENCE_SPIN;

    if ((ret = __rtx_gpu_ioctl(GDEV_IOCTL_CTX_CREATE, (unsigned long)handle))<0){
    	return -ENODEV;
    }
    
    /*driver check*/
    fd_nv = open(DEV_NVIDIA_CTL, O_RDWR);
    if (fd_nv >= 0) {
	close(fd_nv);
	return rtx_nvrm_init(arg, dev_id);
    }else{
	return ret;
    }

}

int rtx_gpu_launch(struct rtxGhandle **arg)
{
    int ret;
    struct rtxGhandle *handle = *arg; 

    if ( !(handle) )
	return -EINVAL;
   
    fprintf(stderr,"[%s-%d-%d] goto launch!\n",__func__,handle->vdev_id,sched_getcpu());

    if ( !(handle->sched_flag & GPU_SCHED_FLAG_OPEN)){
	fprintf(stderr, "rtx_gpu_open is not called yet\n");
	fprintf(stderr, "rtx_gpu_open was not called\n");
	fprintf(stderr,	"Please call rtx_gpu_open before rtx_gpu_launch!\n");
	return -EINVAL;
    }

    handle->sched_flag &= ~GPU_SCHED_FLAG_SYNCH;
    handle->sched_flag |= GPU_SCHED_FLAG_LAUNCH;

    ret = __rtx_gpu_ioctl(GDEV_IOCTL_LAUNCH, (unsigned long)handle);
    return ret;
}


static void *rtx_gpu_notify_thread(struct rtxGhandle **arg)
{
    int ret;
    struct rtxGhandle *handle = *arg; 
    while(1){
	ret = __rtx_gpu_ioctl(GDEV_IOCTL_NOTIFY, (unsigned long)handle);
	if(ret == 0x600dcafe)
	    break;
#ifdef USE_NVIDIA_DRIVER
	rtx_nvrm_notify(arg);
#endif
    }
    pthread_detach(pthread_self());
    return NULL;
}

int rtx_gpu_notify(struct rtxGhandle **arg)
{
    int ret;
    struct rtxGhandle *handle = *arg; 
    
    if ( !(handle) )
	return -EINVAL;

    if (handle->nvdesc)
    {
	switch(handle->sync_flag){
	    case GSYNC_FENCE_SPIN:
	    case GSYNC_FENCE_YIELD:
		rtx_nvrm_fence(arg);
		break;
	    case GSYNC_NOTIFY:
		rtx_nvrm_notify(arg);
		ret = __rtx_gpu_ioctl(GDEV_IOCTL_NOTIFY, (unsigned long)handle);
		break;
	    default:
		printf("Unknown sync flag:%d\n",handle->sync_flag);
		break;
	}
    }else{
	/*nouveau driver*/
	ret = __rtx_gpu_ioctl(GDEV_IOCTL_NOTIFY, (unsigned long)handle);
    }

    return ret;
}

int rtx_gpu_sync(struct rtxGhandle **arg)
{
    struct rtxGhandle *handle = *arg; 
    int ret;
    
    if ( !(handle) )
	return -EINVAL;
    
    if ( !((handle)->sched_flag & GPU_SCHED_FLAG_LAUNCH)){
	fprintf(stderr, "rtx_gpu_launch is not called yet\n");
	fprintf(stderr,	"Sync must call after launch!\n");
	return -EINVAL;
    }

    if ( ((handle)->sched_flag & GPU_SCHED_FLAG_SYNCH)){
	fprintf(stderr, "rtx_gpu_sync has already called\n");
	fprintf(stderr,	"Sync is only one call per one handle.\n");
	return -EINVAL;
    }


    handle->sched_flag |= GPU_SCHED_FLAG_SYNCH;
    handle->sched_flag &= ~GPU_SCHED_FLAG_LAUNCH;

    if (handle->nvdesc)
	switch(handle->sync_flag){
	    case GSYNC_FENCE_SPIN:
	    case GSYNC_FENCE_YIELD:
		ret = rtx_nvrm_fence_poll(arg, handle->sync_flag, handle->cid);
		__rtx_gpu_ioctl(GDEV_IOCTL_SYNC, (unsigned long)handle);
		break;
	    case GSYNC_NOTIFY:
//VV		rtx_nvrm_notify(arg);
		ret = __rtx_gpu_ioctl(GDEV_IOCTL_SYNC, (unsigned long)handle);
		break;
	    default: 
		    break;
	}
    else{
	ret = __rtx_gpu_ioctl(GDEV_IOCTL_SYNC, (unsigned long)handle);
    }
    return ret;
}

int rtx_gpu_close(struct rtxGhandle **arg)
{
    struct rtxGhandle *handle = *arg; 
    
    if ( !handle )
	return -EINVAL;

    if ( !(handle->sched_flag & GPU_SCHED_FLAG_OPEN)){
	fprintf(stderr, "rtx_gpu_open is not called yet\n");
	return -EINVAL;
    }

    int ret = __rtx_gpu_ioctl(GDEV_IOCTL_CLOSE, (unsigned long)handle);

    free(handle);
    if(fd)
	close(fd);
 
    return ret;
}

int rtx_gpu_setcid(struct rtxGhandle **arg, int new_cid)
{
    struct rtxGhandle *handle = *arg;

    if(!fd){
	return -ENODEV;
    }

    if( !(*ghandler) ){
	return -ENODEV;
    }

    return (*ghandler)->cid;
#if 0

    if( !(arg) ){
	__arg = *ghandler;
    }
 
    __arg->setcid = new_cid;

    return __rtx_gpu_ioctl(GDEV_IOCTL_SETCID, (unsigned long)__arg);
#endif
}
