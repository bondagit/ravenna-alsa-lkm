# Changelog

## v1.1.74 (2019-06-28)

#### Enhancements:

- [RAV-980](https://jira.merging.com/browse/RAV-980) ALSA mmap support
- [RAV-988](https://jira.merging.com/browse/RAV-988) Remove the 8 channels limit of public ALSA driver when a Merging product is present
- [RAV-944](https://jira.merging.com/browse/RAV-944) Webapp : check if user provides a valid IP in
- [RAV-947](https://jira.merging.com/browse/RAV-947) Global safety playout delay added
- [RAV-987](https://jira.merging.com/browse/RAV-987) SSM option added
- [ZOEM-195](https://jira.merging.com/browse/ZOEM-195) Sources can be enabled/disabled

#### Bug Fixes:

- [RAV-990](https://jira.merging.com/browse/RAV-990) Advanded webpage may infinity popup new windows
- [RAV-957](https://jira.merging.com/browse/RAV-957) Wrong TTL value in source's SDP
- [ZOEM-185](https://jira.merging.com/browse/ZOEM-185) Session sink reports RTP status error while the audio is correct

---

## v1.1.93 (2020-06-2)

#### New features:

- REST API for Merging AES67 products (Doc avialalbe for OEM only)
- NMOS IS-04/05 support

#### Enhancements:

- Hostname was limited to 11 characters. New limitation is 32 characters.
- Compilation fix with Kernel version > 4.18
- [RAV-997] (https://jira.merging.com/browse/RAV-997) verify PTP packets checksum -> reject packet if wrong
- Advanced web page graphic improvement

#### Bug Fixes:
- [RAV-1251] (https://jira.merging.com/browse/RAV-1251) Two sinks using the same multicast address limitation
- [NAD-654] (https://jira.merging.com/browse/NAD-654) NADAC+PLAYER noise on the channel 3 output when playing out DSD stereo files
- [NAD-647] (https://jira.merging.com/browse/NAD-647) NADAC Volume jumps when adjusted from remote

#### Known issues:

- Compilation failure with Kernel version > 5.1 (tasklet_hrtimer_XXX functions have to be replace)