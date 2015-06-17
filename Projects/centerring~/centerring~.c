#include "fftease.h"

#define OBJECT_NAME "centerring~"
#define MAX_WARP 16.0

static t_class *centerring_class;

#define OBJECT_NAME "centerring~"

typedef struct _centerring
{

	t_pxobject x_obj;
	t_fftease *fft;

    int bufferLength;
    int recalc;
    int	seed;
    double baseFreq;
    double constFreq;
    double bandFreq;
    double frameR;
    double *ringPhases;
    double *ringIncrements;
    double *sineBuffer;
    double *bufferOne;
    double *channelOne;
	short connected[8];
	short mute;
	short bypass;
	long fftsize_attr;
	long overlap_attr;
} t_centerring;

/* msp function prototypes */

void *centerring_new(t_symbol *s, int argc, t_atom *argv);
t_int *centerring_perform(t_int *w);
void centerring_dsp(t_centerring *x, t_signal **sp, short *count);
void centerring_float(t_centerring *x, double myFloat);
void centerring_assist(t_centerring *x, void *b, long m, long a, char *s);
void centerring_dest(t_centerring *x, double f);
void centerring_messages(t_centerring *x, t_symbol *s, short argc, t_atom *argv);
void centerring_adjust( t_centerring *x );
void centerring_zerophases( t_centerring *x );
void centerring_randphases( t_centerring *x );
void centerring_free(t_centerring *x);
void centerring_init(t_centerring *x);
void centerring_mute(t_centerring *x, t_floatarg toggle);
//void centerring_overlap(t_centerring *x, t_floatarg o);
void centerring_fftinfo( t_centerring *x );
void centerring_winfac(t_centerring *x, t_floatarg f);
//void centerring_fftsize(t_centerring *x, t_floatarg f);
void centerring_bypass(t_centerring *x, t_floatarg toggle);
t_max_err set_fftsize(t_centerring *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_centerring *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_centerring *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_centerring *x, void *attr, long *ac, t_atom **av);
void centerring_dsp64(t_centerring *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void centerring_perform64(t_centerring *x, t_object *dsp64, double **ins,
                          long numins, double **outs,long numouts, long vectorsize,
                          long flags, void *userparam);

void centerring_float( t_centerring *x, t_floatarg df )
{
	float myFloat = (float) df;
	int inlet = x->x_obj.z_in;
	
	if ( inlet == 1 ) {
		x->baseFreq = myFloat;
		x->recalc = 1;
	}	
	if ( inlet == 2 ) {
		x->bandFreq = myFloat;
		x->recalc = 1;
	}
	if ( inlet == 3 ) {
		x->constFreq = myFloat;
		x->recalc = 1;
	}
}

int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.centerring~", (method)centerring_new, (method)centerring_free, 
				  sizeof(t_centerring),0,A_GIMME,0);
    class_addmethod(c,(method)centerring_dsp64, "dsp64", A_CANT, 0);
    class_addmethod(c,(method)centerring_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)centerring_messages,"seed", A_GIMME, 0);    
	class_addmethod(c,(method)centerring_messages,"zerophases", A_GIMME, 0);
	class_addmethod(c,(method)centerring_messages,"randphases", A_GIMME, 0);
	class_addmethod(c,(method)centerring_mute,"mute", A_FLOAT, 0);
	class_addmethod(c,(method)centerring_bypass,"bypass", A_FLOAT, 0);
	class_addmethod(c,(method)centerring_winfac,"winfac", A_FLOAT, 0);
	class_addmethod(c,(method)centerring_fftinfo,"fftinfo", 0);
	class_addmethod(c,(method)centerring_float,"float",0);
    
	CLASS_ATTR_LONG(c, "fftsize", 0, t_centerring, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_LONG(c, "overlap", 0, t_centerring, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	centerring_class = c;	
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

void centerring_messages(t_centerring *x, t_symbol *s, short argc, t_atom *argv)
{	
	
	if (s == gensym("seed"))
		x->seed = (int) atom_getfloatarg(0,argc,argv);
	
	if (s == gensym("zerophases")) 
		centerring_zerophases( x );
	
	if (s == gensym("randphases"))
		centerring_randphases( x );
}

/* diagnostic messages for Max */

void centerring_assist (t_centerring *x, void *b, long msg, long arg, char *dst)
{
	
	if (msg == 1) {
		
		switch (arg) {
			case 0:		sprintf(dst,"(signal) Input"); break;
			case 1:		sprintf(dst,"(signal/float) Base Modulation Frequency"); break;
			case 2:		sprintf(dst,"(signal/float) Frequency Deviation Bandwidth"); break;
			case 3:		sprintf(dst,"(signal/float) Frequency Deviation Constant"); break;		
		}
	}
	
	else {
		if (msg == 2)
			sprintf(dst,"(signal) Output");
	}
}


void *centerring_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;

	t_centerring *x = (t_centerring *)object_alloc(centerring_class);
	dsp_setup((t_pxobject *)x, 4);
	outlet_new((t_pxobject *)x, "signal");

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
 	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize(); 
	/* optional arguments: baseFreq, bandFreq, constFreq, seed, overlap, winfac */
	x->baseFreq = 1.0;
	x->bandFreq = 0.2;
	x->constFreq = 1.0;

	x->seed = 1977;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	
	attr_args_process(x, argc, argv);
	centerring_init(x);
	return x;
}

void centerring_init(t_centerring *x)
{
	t_fftease *fft = x->fft;
	int Nw;
	int N;
	int N2;
	short initialized= fft->initialized;
	x->x_obj.z_disabled = 1;		
	fftease_init(fft);
	Nw = fft->Nw;
	N = fft->N;
	N2 = fft->N2;
	if(! fftease_msp_sanity_check(fft,OBJECT_NAME)){
		post("failed sanity check!");
		return;
	}

	x->frameR = (float) fft->R / (float) fft->D;

	if(!initialized){
		x->mute = 0;
		x->bufferLength = 131072;
		x->recalc = 0;
		x->ringPhases = (double *) sysmem_newptrclear((N2 + 1) * sizeof(double));
		x->ringIncrements = (double *) sysmem_newptrclear((N2 + 1) * sizeof(double));
		x->sineBuffer = (double *) sysmem_newptrclear((x->bufferLength + 1)* sizeof(double));
		makeSineBuffer(x->sineBuffer, x->bufferLength);
	} else {
		x->ringIncrements = (double *)sysmem_resizeptrclear((void *)x->ringIncrements, (N2 + 1) * sizeof(double));
		x->ringPhases = (double *)sysmem_resizeptrclear((void *)x->ringPhases, (N2 + 1) * sizeof(double));
	}

	centerring_adjust(x);
	centerring_zerophases(x); 
	x->x_obj.z_disabled = 0;	
}

void centerring_free(t_centerring *x)
{
	dsp_free((t_pxobject *) x);
	// fftease_free(x->fft);
    sysmem_freeptr(x->fft);
	sysmem_freeptr(x->ringPhases);
	sysmem_freeptr(x->ringIncrements);
	sysmem_freeptr(x->sineBuffer);
}

void centerring_adjust( t_centerring *x ) {
	
	int		i;
	double	*ringIncrements = x->ringIncrements;
	int N2 = x->fft->N2;
	
	if(x->frameR == 0){
		post("centerring_adjust got at 0 SR!");
		return;
	}
    for (i=0; i < N2; i++) {

		*(ringIncrements+i) = 
		frequencyToIncrement( 
			x->frameR, 
			x->baseFreq * ((rrand(&(x->seed)) * x->bandFreq) + x->constFreq ), 
			x->bufferLength
		);
    }
}


void centerring_zerophases( t_centerring *x ) {
	
	int	i;
	
	for (i=0; i < x->fft->N2; i++)
		*((x->ringPhases)+i) = 0.;
}


void centerring_randphases( t_centerring *x ) {
	
	int	i;
	
	for (i=0; i < x->fft->N2; i++)
		*((x->ringPhases)+i) = prand(&(x->seed)) * (float) (x->bufferLength);
	
}

void do_centerring(t_centerring *x)
{
	t_fftease *fft = x->fft;
	double *buffer = fft->buffer;
	double *channel = fft->channel;
	int i, odd, even;
	double a1,b1;
	int N2 = fft->N2;
	int bufferLength = x->bufferLength;
	double *ringPhases = x->ringPhases;
	double *ringIncrements = x->ringIncrements;
	double *sineBuffer = x->sineBuffer;
	
	/* recalculate our oscillator values if object inputs have been updated */
	
	if (x->recalc)
		centerring_adjust( x );
	
	x->recalc = 0;
	
	fold(fft);
	rdft(fft,1);
	
	/* convert to polar coordinates from complex values */ 
	
	for ( i = 0; i <= N2; i++ ) {
		odd = ( even = i<<1 ) + 1;
		
		a1 = ( i == N2 ? *(buffer+1) : *(buffer + even) );
		b1 = ( i == 0 || i == N2 ? 0. : *(buffer + odd) );
		*(channel + even) = hypot( a1, b1 );
		*(channel + odd) = -atan2( b1, a1 );      
	}
	
	
	/* perform ring modulation on successive fft frames */
	
	for (i=0; i < N2; i++) {
		even = i<<1;
		
		*(channel + even) *= bufferOscil( ringPhases+i,
										  *(ringIncrements+i), sineBuffer, bufferLength );
	}
	
	/* convert from polar to cartesian */	
	
	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		*(buffer + even) = *(channel + even) * cos( *(channel + odd) );
		
		if ( i != N2 )
			*(buffer + odd) = (*(channel + even)) * -sin( *(channel + odd) );
	}
	rdft(fft,-1);
	overlapadd(fft);
}

