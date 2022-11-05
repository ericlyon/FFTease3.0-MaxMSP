#include "fftease.h"


void leanunconvert(t_fftease *fft)

{
	int		real, imag, amp, phase;
	register int		i;
	double *buffer = fft->buffer;
	double *channel = fft->channel;
	int N2 = fft->N2;	
	
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		buffer[real] = *(channel+amp) * cos( *(channel+phase) );
		if ( i != N2 )
			buffer[imag] = -*(channel+amp) * sin( *(channel+phase) );
	}
}
