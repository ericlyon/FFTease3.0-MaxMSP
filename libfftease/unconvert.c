#include "fftease.h"



//void unconvert( double *C, double *S, int N2, double *lastphase, double fundamental, double factor )
//	unconvert( channel, buffer, N2, c_lastphase_out, c_fundamental, c_factor_out  );

void unconvert(t_fftease *fft)

{
	double *channel = fft->channel;
	double *buffer = fft->buffer;
	int N2 = fft->N2;
	double *lastphase = fft->c_lastphase_out;
	double fundamental = fft->c_fundamental;
	double factor = fft->c_factor_out;
	
	int 	i, real, imag, amp, freq;
	double 	mag, phase;

	
    for ( i = 0; i <= N2; i++ ) {
		
		imag = freq = ( real = amp = i<<1 ) + 1;
		
		if ( i == N2 )
			real = 1;
		
		mag = channel[amp];
		lastphase[i] += channel[freq] - i*fundamental;
		phase = lastphase[i]*factor;
		buffer[real] = mag*cos( phase );
		
		if ( i != N2 )
			buffer[imag] = -mag*sin( phase );
		
    }
	
}
