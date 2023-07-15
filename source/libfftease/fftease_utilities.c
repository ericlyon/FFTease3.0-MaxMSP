#include "fftease.h"
#define FFTEASE_LIB_VERSION "FFTease library 3.002"

void fftease_noalias(t_fftease* fft, short flag)
{
	fft->noalias = flag;
}

int fftease_fft_size( int testfft )
{
int test = 2;
	if( testfft <= 0 )
		return DEFAULT_FFTEASE_FFTSIZE;
	while( test < testfft && test < FFTEASE_MAX_FFTSIZE){
		test *= 2;
	}
	if( test != testfft ){
		post("incorrect FFT size specified, using %d", DEFAULT_FFTEASE_FFTSIZE);
		test = DEFAULT_FFTEASE_FFTSIZE;
	}
	if( test == FFTEASE_MAX_FFTSIZE){
		post("fftsize capped at maximum: %d", test);
	}
	return test;
}

void fftease_set_fft_buffers(t_fftease *fft)
{
	if( fft->D <= 0 || fft->MSPVectorSize <= 0 ){
		return;
	}
    // post("setting up FFTease buffers");
	fft->operationCount = 0;
	if( fft->D > fft->MSPVectorSize ){
		fft->operationRepeat = fft->D / fft->MSPVectorSize;
		fft->bufferStatus = BIGGER_THAN_MSP_VECTOR;
		// post("fftease_set_fft_buffers: bigger than MSP vector");
	} else if( fft->D < fft->MSPVectorSize ){
		fft->operationRepeat = fft->MSPVectorSize / fft->D;
		fft->bufferStatus = SMALLER_THAN_MSP_VECTOR;
		// post("fftease_set_fft_buffers: smaller than MSP buffer");
	} else {
		fft->operationRepeat = 1;
		fft->bufferStatus = EQUAL_TO_MSP_VECTOR;
		// post("fftease_set_fft_buffers: equal to MSP buffer");
	}
}

int fftease_overlap( int overlap )
{
	int target = 2;
	while( target < overlap && target < 64 ){
		target *= 2;
	}
	if( target != overlap ){
		error("fftease_overlap: %d is not a legal overlap factor",overlap);
		return -1;
	}
	return overlap;
}

int fftease_winfac( int winfac)
{
	int target = 1;
	while( target < winfac && target < 64 ){
		target *= 2;
	}
	if( target != winfac ){
		// error("%d is not a legal window factor", winfac);
		return 1;
	}
	return winfac;
}

void fftease_oscbank_setbins(t_fftease *fft, double lowfreq, double highfreq)
{
	if(fft->initialized == -1){
		post("oscbank setbins inhibited");
		return;
	}
	double curfreq = 0;
	fft->hi_bin = 1;
	int N2 = fft->N2;
	while(curfreq < highfreq){
		++(fft->hi_bin);
		curfreq += fft->c_fundamental;
	}
	fft->lo_bin = 0;
	curfreq = 0;
	while(curfreq < lowfreq){
		++(fft->lo_bin);
		curfreq += fft->c_fundamental;
	}
	if(fft->hi_bin > N2)
		fft->hi_bin = N2;
//	post("lowfreq %f highfreq %f low bin %d high bin %d",lowfreq, highfreq, fft->lo_bin, fft->hi_bin);
}