void centerring_perform64(t_centerring *x, t_object *dsp64, double **ins,
                            long numins, double **outs,long numouts, long vectorsize,
                            long flags, void *userparam)
{

	int i, j;
	t_fftease *fft = x->fft;
	double *MSPInputVector = ins[0];
	double *vec_baseFreq = ins[1];
	double *vec_bandFreq = ins[2];
	double *vec_constFreq = ins[3];
	double *MSPOutputVector = outs[0];
	double *input = fft->input;
	int D = fft->D;
	int Nw = fft->Nw;
	double *output = fft->output;
	float mult = fft->mult ;
	int MSPVectorSize = fft->MSPVectorSize;
	double *internalInputVector = fft->internalInputVector;
	double *internalOutputVector = fft->internalOutputVector;		
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;	

	if(x->mute || x->x_obj.z_disabled){
		for(i=0; i < vectorsize; i++){ MSPOutputVector[i] = 0.0; }
        return;
	}
	if (x->bypass) {
		for( j = 0; j < MSPVectorSize; j++) {
			*MSPOutputVector++ = *MSPInputVector++ * FFTEASE_BYPASS_GAIN;
		}
		return;
	}
	
	short *connected = x->connected;
	
	if(connected[1]){
		x->recalc = 1;
		x->baseFreq = *vec_baseFreq;	
	}
	if(connected[2]){
		x->recalc = 1;
		x->bandFreq = *vec_bandFreq;	
	}
	if(connected[3]){
		x->recalc = 1;
		x->constFreq = *vec_constFreq;	
	}
    // do_centerring(x);
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_centerring(x);
        
		for ( j = 0; j < D; j++ ){
			*MSPOutputVector++ = output[j] * mult;
        }
        sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(MSPInputVector + (D*i), input + (Nw-D), D * sizeof(t_double));
            
			do_centerring(x);
			
			for ( j = 0; j < D; j++ ){
				*MSPOutputVector++ = output[j] * mult;
			}
            sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
	}
	else if( fft->bufferStatus == BIGGER_THAN_MSP_VECTOR ) {
        sysmem_copyptr(MSPInputVector,internalInputVector + (operationCount * MSPVectorSize),MSPVectorSize * sizeof(t_double));
        sysmem_copyptr(internalOutputVector + (operationCount * MSPVectorSize),MSPOutputVector,MSPVectorSize * sizeof(t_double));
		operationCount = (operationCount + 1) % operationRepeat;
		
		if( operationCount == 0 ) {
			
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(internalInputVector, input + (Nw - D), D * sizeof(t_double));
            
			do_centerring(x);
			
			for ( j = 0; j < D; j++ ){
				internalOutputVector[j] = output[j] * mult;
			}
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}		



void centerring_mute(t_centerring *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void centerring_bypass(t_centerring *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void centerring_overlap(t_centerring *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	centerring_init(x);
}

void centerring_winfac(t_centerring *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	centerring_init(x);
}

void centerring_fftsize(t_centerring *x, t_floatarg f)
{
	x->fft->N = (int) f;
	centerring_init(x);
}

void centerring_fftinfo( t_centerring *x )
{
	fftease_fftinfo( x->fft, OBJECT_NAME );	
}

t_max_err get_fftsize(t_centerring *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_centerring *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		centerring_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_centerring *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_centerring *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		centerring_init(x);
	}
	return MAX_ERR_NONE;
}
void centerring_dsp64(t_centerring *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    int i;
    t_fftease *fft = x->fft;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
	for( i = 0; i < 4; i++ ){
		x->connected[i] = count[i];
	}
	
	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
	}
    if(reset_required){
        centerring_init(x);
    }
    object_method(dsp64, gensym("dsp_add64"),x,centerring_perform64,0,NULL);
}


