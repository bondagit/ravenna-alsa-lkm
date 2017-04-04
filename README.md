# RAVENNA/AES67 ALSA LKM #

## Introduction ##

TODO Quick summary

TODO Version

[Merging Technologies](http://www.merging.com)

## Architecture ##
The RAVENNA ALSA implementation is splitted into 2 parts:

1. A linux kernel module (LKM) : MergingRavennaALSA.ko

2. A a user land binary call the Daemon : Merging_RAVENNA_Daemon

### 1. The kernel part is responsible of ###
* Registered as an ALSA driver
* Generate and receive RTP audio packets
* PTP driven interrupt loop
* Netlink communication between user and kernel
	
### 2. The Daemon part is responsible of ###
* Web server
* High level RAVENNA/AES67 protocol implementation
  * mDNS discovery
  * SAP discovery
* Remote volume control
* RAVENNA devices sample rate arbitration
* Communication and configuration of the LKM

The Daemon cannot be launched if the LKM has not been previously inserted.
The LKM cannot be removed as long as the Daemon is running

**The Daemon is not provided with this package. This binary is built by Merging Technologies.**

### ALSA Features ###
* Volume control
* 1 to 8 FS support
* PCM up to 384 kHz
* Native DSD (64/128/256) support (DOP not supported)
* Interleaved and non-interleaved 16/24/32 bit integer formats
* Up to 64 I/O (I is not yet supported)


## Configuration ##
### Linux Kernel prerequisite ###
The following kernel config are required :

* NETFILTER
* HIGH_RES_TIMERS
* NETLINK
* Kernel > 2.4 (3.18 for DSD)

About the Kernel config, please ensure that at least scenario 2 described in the following doc is achieved
https://www.kernel.org/doc/Documentation/timers/NO_HZ.txt

For a better high res timer performance, the following option should be set at 1000 or more
CONFIG_HZ=1000

### ALSA ###
ALSA lib superior or equal to 1.0.29 for DSD support


## Compilation ##

TODO

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