void fftease_init(t_fftease *fft)
{
	int i;
	int mem;
	if(fft->initialized == -1){
		// post("fftease_init manually aborted with initialization status -1");
		return;
	}
	if(!fft->R){
		// post("fftease_init: zero sample rate, aborting init");
        return;
    }
	// fft->init_status = 0;
    fft->overlap = fftease_overlap(fft->overlap);
    fft->winfac = fftease_winfac(fft->winfac);
	/*
	if( fft->R > 1000000 || fft->MSPVectorSize > 100000 || fft->MSPVectorSize <= 0 ){
		post("fftease init: bad data");
		return;
	}
	*/

	if(fft->P <= 0)
		fft->P = 1.0;		
	fft->N = fftease_fft_size( fft->N );
	fft->D = fft->N / fft->overlap;
	fft->Nw = fft->N * fft->winfac;
		
	fft->Iinv = 1.0/fft->D;
	fft->N2 = fft->N / 2;
	fft->Nw2 = fft->Nw / 2;
	
	fft->in_count = -(fft->Nw);
	fft->out_count = fft->in_count;
	fft->mult = 1.0 / (double) fft->N;
	fft->c_fundamental =  (double) fft->R/(double) fft->N;
	fft->c_factor_in =  (double) fft->R/((double)fft->D * TWOPI);
	fft->c_factor_out = TWOPI * (double)  fft->D / (double) fft->R;
	// fft->synt = 0.001;
	fft->L = FFTEASE_OSCBANK_TABLESIZE;
	fft->pitch_increment = fft->P * fft->L / fft->R;
	fft->ffac = fft->P * PI / fft->N;
	fft->nyquist = (double) fft->R / 2.0;
	
	if(! fft->initialized){
		// post("Initializing FFT Memory");
		fft->P = 1.0;
		fft->obank_flag = 0; // default no oscbank
		fft->lo_bin = 0; 
		fft->hi_bin = fft->N2; 
		mem = (fft->Nw) * sizeof(double);
		fft->Wanal = (double *) sysmem_newptrclear(mem);
		fft->Wsyn = (double *) sysmem_newptrclear(mem);	
		fft->Hwin = (double *) sysmem_newptrclear(mem);	
		fft->input = (double *) sysmem_newptrclear(mem);		
		fft->output = (double *) sysmem_newptrclear(mem);	
		mem = (fft->N + 2) * sizeof(double);
		fft->buffer = (double *) sysmem_newptrclear(mem);	
		mem = (fft->N + 2) * sizeof(double);
		fft->channel = (double *) sysmem_newptrclear(mem);	
		mem = (fft->N * 2) * sizeof(int);
		fft->bitshuffle = (int *) sysmem_newptrclear(mem);	
		mem = (fft->N*2)*sizeof(double);
		fft->trigland = (double *) sysmem_newptrclear(mem);	
		mem = (fft->N2+1)*sizeof(double);
		fft->c_lastphase_in = (double *) sysmem_newptrclear(mem);	
		fft->c_lastphase_out = (double *)sysmem_newptrclear(mem);	
		// oscbank stuff
		mem = (fft->N+1)*sizeof(double);
		fft->lastamp = (double *) sysmem_newptrclear(mem);	
		fft->lastfreq = (double *) sysmem_newptrclear(mem);	
		fft->bindex = (double *) sysmem_newptrclear(mem);	
		mem = (2 + fft->L)*sizeof(double); // includes guardpoint
		fft->table = (double *) sysmem_newptrclear(mem);	
		// double buffering
		mem = fft->D * sizeof(double);
		fft->internalInputVector = (double *) sysmem_newptrclear(mem);
		fft->internalOutputVector = (double *) sysmem_newptrclear(mem);
		fft->initialized = 1;
	}
    else if( (fft->N == fft->last_N) && (fft->overlap == fft->last_overlap) &&
            (fft->winfac == fft->last_winfac) && (fft->last_R == fft->R) ) {
        //post("fftease_init: no change in vital parameters so memory reallocation skipped");
        return;
    }
    else {
		// post("Resizing FFT Memory");
		mem = (fft->Nw)*sizeof(double);
		fft->Wanal = (double *) sysmem_resizeptrclear((void *) fft->Wanal, mem);
		fft->Wsyn = (double *) sysmem_resizeptrclear((void *) fft->Wsyn, mem);
		fft->Hwin = (double *)sysmem_resizeptrclear((void *) fft->Hwin, mem);
		fft->input = (double *) sysmem_resizeptrclear((void *) fft->input, mem);
		fft->output = (double *) sysmem_resizeptrclear((void *) fft->output, mem);	
		mem = (fft->N + 2)*sizeof(double);
		fft->buffer = (double *) sysmem_resizeptrclear((void *) fft->buffer, mem);
		mem = (fft->N+2)*sizeof(double);
		fft->channel = (double *) sysmem_resizeptrclear((void *) fft->channel, mem);
		mem = (fft->N*2)*sizeof(int);
		fft->bitshuffle = (int *) sysmem_resizeptrclear((void *) fft->bitshuffle, mem);
		mem = (fft->N*2)*sizeof(double);
		fft->trigland = (double *) sysmem_resizeptrclear((void *) fft->trigland, mem);
		mem = (fft->N2+1)*sizeof(double);
		fft->c_lastphase_in = (double *) sysmem_resizeptrclear((void *) fft->c_lastphase_in, mem);
		fft->c_lastphase_out = (double *)sysmem_resizeptrclear((void *)fft->c_lastphase_out, mem);
		mem = (fft->N+1)*sizeof(double);
		fft->lastamp = (double *) sysmem_resizeptrclear((void *) fft->lastamp, mem);
		fft->lastfreq = (double *) sysmem_resizeptrclear((void *) fft->lastfreq, mem);
		fft->bindex = (double *) sysmem_resizeptrclear((void *) fft->bindex, mem);	
		mem = fft->D * sizeof(double);
		fft->internalInputVector = (double *)sysmem_resizeptrclear((void *) fft->internalInputVector, mem);
		fft->internalOutputVector = (double *) sysmem_resizeptrclear((void *) fft->internalOutputVector, mem);		
	}
    fft->last_N = fft->N;
    fft->last_overlap = fft->overlap;
    fft->last_winfac = fft->winfac;
    fft->last_R = fft->R;
	for ( i = 0; i < fft->L; i++ ) {
		fft->table[i] = (double) fft->N * cos((double)i * TWOPI / (double)fft->L);
	}
	fft->table[fft->L] = fft->table[fft->L - 1]; // guard point
	makewindows( fft->Hwin, fft->Wanal, fft->Wsyn, fft->Nw, fft->N, fft->D);
	init_rdft( fft->N, fft->bitshuffle, fft->trigland);
	fftease_set_fft_buffers(fft);
	fftease_oscbank_setbins(fft,0,fft->nyquist);
	fft->init_status = 1;
}

