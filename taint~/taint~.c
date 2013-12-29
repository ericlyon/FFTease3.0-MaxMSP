#include "fftease.h"

static t_class *taint_class;

#define OBJECT_NAME "taint~"


/*
 Adding -32dB pad for invert option. Also added latency mechanism in
 switching from normal to "invert" to avoid glitches from extreme
 amplitude disparities.
 
 Made all inlets of type signal (with float options).
 
 Threshold input is now linear, not dB (with Max doing the conversion
 if desired).
 
 -EL 10/1/2005
 
 */

typedef struct _taint
{
	t_pxobject x_obj;
	t_fftease *fft;
	t_fftease *fft2;
	double mult;
	short mute;
	short bypass;
	int invert;
	short connected[4];
	int invert_countdown; // delay onset of invert effect to avoid loud glitches
	int invert_nextstate; // next state for invert   
	double invert_pad;   
	double threshold;
	double exponent;
	long fftsize_attr;
	long overlap_attr;
} t_taint;


/* msp function prototypes */

void *taint_new(t_symbol *s, int argc, t_atom *argv);
void taint_dsp(t_taint *x, t_signal **sp, short *count);
void taint_assist(t_taint *x, void *b, long m, long a, char *s);
void taint_float(t_taint *x, double f);
void taint_invert(t_taint *x, t_floatarg toggle);
void taint_free(t_taint *x);
void taint_mute(t_taint *x, t_floatarg toggle);
void taint_bypass(t_taint *x, t_floatarg toggle);
void taint_fftinfo(t_taint *x);
void taint_tilde_setup(void);
// void taint_overlap(t_taint *x, t_floatarg o);
void taint_winfac(t_taint *x, t_floatarg o);
void taint_init(t_taint *x);
void taint_pad(t_taint *x, t_floatarg pad);
// void taint_fftsize(t_taint *x, t_floatarg f);
t_max_err set_fftsize(t_taint *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_taint *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_taint *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_taint *x, void *attr, long *ac, t_atom **av);
void taint_dsp64(t_taint *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void taint_perform64(t_taint *x, t_object *dsp64, double **ins,
                    long numins, double **outs,long numouts, long vectorsize,
                    long flags, void *userparam);

int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.taint~", (method)taint_new, (method)taint_free, sizeof(t_taint),0,A_GIMME,0);
	
	class_addmethod(c,(method)taint_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)taint_assist,"assist",A_CANT,0);    
	class_addmethod(c,(method)taint_invert,"invert", A_FLOAT, 0);  	
//	class_addmethod(c,(method)taint_overlap,"overlap", A_FLOAT, 0);
	class_addmethod(c,(method)taint_winfac,"winfac", A_FLOAT, 0);
//	class_addmethod(c,(method)taint_fftsize, "fftsize", A_FLOAT, 0);
	class_addmethod(c,(method)taint_mute,"mute", A_FLOAT, 0);
	class_addmethod(c,(method)taint_bypass,"bypass", A_FLOAT, 0);
	class_addmethod(c,(method)taint_pad,"pad", A_FLOAT, 0);
	class_addmethod(c,(method)taint_fftinfo,"fftinfo", 0);
	class_addmethod(c,(method)taint_float,"float", A_FLOAT, 0);
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_taint, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_taint, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	taint_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
	
}

/* float input handling routine (MSP only)*/
void taint_float(t_taint *x, double f)
{
	int inlet = x->x_obj.z_in;
	
	if ( inlet == 2 ) {
		x->exponent = f;	
	}
	
	if ( inlet == 3 ){
		/* x->threshold = (float) (pow( 10., (f * .05))); */
		x->threshold = f;
	}
}

