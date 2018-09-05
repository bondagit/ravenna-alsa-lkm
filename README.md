# RAVENNA/AES67 ALSA LKM #

## Introduction ##

This package provides an ALSA driver to enable RAVENNA and AES67 audio over IP protocols support to various Linux distributions.

TODO Version

More information about Merging Technologies, RAVENNA and AES67 can be found on : [here](https://www.merging.com/highlights/audio-networking#audio-networking)

## Architecture ##
The RAVENNA ALSA implementation is splitted into 2 parts:

1. A linux kernel module (LKM) : MergingRavennaALSA.ko

2. A a user land binary call the Daemon : Merging_RAVENNA_Daemon

### The kernel part is responsible of ###
* Register as an ALSA driver
* Generate and receive RTP audio packets
* PTP driven interrupt loop
* Netlink communication between user and kernel
	
### The Daemon part is responsible of ###
* Web server
* High level RAVENNA/AES67 protocol implementation
  * mDNS discovery
  * SAP discovery
* Remote volume control
* RAVENNA devices sample rate arbitration
* Communication and configuration of the LKM

The Daemon cannot be launched if the LKM has not been previously inserted.
The LKM cannot be removed as long as the Daemon is running

**The Daemon is not provided with this package. This binary is built for each specific platform by Merging Technologies and delivered under a license agreement.**

### ALSA Features ###
* Volume control
* PCM up from 44.1 kHz to 384 kHz
* Native DSD (64/128/256) support (DOP not supported)
* Interleaved and non-interleaved 16/24/32 bit integer formats
* Up to 64 playback streams (Capture streams are not yet supported)


## Configuration ##
### Linux Kernel prerequisite ###
The following kernel config are required :

* NETFILTER
* HIGH_RES_TIMERS
* NETLINK
* Kernel > 2.4 (3.18 for DSD)
* Kernel < 4.13 (Netfilter issue to be solve)

About the Kernel config, please ensure that at least scenario 2 described in the following doc is achieved
https://www.kernel.org/doc/Documentation/timers/NO_HZ.txt

For a better high res timer performance, the following option should be set at 1000 or more
CONFIG_HZ=1000

## Setup ##

### Hardware (pro audio only) ###
* A PTP master device (this driver do not act as a PTP master). E.g. Horus/Hapi
* A switch supporting multicast traffic (RFC 1112), multicast forwarding, IGMPv2 (RFC 2236), IGMP snooping (RFC 4541). 
* A AD/DA AES67 or RAVENNA device. E.g. Horus/Hapi

### ALSA ###
ALSA lib superior or equal to 1.0.29 for DSD support


## Compilation ##

```
#!makefile
git clone -b capture https://AlsaIntegrator@bitbucket.org/MergingTechnologies/ravenna-alsa-lkm.git .
cd driver
make
sudo insmod MergingRavennaALSA.ko
```


## Contact ##
alsa@merging.com