void fftease_free(t_fftease *fft)
{
	if(fft->init_status == 1){
        
		sysmem_freeptr(fft->trigland);
		sysmem_freeptr(fft->bitshuffle);
		sysmem_freeptr(fft->Wanal);
		sysmem_freeptr(fft->Wsyn);
		sysmem_freeptr(fft->Hwin);
		sysmem_freeptr(fft->buffer);
        
		sysmem_freeptr(fft->channel); // this is the killer
        
		sysmem_freeptr(fft->input);
		sysmem_freeptr(fft->output);
		sysmem_freeptr(fft->internalInputVector);
		sysmem_freeptr(fft->internalOutputVector);
		sysmem_freeptr(fft->c_lastphase_in);
		sysmem_freeptr(fft->c_lastphase_out);
		sysmem_freeptr(fft->lastamp);
		sysmem_freeptr(fft->lastfreq);
		sysmem_freeptr(fft->bindex);
		sysmem_freeptr(fft->table);
        
	}
}

void fftease_fftinfo(t_fftease *fft, char *object_name)
{
	if( ! fft->overlap ){
		post("%s: zero overlap!", object_name);
		return;
	}
	post("%s: FFT size %d, hopsize %d, windowsize %d, MSP Vector Size %d", object_name, 
	fft->N, fft->N/fft->overlap, fft->Nw, fft->MSPVectorSize);
	post("%s\n", FFTEASE_LIB_VERSION);
}

int fftease_msp_sanity_check(t_fftease *fft, char *oname)
{
	if( fft->R <= 0 || fft->R > 10000000 || fft->MSPVectorSize <= 0 || fft->D <= 0){
		post("%s is concerned that perhaps no audio driver has been loaded",oname);
		post("R: %d, vector size: %d, D: %d", fft->R, fft->MSPVectorSize, fft->D);
		return 0;
	} else {
		return 1;
	}
}

double fftease_randf(double min, double max)
{
	double randv;
	
	randv = (double) (rand() % 32768) / 32768.0 ;
	return min + (max-min) * randv;
}

int fftease_randi(int min, int max)
{
    // random values inclusive of min and max
    return min + (rand() % (1 + (max - min)));
}
