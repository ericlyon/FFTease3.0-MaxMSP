#include "fftease.h"

void oscbank( t_fftease *fft )
{
	int    amp,freq,chan, n;
	
	double    a,ainc,f,finc,address;
	int D = fft->D;
	int I = D;
	int L = fft->L;
	double synt = fft->synt;
	double P  = fft->P;
	int R = fft->R;
	double Iinv = 1./fft->D;
	double pitch_increment = fft->pitch_increment;
	double *table = fft->table;
	double *lastamp = fft->lastamp ;
	double *lastfreq = fft->lastfreq ;
	double *bindex = fft->bindex;
	double *channel = fft->channel;
	double *output = fft->output;
	int hi_bin = fft->hi_bin;
	int lo_bin = fft->lo_bin;
	double maxamp = 0.0;
    double localthresh, testamp;
	short noalias = fft->noalias;
	double nyquist = fft->nyquist;
	if(! fft->init_status ){ // means memory alloc in effect
		goto exit;
	}
	if(R == 0){
		post("oscbank got 0 SR");
		return;
	}
	pitch_increment = P * (double) L / (double) R;
	
	if( synt > 0.0 ){
		maxamp = 0.0;
		for ( chan = lo_bin; chan < hi_bin; chan++ ){
			amp = chan << 1;
			testamp = fabs( channel[amp] );
			if( testamp > maxamp )
				maxamp = testamp;
		}
	}
	localthresh = synt * maxamp;
	
    for ( chan = lo_bin; chan < hi_bin; chan++ ) {
		if(! fft->init_status ){ // means memory alloc in effect
			goto exit;
		}
		freq = ( amp = ( chan << 1 ) ) + 1;
		if(noalias){
			if( channel[freq] * P >= nyquist )
				channel[amp] = 0;
		}
		if ( channel[amp] > localthresh ){ 
			channel[freq] *= pitch_increment;
			finc = ( channel[freq] - ( f = lastfreq[chan] ) )*Iinv;
			ainc = ( channel[amp] - ( a = lastamp[chan] ) )*Iinv;
			address = bindex[chan];
			if( address < 0 || address >= L){
				address = 0.0;
				// post("limited oscbank: bad address");
			}
			for ( n = 0; n < I; n++ ) {
				// taking this out now:
				
				if(! fft->init_status ){ // means memory alloc in effect
					goto exit;
				}
				
				output[n] += a*table[ (int) address ];
				address += f;
				while ( address >= L )
					address -= L;
				while ( address < 0 )
					address += L;
				a += ainc;
				f += finc;
			}
			lastfreq[chan] = channel[freq];
			lastamp[chan] = channel[amp];
			bindex[chan] = address;
		}
    }
exit:;
}