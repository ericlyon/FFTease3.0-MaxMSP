#include "fftease.h"

static t_class *leaker_class;

#define OBJECT_NAME "leaker~"

typedef struct _leaker
{
	t_pxobject x_obj;
	t_fftease *fft;
	t_fftease *fft2;
	int *sieve;
	short mute;
	short bypass;
	short fade_connected;
	double fade_value;
	long fftsize_attr;
	long overlap_attr;
} t_leaker;

static void leaker_free(t_leaker *x);
void *leaker_new(t_symbol *msg, short argc, t_atom *argv);
void leaker_assist(t_leaker *x, void *b, long m, long a, char *s);
void leaker_upsieve(t_leaker *x) ;
void leaker_downsieve(t_leaker *x) ;
void leaker_randsieve(t_leaker *x) ;
void leaker_bypass(t_leaker *x, t_floatarg state);
void leaker_mute(t_leaker *x, t_floatarg state);
void leaker_float(t_leaker *x, double f);
void leaker_init(t_leaker *x);
void leaker_winfac(t_leaker *x, t_floatarg f);
void leaker_fftinfo(t_leaker *x);
t_max_err set_fftsize(t_leaker *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_leaker *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_leaker *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_leaker *x, void *attr, long *ac, t_atom **av);
void leaker_dsp64(t_leaker *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void leaker_perform64(t_leaker *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam);
int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.leaker~", (method)leaker_new, (method)leaker_free, sizeof(t_leaker), 0,A_GIMME, 0);
	class_addmethod(c,(method)leaker_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)leaker_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)leaker_upsieve, "upsieve", 0);
	class_addmethod(c,(method)leaker_downsieve, "downsieve", 0);
	class_addmethod(c,(method)leaker_randsieve, "randsieve", 0);
	class_addmethod(c,(method)leaker_bypass,"bypass",A_DEFFLOAT,0);
	class_addmethod(c,(method)leaker_mute,"mute",A_DEFFLOAT,0);
	class_addmethod(c,(method)leaker_winfac,"winfac",A_DEFFLOAT,0);
	class_addmethod(c,(method)leaker_fftinfo,"fftinfo",0);
	class_addmethod(c,(method)leaker_float,"float", A_FLOAT, 0);

	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_leaker, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_leaker, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	leaker_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

void leaker_fftsize(t_leaker *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	x->fft2->N = (int) f;
	leaker_init(x);
}

void leaker_overlap(t_leaker *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	x->fft2->overlap = (int) f;
	leaker_init(x);
}

void leaker_winfac(t_leaker *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	x->fft2->winfac = (int) f;
	leaker_init(x);
}

void leaker_fftinfo(t_leaker *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void leaker_free( t_leaker *x ){
	dsp_free((t_pxobject *)x);
	fftease_free(x->fft);
	fftease_free(x->fft2);
    sysmem_freeptr(x->fft);
    sysmem_freeptr(x->fft2);
	sysmem_freeptr(x->sieve);
}

void leaker_upsieve(t_leaker *x) {
	int i;
	int *sieve = x->sieve;
	for( i = 0; i < x->fft->N2; i++ ){
		sieve[i] = i + 1;
	}
}

void leaker_downsieve(t_leaker *x) {
	int i;
	int *sieve = x->sieve;
	int N2 = x->fft->N2;
	for( i = 0; i < N2; i++ ){
		sieve[i] = N2  - i;
	}
}

void leaker_randsieve(t_leaker *x) {
	int i;
//	int NSwitch = 100000 ;
	int temp;
	int pos1, pos2;
	int N2 = x->fft->N2;
	int *sieve = x->sieve;
    int maxswap = N2 - 1;
	
	for( i = 0; i < N2; i++ ){
		sieve[i] = i + 1;
	}
    while(maxswap > 0){
        pos1 = maxswap;
        pos2 = rand() % (N2 - 1);
        temp = sieve[pos1];
        sieve[pos1] = sieve[pos2];
        sieve[pos2] = temp;
        --maxswap;
    }
    /*
	for( i = 0; i < NSwitch; i++ ){
		pos1 = rand() % N2;
		pos2 = rand() % N2;
		temp = x->sieve[pos2];
		sieve[pos2] = sieve[pos1];
		sieve[pos1] = temp ;
	}
    */
}

void leaker_bypass(t_leaker *x, t_floatarg state)
{
	x->bypass = (short)state;	
}
void leaker_mute(t_leaker *x, t_floatarg state)
{
	x->mute = (short)state;	
}


void leaker_assist (t_leaker *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input 1");break;
			case 1: sprintf(dst,"(signal) Input 2");break;
			case 2: sprintf(dst,"(signal/float) Crossfade Position (0.0 - 1.0)");break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output ");
	}
}

