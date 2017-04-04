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

#include <linux/interrupt.h> // for tasklet

#include "module_main.h"
#include "module_timer.h"

static DECLARE_WAIT_QUEUE_HEAD(wq);

static struct tasklet_hrtimer my_hrtimer_;
static ktime_t prev_time_;
static uint64_t base_period_;
static uint64_t max_period_allowed;
static uint64_t min_period_allowed;
static int stop_;


enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
    int ret_overrun;
    ktime_t kt_now;
    ktime_t period;
    uint64_t next_wakeup;

    //do
    {
        t_clock_timer(&next_wakeup);
        kt_now = hrtimer_cb_get_time(timer);
        period = ktime_set(0, next_wakeup - ktime_to_ns(kt_now));

		if (ktime_to_ns(kt_now) > next_wakeup)
		{
			period = ktime_set(0, 0);
		}
		else if (ktime_to_ns(period) > max_period_allowed || ktime_to_ns(period) < min_period_allowed)
        {
            printk(KERN_INFO "Timer period out of range: %lld [ms]. Target period = %lld\n", ktime_to_ns(period) / 1000000, base_period_ / 1000000);
            if (ktime_to_ns(period) > (unsigned long)5E9L)
            {
                printk(KERN_ERR "Timer period greater than 5s, set it to 1s!\n");
                period = ktime_set(0,((unsigned long)1E9L)); //1s
            }
        }
        if (ktime_to_ns(kt_now) - ktime_to_ns(prev_time_) > max_period_allowed)
        {
            // comment it when running in VM
            printk(KERN_INFO "Timer wake up time period is greater than %lld [ms] (%lld [ms])\n", (uint64_t)(max_period_allowed / 1000000), (ktime_to_ns(kt_now) - ktime_to_ns(prev_time_)) /1000000);
        }
        prev_time_ = kt_now;

        if(stop_)
        {
            return HRTIMER_NORESTART;
        }
    }
    //while (ktime_to_ns(period) < 0); // this able to be rarely true

    ///ret_overrun = hrtimer_forward(timer, kt_now, period);
    ret_overrun = hrtimer_forward_now(timer, period);
    // comment it when running in VM
    if(ret_overrun > 1)
        printk(KERN_INFO "Timer overrun ! (%d times)\n", ret_overrun);
    return HRTIMER_RESTART;

}

int init_clock_timer(void)
{
	stop_ = 0;
	///hrtimer_init(&my_hrtimer_, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	tasklet_hrtimer_init(&my_hrtimer_, timer_callback, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
    ///my_hrtimer_.function = &timer_callback;

    //base_period_ = 100 * ((unsigned long)1E6L); // 100 ms
    base_period_ = 1333333; // 1.3 ms
    set_base_period(base_period_);

    //start_clock_timer(); //used when no daemon
	return 0;
}

void kill_clock_timer(void)
{
    //stop_clock_timer(); //used when no daemon
}

int start_clock_timer(void)
{
    ktime_t period = ktime_set(0, base_period_); //100 ms
	tasklet_hrtimer_start(&my_hrtimer_, period, HRTIMER_MODE_ABS);

	return 0;
}

void stop_clock_timer(void)
{

    tasklet_hrtimer_cancel(&my_hrtimer_);
	/*int ret_cancel = 0;
    while(hrtimer_callback_running(&my_hrtimer_))
        ++ret_cancel;

    if(hrtimer_active(&my_hrtimer_) != 0)
        ret_cancel = hrtimer_cancel(&my_hrtimer_);
    if (hrtimer_is_queued(&my_hrtimer_) != 0)
        ret_cancel = hrtimer_cancel(&my_hrtimer_);*/
}

void get_clock_time(uint64_t* clock_time)
{
	ktime_t kt_now;
	kt_now = ktime_get();
	*clock_time = (uint64_t)ktime_to_ns(kt_now);

	/*raw_spinlock_t  *timer_lock = &my_hrtimer_.timer.base->cpu_base->lock;
	int this_cpu = smp_processor_id(); // SMP issue
	bool lock = this_cpu != timer_cpu_;
	if (lock)
		raw_spin_lock(timer_lock);
	kt_now = hrtimer_cb_get_time(&my_hrtimer_.timer);
	if (lock)
		raw_spin_unlock(timer_lock);
	*clock_time = (uint64_t)ktime_to_ns(kt_now);*/
	return;
}

void set_base_period(uint64_t base_period)
{
    base_period_ = base_period;
    min_period_allowed = base_period_ / 7;
    max_period_allowed = (base_period_ * 10) / 6;
    printk(KERN_INFO "Base period set to %lld ns\n", base_period_);
}