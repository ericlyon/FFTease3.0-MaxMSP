/*
 * input I is a folded spectrum of length N; output O and
 * synthesis window W are of length Nw--overlap-add windowed,
 * unrotated, unfolded input data into output O
 */

#include "fftease.h"

void overlapadd(t_fftease *fft)

{
	double *buffer = fft->buffer;
	int N = fft->N;
	double *Wsyn = fft->Wsyn;
	double *output = fft->output;
	int Nw = fft->Nw;
	int out_count = fft->out_count;
	
	int i ;
    while ( out_count < 0 )
		out_count += N ;
    out_count %= N ;
    for ( i = 0 ; i < Nw ; i++ ) {
		output[i] += buffer[out_count] * Wsyn[i];
		if ( ++out_count == N )
			out_count = 0 ;
    }
	fft->out_count = (fft->out_count + fft->D) % fft->Nw;
}