void *leaker_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_fftease *fft, *fft2;

	t_leaker *x = (t_leaker *)object_alloc(leaker_class);
	dsp_setup((t_pxobject *)x,3);
	outlet_new((t_pxobject *)x, "signal");

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	x->fft2 = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft2 = x->fft2;
		
	fft->initialized = 0;
	fft2->initialized = 0;

	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	
	fft2->N = FFTEASE_DEFAULT_FFTSIZE;
	fft2->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft2->winfac = FFTEASE_DEFAULT_WINFAC;
	
	fft2->R = fft->R = sys_getsr();
	fft2->MSPVectorSize = fft->MSPVectorSize = sys_getblksize();
	if( fft->R <= 0 || fft->R > 1000000 || fft->MSPVectorSize <= 0 ){
		error("%s is concerned that perhaps no audio driver has been loaded",OBJECT_NAME);
		return 0;
	}
	attr_args_process(x, argc, argv);
	leaker_init(x);	
	return x;
}

void leaker_init(t_leaker *x)
{
	int i;
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	short initialized = fft->initialized;
	x->x_obj.z_disabled = 1;
	
	fftease_init(fft);
	fftease_init(fft2);

	if(!initialized) {
		x->mute = 0;
		x->bypass = 0;
		x->fade_connected = 0;
		x->fade_value = 0;
		x->sieve = (int *) sysmem_newptrclear((fft->N2 + 1) * sizeof(int));
	}  else {
		/*
		memset((char *)fft->input,0,fft->Nw);
		memset((char *)fft2->input,0,fft->Nw);
		memset((char *)fft->output,0,fft->Nw);
		memset((char *)fft->c_lastphase_in,0,(fft->N2+1) * sizeof(float));
		memset((char *)fft2->c_lastphase_in,0,(fft->N2+1) * sizeof(float));
		memset((char *)fft->c_lastphase_out,0,(fft->N2+1) * sizeof(float));
		*/
	}
    
	if(initialized != 2){
		for(i = 0; i < fft->N2; i++){
			x->sieve[i] = i;
		}
	}

	x->x_obj.z_disabled = 0;
	return;	
}

void do_leaker(t_leaker *x)
{
	int i,odd,even;
	double a1,a2,b1,b2;
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int N2 = fft->N2;
	double *buffer1 = fft->buffer;
	double *buffer2 = fft2->buffer;
	double *channel1 = fft->channel;
	int *sieve = x->sieve;
	double fade_value = x->fade_value;

	fold(fft);		
	fold(fft2);	
	rdft(fft,1);
	rdft(fft2,1);
	
	
	for ( i = 0; i <= N2; i++ ) {
		odd = ( even = i<<1 ) + 1;
		if( fade_value <= 0 || fade_value < sieve[i]  ){
			a1 = ( i == N2 ? *(buffer1+1) : *(buffer1+even) );
			b1 = ( i == 0 || i == N2 ? 0. : *(buffer1+odd) );
			
			*(channel1+even) = hypot( a1, b1 ) ;
			*(channel1+odd) = -atan2( b1, a1 );
			*(buffer1+even) = *(channel1+even) * cos(*(channel1+odd));
			if ( i != N2 ){
				*(buffer1+odd) = -(*(channel1+even)) * sin(*(channel1+odd));
			}
		} else {
			a2 = ( i == N2 ? *(buffer2+1) : *(buffer2+even) );
			b2 = ( i == 0 || i == N2 ? 0. : *(buffer2+odd) );
			*(channel1+even) = hypot( a2, b2 ) ;
			*(channel1+odd) = -atan2( b2, a2 );
			*(buffer1+even) = *(channel1+even) * cos(*(channel1+odd) );
			if ( i != N2 ){
				*(buffer1+odd) = -(*(channel1+even)) * sin( *(channel1+odd) );
			}
		}
	}
	rdft(fft,-1);
	overlapadd(fft);
}