void taint_mute(t_taint *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void taint_bypass(t_taint *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void taint_fftsize(t_taint *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	x->fft2->N = (int) f;
	taint_init(x);
}

void taint_overlap(t_taint *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	x->fft2->overlap = (int) f;
	taint_init(x);
}

void taint_winfac(t_taint *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	x->fft2->winfac = (int) f;
	taint_init(x);
}

void taint_fftinfo( t_taint *x )
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void taint_free(t_taint *x)
{
	dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
	fftease_free(x->fft2);
    sysmem_freeptr(x->fft);
    sysmem_freeptr(x->fft2);
}

void taint_pad(t_taint *x, t_floatarg pad)
{
	x->invert_pad = pad;
	taint_invert(x,x->invert);//resubmit to invert
}

void taint_invert(t_taint *x, t_floatarg toggle)
{
	t_fftease *fft = x->fft;
	x->invert_nextstate = toggle;
	x->invert_countdown = fft->overlap; // delay effect for "overlap" vectors
	
	if(x->invert_nextstate){ // lower gain immediately; delay going to invert
		x->fft->mult = (1. / (double) x->fft->N) * x->invert_pad;
	} else {
		x->invert = 0; //immediately turn off invert; delay raising gain
	}
	// post("invert has been set");
}

void taint_assist (t_taint *x, void *b, long msg, long arg, char *dst)
{
	
	if (msg == 1) {
		
		switch (arg) {
			case 0:		sprintf(dst,"(signal) Input One");break;
			case 1:		sprintf(dst,"(signal) Input Two"); break;
			case 2:		sprintf(dst,"(signal/float) Scaling Exponent"); break;
			case 3:		sprintf(dst,"(signal/float) Inverse Threshold"); break;
		}
	}
	
	else {
		
		if (msg == 2)
			sprintf(dst,"(signal) Output");
		
	}
}


void *taint_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft, *fft2;		

	t_taint *x = (t_taint *)object_alloc(taint_class);
	dsp_setup((t_pxobject *)x,4);
	outlet_new((t_pxobject *)x, "signal");
	x->x_obj.z_misc |= Z_NO_INPLACE;

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	x->fft2 = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft2 = x->fft2;
	fft->initialized = 0;
	fft2->initialized = 0;
	
	x->exponent = 0.25;
	x->threshold = 0.01;

	
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	fft2->N = FFTEASE_DEFAULT_FFTSIZE;
	fft2->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft2->winfac = FFTEASE_DEFAULT_WINFAC;
	
	fft2->R = fft->R = sys_getsr();
	fft2->MSPVectorSize = fft->MSPVectorSize = sys_getblksize();	
	attr_args_process(x, argc, argv);
	taint_init(x);
	
	return x;
}

void taint_init(t_taint *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	
	x->x_obj.z_disabled = 1;
	short initialized = fft->initialized;
	
	fftease_init(fft);
	fftease_init(fft2);	
	if(!initialized){
		x->invert_pad = 0.025; // -32 dB
		x->invert_countdown = 0;
		x->mute = 0;
		x->invert = 0;
	} 
	if(x->invert){
		x->mult *= x->invert_pad;
    }
	x->x_obj.z_disabled = 0;
}

void do_taint(t_taint *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int i;
	int odd,even;
	double a1,b1,a2,b2;
	double *bufferOne = fft->buffer;
	double *bufferTwo = fft2->buffer;
	int N2 = fft->N2;
	double *channelOne = fft->channel;
	double *channelTwo = fft2->channel;
	double threshold = x->threshold;
	double exponent = x->exponent;
	int invert = x->invert;
	
	
	/* apply hamming window and fold our window buffer into the fft buffer */ 
	
	fold(fft);
	fold(fft2);
	
	/* do an fft */ 
	
	rdft(fft,FFT_FORWARD);
	rdft(fft2,FFT_FORWARD);
	
	/* convert to polar coordinates from complex values */
	
	if (invert) {
		
		for ( i = 0; i <= N2; i++ ) {
			
			double magnitude;	
			
			odd = ( even = i<<1 ) + 1;
			
			a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
			b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
			
			a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
			b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
			
			*(channelOne+even) = hypot( a1, b1 );
			*(channelOne+odd) = -atan2( b1, a1 );
			
			magnitude = *(channelTwo+even) = hypot( a2, b2 );
			*(channelTwo+odd) = -atan2( b2, a2 );
			
			/* use threshold for inverse filtering to avoid division by zero */
			
			if ( magnitude < threshold )
				magnitude = 0.;
			
			else  
				magnitude = 1. / magnitude;
			
			*(channelOne+even) *= magnitude;
			*(channelOne+even) = pow( *(channelOne+even), exponent );
		}
	}
	
	
	else {
		
		for ( i = 0; i <= N2; i++ ) {
			
			odd = ( even = i<<1 ) + 1;
			
			a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
			b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
			
			a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
			b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
			
			*(channelOne+even) = hypot( a1, b1 );
			*(channelOne+odd) = -atan2( b1, a1 );
			
			*(channelTwo+even) = hypot( a2, b2 );
			*(channelTwo+odd) = -atan2( b2, a2 );
			
			/* simple multiplication of magnitudes */
			
			*(channelOne+even) *= *(channelTwo+even);
			
			*(channelOne+even) = pow( *(channelOne+even), exponent );
		}
	}
	
	/* convert back to complex form, read for the inverse fft */
	
	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		*(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
		
		if ( i != N2 )
			*(bufferOne+odd) = -(*(channelOne+even)) * sin( *(channelOne+odd) );
	}
	
	
	
	rdft(fft,FFT_INVERSE);

	overlapadd(fft);
	
}

void taint_perform64(t_taint *x, t_object *dsp64, double **ins,
                     long numins, double **outs,long numouts, long vectorsize,
                     long flags, void *userparam)
{
	int		
    i,j;
	
	
	/* get our inlets and outlets */
		
	double *MSPInputVector1 = ins[0];
	double *MSPInputVector2 = ins[1];
	double *vec_exponent = ins[2];
	double *vec_threshold = ins[3];
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
	double  *inputTwo = fft2->input;
	double  *output = fft->output;
	int D = fft->D;
	int Nw = fft->Nw;
	double mult = fft->mult;	
	
	short *connected = x->connected;
	/* dereference structure  */	
	if(connected[2])
		x->exponent = *vec_exponent;
	if(connected[3]){
		x->threshold = *vec_threshold;
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
	if(x->invert_countdown > 0){

		--(x->invert_countdown);
		if(! x->invert_countdown){ // countdown just ended
			if(x->invert_nextstate){ // moving to invert (gain is already down)
				x->invert = x->invert_nextstate;
			} else { // invert is already off - now reset gain
				mult = x->fft->mult = 1. / (double) x->fft->N;
			}
		}
	}
//
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(inputOne + D, inputOne, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector1, inputOne + (Nw - D), D * sizeof(t_double));
        sysmem_copyptr(inputTwo + D, inputTwo, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector2, inputTwo + (Nw - D), D * sizeof(t_double));
		do_taint(x);
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
			do_taint(x);
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
			do_taint(x);
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}		

t_max_err get_fftsize(t_taint *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_taint *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		x->fft2->N = (int) val;
		taint_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_taint *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_taint *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		x->fft2->overlap = (int) val;
		taint_init(x);
	}
	return MAX_ERR_NONE;
}

void taint_dsp64(t_taint *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    int i;
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
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
		fft2->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft2);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
        fft2->R = samplerate;
	}
    if(reset_required){
        taint_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,taint_perform64,0,NULL);
}

