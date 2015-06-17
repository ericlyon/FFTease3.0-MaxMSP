#include "fftease.h"

void leanconvert(t_fftease *fft)

{
	
	int		real, imag, amp, phase;
	double		a, b;
	int		i;
	double *buffer = fft->buffer;
	double *channel = fft->channel;
	int N2 = fft->N2;	
	
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		a = ( i == N2 ? buffer[1] : buffer[real] );
		b = ( i == 0 || i == N2 ? 0. : buffer[imag] );
		channel[amp] = hypot( a, b );
		channel[phase] = -atan2( b, a );
	}
}

