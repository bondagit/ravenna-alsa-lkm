/****************************************************************************
*
*  Module Name    : module_timer.c
*  Version        : 
*
*  Abstract       : RAVENNA/AES67 ALSA LKM
*
*  Written by     : Baume Florian
*  Date           : 15/04/2016
*  Modified by    :
*  Date           :
*  Modification   :
*  Known problems : None
*
* Copyright(C) 2017 Merging Technologies
*
* RAVENNA/AES67 ALSA LKM is free software; you can redistribute it and / or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* RAVENNA/AES67 ALSA LKM is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with RAVAENNA ALSA LKM ; if not, see <http://www.gnu.org/licenses/>.
*
****************************************************************************/

#include <linux/module.h>
#include <linux/atomic.h>
#include <linux/interrupt.h> // for tasklet
#include <linux/wait.h>
#include <linux/cpumask.h>
#include <linux/smp.h>

#include "module_main.h"
#include "module_timer.h"

/* Module parameter for CPU core pinning */
static int audio_cpu_affinity = -1; /* -1 means no CPU pinning (any CPU) */
module_param(audio_cpu_affinity, int, 0444);
MODULE_PARM_DESC(audio_cpu_affinity, "CPU core to pin the hrtimer to (-1 for any CPU, default -1)");


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)

struct tasklet_hrtimer {
	struct hrtimer		timer;
	struct tasklet_struct	tasklet;
	enum hrtimer_restart	(*function)(struct hrtimer *);
};

static struct tasklet_hrtimer my_hrtimer_;

static inline
void tasklet_hrtimer_cancel(struct tasklet_hrtimer *ttimer)
{
	hrtimer_cancel(&ttimer->timer);
	tasklet_kill(&ttimer->tasklet);
}

static enum hrtimer_restart __hrtimer_tasklet_trampoline(struct hrtimer *timer)
{
	struct tasklet_hrtimer *ttimer =
		container_of(timer, struct tasklet_hrtimer, timer);

	tasklet_hi_schedule(&ttimer->tasklet);
	return HRTIMER_NORESTART;
}

static void __tasklet_hrtimer_trampoline(unsigned long data)
{
	struct tasklet_hrtimer *ttimer = (void *)data;
	enum hrtimer_restart restart;
	restart = ttimer->function(&ttimer->timer);
	if (restart != HRTIMER_NORESTART)
		hrtimer_restart(&ttimer->timer);
}

static void tasklet_hrtimer_init(struct tasklet_hrtimer *ttimer,
			  enum hrtimer_restart (*function)(struct hrtimer *),
			  clockid_t which_clock, enum hrtimer_mode mode)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
    hrtimer_setup(&ttimer->timer, __hrtimer_tasklet_trampoline, which_clock, mode);
#else
	hrtimer_init(&ttimer->timer, which_clock, mode);
	ttimer->timer.function = __hrtimer_tasklet_trampoline;
#endif
	tasklet_init(&ttimer->tasklet, __tasklet_hrtimer_trampoline,
		     (unsigned long)ttimer);
	ttimer->function = function;
}

static inline
void tasklet_hrtimer_start(struct tasklet_hrtimer *ttimer, ktime_t time,
			   const enum hrtimer_mode mode)
{
	hrtimer_start(&ttimer->timer, time, mode);
}
#endif

struct start_clock_timer_info {
    ktime_t period;
};

static void start_clock_timer_on_cpu(void *info)
{
    struct start_clock_timer_info *timer_info = info;
    ktime_t period = timer_info->period;

    tasklet_hrtimer_start(&my_hrtimer_, period,
                  audio_cpu_affinity != -1 ? HRTIMER_MODE_ABS_PINNED : HRTIMER_MODE_ABS);
}

static uint64_t base_period_;
static uint64_t max_period_allowed;
static uint64_t min_period_allowed;
static atomic_t stop_ = ATOMIC_INIT(0);


static enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
    int ret_overrun;
    ktime_t period;
    uint64_t next_wakeup;
    uint64_t now;

    do
    {
        get_clock_time(&now);
        t_clock_timer(&next_wakeup, now);
        period = ktime_set(0, next_wakeup - now);
        if (now > next_wakeup)
        {
            //printk(KERN_INFO "Timer won't sleep, clock_timer is recall instantly\n");
            period = ktime_set(0, 0);
        }
        else if (ktime_to_ns(period) > READ_ONCE(max_period_allowed) || 
                    ktime_to_ns(period) < READ_ONCE(min_period_allowed))
        {
            if (ktime_to_ns(period) > (unsigned long)5E9L)
            {
                //printk(KERN_ERR "Timer period greater than 5s, set it to 1s!\n");
                period = ktime_set(0,((unsigned long)1E9L)); //1s
            }
        }

        if (atomic_read(&stop_))
        {
            return HRTIMER_NORESTART;
        }
    }
    while (ktime_to_ns(period) == 0); // this able to be rarely true

    ///ret_overrun = hrtimer_forward(timer, kt_now, period);
    ret_overrun = hrtimer_forward_now(timer, period);
    // comment it when running in VM
    /*if(ret_overrun > 1)
        printk(KERN_INFO "Timer overrun ! (%d times)\n", ret_overrun);*/
    return HRTIMER_RESTART;

}

static int validate_audio_cpu_affinity(int cpu)
{
    /* Validate CPU core number */
    if (cpu == -1)
    {
        /* -1 means no pinning (any CPU) */
        return 0;
    }
    
    if (cpu < 0 || cpu >= nr_cpu_ids)
    {
        printk(KERN_ERR "MergingRavennaALSA: Invalid CPU core %d (valid range: 0-%d or -1 for any)", 
               cpu, nr_cpu_ids - 1);
        return -EINVAL;
    }
    
    if (!cpu_online(cpu))
    {
        printk(KERN_WARNING "MergingRavennaALSA: CPU core %d is not online", cpu);
        return -EINVAL;
    }
    
    return 0;
}

int init_clock_timer(void)
{
    int ret;
    
    /* Validate the CPU core parameter */
    ret = validate_audio_cpu_affinity(audio_cpu_affinity);
    if (ret != 0)
    {
        printk("MergingRavennaALSA: Failed to validate CPU audio affinity parameter");
        return ret;
    }
    
    if (audio_cpu_affinity != -1)
    {
        printk("MergingRavennaALSA: Pinning hrtimer to CPU core %d", audio_cpu_affinity);
    }
    else
    {
        printk("MergingRavennaALSA: hrtimer will run on any available CPU core");
    }
    
    atomic_set(&stop_, 0);
    smp_wmb();
    tasklet_hrtimer_init(&my_hrtimer_, timer_callback, CLOCK_MONOTONIC/*_RAW*/, 
        audio_cpu_affinity != -1 ? HRTIMER_MODE_ABS_PINNED : HRTIMER_MODE_ABS);
    WRITE_ONCE(base_period_, 1000000); /* 1ms default (AES67 48 frames @ 48kHz) */
    set_base_period(1000000);
    return 0;
}

void kill_clock_timer(void)
{
    stop_clock_timer(); //used when no daemon
}

int start_clock_timer(void)
{
    uint64_t period = READ_ONCE(base_period_);
    ktime_t expiry;

    atomic_set(&stop_, 0);
    smp_wmb(); /* ensure stop_=0 visible before timer fires */

    expiry = ktime_add(ktime_get(), ns_to_ktime(period));

    if (audio_cpu_affinity != -1)
    {
        struct start_clock_timer_info info = { .period = expiry };
        smp_call_function_single(audio_cpu_affinity, start_clock_timer_on_cpu,
                                 &info, true);
    }
    else
    {
        tasklet_hrtimer_start(&my_hrtimer_, expiry, HRTIMER_MODE_ABS);
    }

    return 0;
}

void stop_clock_timer(void)
{
    atomic_set(&stop_, 1);
    smp_wmb();
    tasklet_hrtimer_cancel(&my_hrtimer_);
}

void get_clock_time(uint64_t* clock_time)
{
    ktime_t kt_now;
    kt_now = ktime_get();
    *clock_time = (uint64_t)ktime_to_ns(kt_now);

}

void set_base_period(uint64_t base_period)
{
    WRITE_ONCE(base_period_, base_period);
    WRITE_ONCE(min_period_allowed, base_period / 7);
    WRITE_ONCE(max_period_allowed, (base_period * 10) / 6);
    printk(KERN_INFO "ravenna: base period set to %llu ns\n",
           (unsigned long long)base_period);
}

void update_base_period(uint32_t tic_frame_size, uint32_t sample_rate)
{
    uint64_t period_ns;
    if (sample_rate == 0)
        return;
    period_ns = ((uint64_t)tic_frame_size * 1000000000ULL) / sample_rate;
    set_base_period(period_ns);
}
