/****************************************************************************
*
*  Module Name    : MTAL_LKernelAPI.c
*  Version        :
*
*  Abstract       : MT Abstraction Layer for Linux Kernel
*
*  Written by     : Beguec Frederic, Baume Florian
*  Date           : 16/03/2016
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

#include "MTAL_LKernelAPI.h"
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0)
#include <linux/timekeeping.h>
#else
#include <linux/time.h>
#endif

#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h> /*NF_IP_PRE_FIRST*/

#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/if.h>

#include <linux/inet.h> /*in_aton()*/

#include <linux/printk.h>


void *malloc (size_t __size)
{
    //printk(KERN_INFO "malloc %lu\n", __size);
    return vmalloc(__size);
}
void *calloc (size_t __nmemb, size_t __size)
{
    //printk(KERN_INFO "vmalloc %lu\n", __nmemb *__size);
    //return vzalloc(__nmemb * __size);
    return vmalloc(__nmemb * __size);
}
/*void *realloc (void *__ptr, size_t __size)
{
    return krealloc(__ptr, __size, GFP_KERNEL);
}*/
void free (void *__ptr)
{
    //printk(KERN_INFO "free\n");
    //kfree(__ptr);
    vfree(__ptr);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,16,0)
#include <linux/stdarg.h>
#else
#include <stdarg.h>
#endif
int MTAL_LK_print(const char *fmt, ...)
{
    va_list args;
    int res = 0;
    va_start(args, fmt);
    res = vprintk(fmt, args);
    va_end(args);
    return res;
}

uint64_t MTAL_LK_GetCounterTime(void) // 100 ns precision
{
    uint64_t timeVal = 0ull;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
    struct timespec64 ts64;
    ktime_get_raw_ts64(&ts64);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0)
    struct timespec64 ts64;
    getrawmonotonic64(&ts64);
#else
    struct timespec ts64;
    getrawmonotonic(&ts64);
#endif // LINUX_VERSION_CODE
    timeVal += (uint64_t)ts64.tv_sec * 10000000ull;
    timeVal += (uint64_t)ts64.tv_nsec / 100ull;
    return timeVal;
}

uint64_t MTAL_LK_GetCounterFreq(void)
{
    return 1000000000ull;
}

uint64_t MTAL_LK_GetSystemTime(void)
{
    uint64_t timeVal = 0ull;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,20,0)
    struct timespec64 ts64;
    ktime_get_real_ts64(&ts64);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0)
    struct timespec64 ts64;
    getnstimeofday64(&ts64);
#else
    struct timespec ts64;
    getnstimeofday(&ts64);
#endif // LINUX_VERSION_CODE
    timeVal += (uint64_t)ts64.tv_sec * 10000000ull;
    timeVal += (uint64_t)ts64.tv_nsec / 100ull;
    return timeVal;
}
