#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
/*
 #include "ext.h"
#include "z_dsp.h"
#include "buffer.h"
#include "ext_obex.h"
*/
#define t_floatarg double

/* choose your poison */

// lame MSP buffer limitation, will hopefully increase soon
#define MSP_BUFFER_MAX_OUTLETS (4) 





/* because Max and Pd have different ideas of what A_FLOAT is, use t_floatarg
to force consistency. Otherwise functions that look good will fail on some
hardware. Also note that Pd messages cannot accept arguments of type A_LONG. */

/* for compiling under XP */

#ifndef PIOVERTWO
#define PIOVERTWO 1.5707963268
#endif
#ifndef TWOPI
#define TWOPI 6.2831853072
#endif


