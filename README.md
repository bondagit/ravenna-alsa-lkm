# RAVENNA/AES67 ALSA LKM #

## Introduction ##

TODO Quick summary
TODO Version
[Merging Technologies](http://www.merging.com)

## Architecture ##

The RAVENNA ALSA implementation is splitted into 2 parts:

    * A linux kernel module (LKM) : MergingRavennaALSA.ko
    * A a user land binary call the Daemon : Merging_RAVENNA_Daemon

* The kernel part is responsible of the following:

    Registered as an ALSA driver
    Generate and receive RTP audio packets
    PTP driven interrupt loop
    Netlink communication between user and kernel

* The Daemon part is responsible of the following :

    Web server
    High level RAVENNA/AES67 protocol implementation
        mDNS discovery
        SAP discovery
    Remote volume control
    RAVENNA devices sample rate arbitration
    Communication and configuration of the LKM

The Daemon cannot be launched if the LKM has not been previously inserted.

The LKM cannot be removed as long as the Daemon is running

### ALSA Features ###

* Volume control
* 1 to 8 FS support
* PCM up to 384 kHz
* Native DSD (64/128/256) support (DOP not supported)
* Interleaved and non-interleaved 16/24/32 bit integer formats
* Up to 64 I/O (I is not yet supported)

### mDNS implementation ###

The RAVENNA protocol uses mDNS. Depending on the platform/distribition the Daemon will use Bonjour or Avahi libraries.

If Avahi is present in the system, the Daemon have to use that library. If Avahi is not present in the system, a built-in Bonjour implementation will be used instead.

In order to correctly build the daemon, we need to know if Avahi is present or not in the system.

## Linux Kernel prerequisite ##

* NETFILTER
* HIGH_RES_TIMERS
* NETLINK
* Kernel > 2.4 (3.18 for DSD)
* Kernel Config

About the Kernel config, please ensure that at least scenario 2 described in the following doc is achieved
https://www.kernel.org/doc/Documentation/timers/NO_HZ.txt
The following option should be set at 1000 or more
CONFIG_HZ=1000
### ALSA ###

ALSA lib superior or equal to 1.0.29 for DSD support


## Compilation ##



### How do I get set up? ###

* Summary of set up
* Configuration
* Dependencies
* Database configuration
* How to run tests
* Deployment instructions

### Contribution guidelines ###

* Writing tests
* Code review
* Other guidelines

### Who do I talk to? ###

* Repo owner or admin
* Other community or team contact