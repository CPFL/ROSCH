#ifndef __NVC0_REG_H__
#define __NVC0_REG_H__

#define NVDEV_ENGINE_GR_INTR     0x1000
#define NVDEV_ENGINE_FIFO_INTR   0x0100
#define NVDEV_ENGINE_COPY2_INTR  0x0080
#define NVDEV_ENGINE_COPY1_INTR  0x0040
#define NVDEV_ENGINE_COPY0_INTR  0x0020

#define NV04_PTIMER_INTR_0      0x009100
#define NV04_PTIMER_INTR_EN_0   0x009140
#define NV04_PTIMER_NUMERATOR   0x009200
#define NV04_PTIMER_DENOMINATOR 0x009210
#define NV04_PTIMER_TIME_0      0x009400
#define NV04_PTIMER_TIME_1      0x009410
#define NV04_PTIMER_ALARM_0     0x009420

#define NV_INTR_EN 0x140
#define NV_INTR_STAT 0x100


struct irq_num {
    int num;
    char dev_name[30];
};

const struct irq_num nvc0_irq_num[] ={
    { 0x04000000,"ENGINE_DISP" },  /* DISP first, so pageflip timestamps work. */
    { 0x00000001,"ENGINE_PPP"   },
    { 0x00000020,"ENGINE_COPY0" },
    { 0x00000040,"ENGINE_COPY1" },
    { 0x00000080,"ENGINE_COPY2" },
    { 0x00000100,"ENGINE_FIFO"  },
    { 0x00001000,"ENGINE_GR"    },
    { 0x00002000,"SUBDEV_FB"    },
    { 0x00008000,"ENGINE_BSP"   },
    { 0x00040000,"SUBDEV_THERM" },
    { 0x00020000,"ENGINE_VP"    },
    { 0x00100000,"SUBDEV_TIMER" },
    { 0x00200000,"SUBDEV_GPIO"  },  /* PMGR->GPIO */
    { 0x00200000,"SUBDEV_I2C"   },  /* PMGR->I2C/AUX */
    { 0x01000000,"SUBDEV_PWR"   },
    { 0x02000000,"SUBDEV_LTCG"  },
    { 0x08000000,"SUBDEV_FB"    },
    { 0x10000000,"SUBDEV_BUS"   },
    { 0x40000000,"SUBDEV_IBUS"  },
    { 0x80000000,"ENGINE_SW"    },
    {0,""},
    {}
};

#endif
