#include "fftease.h"
// extern void post(const char *fmt, ...);
#define PARANOID 0

void limited_oscbank( t_fftease *fft, int osclimit, double framethresh)
{
	int    amp,freq,chan, n;
	
	double    a,ainc,f,finc,address;
	int D = fft->D;
	int I = D;
	int L = fft->L;
	double synt = fft->synt;
	double P  = fft->P; 
	int R = fft->R;
	int N2 = fft->N2;
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
	int oscnt = 0;
#ifdef PARANOID	
	if(! fft->init_status ){ // means memory alloc in effect
		goto exit;
	}
#endif
	
	if(R == 0){
		post("FFTeaseLib: limited oscbank got 0 SR");
		return;
	}

	if(lo_bin < 0 || hi_bin > N2){
		post("FFTeaseLib: limited oscbank: bad bins: %d %d",lo_bin,hi_bin);
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
	if(maxamp > framethresh){
		localthresh = synt * maxamp;
	} else {
		localthresh = synt * framethresh; // watch this line!
	}
	
	
    for ( chan = lo_bin; chan < hi_bin; chan++ ) {
#ifdef PARANOID	
		if(! fft->init_status ){ // means memory alloc in effect
			goto exit;
		}
#endif
		freq = ( amp = ( chan << 1 ) ) + 1;
		if(noalias){
			if( channel[freq] * P >= nyquist )
				channel[amp] = 0;
		}
		if ( channel[amp] > localthresh ){ 
			++oscnt;
#ifdef PARANOID	
			if(! fft->init_status ){ // means memory alloc in effect
				goto exit;
			}
#endif
			if(oscnt > osclimit){
				goto exit;
			}
			channel[freq] *= pitch_increment;
			finc = ( channel[freq] - ( f = lastfreq[chan] ) )*Iinv;
			ainc = ( channel[amp] - ( a = lastamp[chan] ) )*Iinv;
			address = bindex[chan];
			// this was the bug - somewhere bindex was not properly initialized!
			//i_address = (int) address;
			if( address < 0 || address >= L){
				address = 0;
				// post("limited oscbank: bad address");
			}			
			for ( n = 0; n < I; n++ ) {
#ifdef PARANOID	
				if(! fft->init_status ){ // means memory alloc in effect
					goto exit;
				}
#endif
// this is a suspected bug line:
					/*
				iAddress = (int) address;
				if( iAddress == L ){
					iAddress = L - 1;
				}
				
			
				if( iAddress < 0 || iAddress >= L ){
					post("limited oscbank: bad address: %d", iAddress);
				} else {
					output[n] += a*table[ iAddress ];
				}
				*/
				// skip excessive paranoia for efficiency
				output[n] += a*table[ (int) address ]; // this WILL go to L, so tab needs a guardpoint
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