# Welcome to FFTease!

**About FFTease**

FFTease is a collection of objects designed to facilitate spectral sound
processing in Max and Pd. The collection was designed by me and
<A HREF="https://soundcloud.com/christopher-penrose">Christopher Penrose</A> in 1999, and has been maintained by me since
2003. The first paper on FFTease, presented at the 2000 International Computer Music Conference
may be viewed <A HREF="https://quod.lib.umich.edu/cgi/p/pod/dod-idx/fftease-a-collection-of-spectral-signal-processors-for.pdf?c=icmc;idno=bbp2372.2000.207;format=pdf">here</A>. 

**List of Objects**

- bthresher~ similar to thresher~ but with more control
- burrow~ a cross-referenced filtering object
- cavoc~ an 8-rule cellular automata that generates spectra
- cavoc27~ a 27-rule cellular automata object
- centerring~ a spectral modulation object
- codepend~ a classic block convolution object
- cross~ a cross synthesis object with gating
- dentist~ a partial knockout object
- disarrain~ an interpolating version of disarray~
- disarray~ a spectral redistribution object
- drown~ a noise reduction (or increase) object
- enrich~ an oscillator resynthesis with a user-designed waveform
- ether~ another spectral compositing object
- leaker~ a sieve-based cross fader
- loopsea~ spectrally stratified independent looping
- mindwarp~ a spectral formant warping object
- morphine~ a morphing object
- multyq~ a four band filter
- pileup~ a spectral accumulation object
- pvcompand~ a spectral compressor/expander object
- pvgrain~ a spectrum analyzer for granular resynthesis
- pvharm~ a harmonizer
- pvoc~ an additive synthesis phase vocoder
- pvtuner~ a spectrum quantizer for tuning to arbitrary scales
- pvwarp~ a non-linear frequency warper
- pvwarpb~ a non-linear frequency warper with a user-accessible warp array
- reanimator~ an audio texture mapper
- resent~ similar to residency~ but with independent bin control
- residency~ a spectral sampler useful for time scaling
- residency_buffer~ a spectral sampler that writes analysis data to a Pd array
- schmear~ a spectral smear object
- scrape~ a noise reduction (or increase) object with frequency control
- shapee~ a frequency shaping object
- swinger~ a phase swapping object
- taint~ a cross synthesis object
- thresher~ an amplitude/frequency sensitive gating object
- vacancy~ a spectral compositing object
- xsyn~ a cross synthesis with compression object

**Accessibility**

As of October 31, 2023, FFTease is available for Max 8.0 and later as a Package, which can by installed
from the Max Package Manager.

**Compilation** 


Instructions for compiling FFTease for Max are provided in the <A HREF = "https://github.com/ericlyon/FFTease3.0-MaxMSP/blob/master/BUILDME.md">BUILDME</A> document included here.


**Performance Considerations**


The default Pd audio buffer settings for both I/O vector size and signal
vector size will work fine for FFT sizes up to around 4096 or so. For
larger FFT sizes, adjusting the signal vector size and I/O vector
size upward can dramatically improve performance. With larger FFT sizes,
the reported CPU load may fluctuate. This is because a large FFT is
being performed only once for several vectors worth of samples. The
default FFT size is 1024, and the default overlap factor is 8.


**Acknowledgements**


This repository was rebuilt for use with the Max 8.2 SDK by @isabelgk.
Testing and feedback for the Max Package of FFTease was provided by
Gregory Taylor and Alex Van Gils. The FFTease logo for the Max Package
was created by Hiromi Okumura.

Happy spectral surfing!

Eric Lyon  
ericlyon@vt.edu  
Blacksburg, Virginia  
October 31, 2023
