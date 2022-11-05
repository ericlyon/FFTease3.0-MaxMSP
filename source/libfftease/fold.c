#include "fftease.h"

/*
 * multiply current input I by window W (both of length Nw);
 * using modulus arithmetic, fold and rotate windowed input
 * into output array O of (FFT) length N according to current
 * input time n
 */

void fold( t_fftease *fft )
{
	int Nw = fft->Nw;
	int N = fft->N;
	double *Wanal = fft->Wanal;
	double *input = fft->input;
	double *buffer = fft->buffer;
	int in_count = fft->in_count;	
    int i;
	
	memset(buffer, 0.0, N * sizeof(double));
	
    while ( in_count < 0 )
      	in_count += N;
    in_count %= N;
    for ( i = 0; i < Nw; i++ ) {
		buffer[in_count] += input[i] * Wanal[i];
      	if ( ++in_count == N )
			in_count = 0;
    }
	fft->in_count = (fft->in_count + fft->D) % fft->Nw;
}