void leaker_perform64(t_leaker *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam)
{
	int i,j;
	double *MSPInputVector1 = ins[0];
	double *MSPInputVector2 = ins[1];
	double *fade_value = ins[2];
	double *MSPOutputVector = outs[0];
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int MSPVectorSize = fft->MSPVectorSize;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	double *internalInputVector1 = fft->internalInputVector;
	double *internalInputVector2 = fft2->internalInputVector;
	double *internalOutputVector = fft->internalOutputVector;
	double *inputOne = fft->input;
	double *inputTwo = fft2->input;
	double *output = fft->output;
	int D = fft->D;
	int Nw = fft->Nw;
	double mult = fft->mult;	
	int N2 = fft->N2;
	

	if(x->fade_connected){
		x->fade_value = *fade_value * (float) N2;
	}

	if(x->mute || x->x_obj.z_disabled){
		for(i=0; i < vectorsize; i++){ MSPOutputVector[i] = 0.0; }
		return;
	}
	if (x->bypass) {
		for( j = 0; j < MSPVectorSize; j++) {
			*MSPOutputVector++ = *MSPInputVector1++ * FFTEASE_BYPASS_GAIN;
		}
		return;
	}
	//do_leaker(x);
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(inputOne + D, inputOne, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector1, inputOne + (Nw - D), D * sizeof(t_double));
        sysmem_copyptr(inputTwo + D, inputTwo, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector2, inputTwo + (Nw - D), D * sizeof(t_double));
		do_leaker(x);
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            sysmem_copyptr(inputOne + D, inputOne, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(MSPInputVector1 + (D*i), inputOne + (Nw-D), D * sizeof(t_double));
            sysmem_copyptr(inputTwo + D, inputTwo, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(MSPInputVector2 + (D*i), inputTwo + (Nw-D), D * sizeof(t_double));
			do_leaker(x);
			for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
	}
	else if( fft->bufferStatus == BIGGER_THAN_MSP_VECTOR ) {
        sysmem_copyptr(MSPInputVector1,internalInputVector1 + (operationCount * MSPVectorSize),MSPVectorSize * sizeof(t_double));
        sysmem_copyptr(MSPInputVector2,internalInputVector2 + (operationCount * MSPVectorSize),MSPVectorSize * sizeof(t_double));
        sysmem_copyptr(internalOutputVector + (operationCount * MSPVectorSize),MSPOutputVector,MSPVectorSize * sizeof(t_double));
        
		operationCount = (operationCount + 1) % operationRepeat;
		if( operationCount == 0 ) {
            sysmem_copyptr(inputOne + D, inputOne, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(internalInputVector1, inputOne + (Nw - D), D * sizeof(t_double));
            sysmem_copyptr(inputTwo + D, inputTwo, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(internalInputVector2, inputTwo + (Nw - D), D * sizeof(t_double));
			do_leaker(x);
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}		

void leaker_dsp64(t_leaker *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
    x->fade_connected = count[2];
	
	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
        fft2->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
        fftease_set_fft_buffers(fft2);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
        fft2->R = samplerate;
	}
    if(reset_required){
        leaker_init(x);
    }
    object_method(dsp64, gensym("dsp_add64"),x,leaker_perform64,0,NULL);
}

t_max_err get_fftsize(t_leaker *x, void *attr, long *ac, t_atom **av)
{
	if (ac && av) {
		char alloc;
		
		if (atom_alloc(ac, av, &alloc)) {
			return MAX_ERR_GENERIC;
		}
		x->fftsize_attr = x->fft->N;
		atom_setlong(*av, x->fftsize_attr);
	}	
	return MAX_ERR_NONE;
	
}

t_max_err set_fftsize(t_leaker *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		x->fft2->N = (int) val;
		leaker_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_leaker *x, void *attr, long *ac, t_atom **av)
{
	if (ac && av) {
		char alloc;
		
		if (atom_alloc(ac, av, &alloc)) {
			return MAX_ERR_GENERIC;
		}
		x->overlap_attr = x->fft->overlap;
		atom_setlong(*av, x->overlap_attr);
	}	
	return MAX_ERR_NONE;
} 


t_max_err set_overlap(t_leaker *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		x->fft2->overlap = (int) val;
		leaker_init(x);
	}
	return MAX_ERR_NONE;
}


void leaker_float(t_leaker *x, double f)
{
	int inlet = x->x_obj.z_in;
	if( inlet == 2 && f >= 0 && f <= 1){
		x->fade_value = f * (float) x->fft->N2;
	}
}

