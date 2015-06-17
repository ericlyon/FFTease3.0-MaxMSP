#include "fftease.h"

static t_class *codepend_class;

#define OBJECT_NAME "codepend~"


/*
 Adding -32dB pad for invert option. Also added latency mechanism in
 switching from normal to "invert" to avoid glitches from extreme
 amplitude disparities.
 
 Made all inlets of type signal (with float options).
 
 Threshold input is now linear, not dB (with Max doing the conversion
 if desired).
 
 -EL 10/1/2005
 
 */

typedef struct _codepend
{
	t_pxobject x_obj;
	t_fftease *fft;
	t_fftease *fft2; // for cross synthesis use
	double threshold;
	double exponent;
	short connected[8];
	short mute;
	short bypass;
	int invert_countdown; // delay onset of invert effect to avoid loud glitches
	int invert_nextstate;// next state for invert   
	int invert;
	double invert_pad;  
	long fftsize_attr;
	long overlap_attr;
} t_codepend;


/* msp function prototypes */

void *codepend_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *codepend_perform(t_int *w);
// void codepend_dsp(t_codepend *x, t_signal **sp, short *count);
void codepend_assist(t_codepend *x, void *b, long m, long a, char *s);
void codepend_float(t_codepend *x, double f);
void codepend_invert(t_codepend *x, t_floatarg toggle);
void codepend_free(t_codepend *x);
void codepend_mute(t_codepend *x, t_floatarg toggle);
void codepend_fftinfo(t_codepend *x);
void codepend_tilde_setup(void);
void codepend_winfac(t_codepend *x, t_floatarg o);
//void codepend_overlap(t_codepend *x, t_floatarg o);
void codepend_init(t_codepend *x);
void codepend_pad(t_codepend *x, t_floatarg pad);
//void codepend_fftsize(t_codepend *x, t_floatarg f);
void codepend_bypass(t_codepend *x, t_floatarg toggle);
// void codepend_version(void);
t_max_err set_fftsize(t_codepend *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_codepend *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_codepend *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_codepend *x, void *attr, long *ac, t_atom **av);
void codepend_dsp64(t_codepend *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void codepend_perform64(t_codepend *x, t_object *dsp64, double **ins,
                          long numins, double **outs,long numouts, long vectorsize,
                          long flags, void *userparam);
int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.codepend~", (method)codepend_new, (method)dsp_free, sizeof(t_codepend),0,A_GIMME,0);
	
	class_addmethod(c,(method)codepend_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)codepend_assist,"assist",A_CANT,0);    
	class_addmethod(c,(method)codepend_invert,"invert", A_FLOAT, 0);  
	class_addmethod(c,(method)codepend_bypass,"bypass", A_FLOAT, 0);	
	class_addmethod(c,(method)codepend_mute,"mute", A_FLOAT, 0);
	class_addmethod(c,(method)codepend_pad,"pad", A_FLOAT, 0);
	class_addmethod(c,(method)codepend_winfac,"winfac", A_FLOAT, 0);
	class_addmethod(c,(method)codepend_fftinfo,"fftinfo", 0);
	class_addmethod(c,(method)codepend_float,"float", 0);
	CLASS_ATTR_LONG(c, "fftsize", 0, t_codepend, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_LONG(c, "overlap", 0, t_codepend, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	codepend_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

/* float input handling routine (MSP only)*/
void codepend_float(t_codepend *x, double df)
{
	float f = (float) df;
	int inlet = x->x_obj.z_in;
	
	if ( inlet == 2 ) {
		x->exponent = f;	
	}
	
	if ( inlet == 3 ){
		/* x->threshold = (float) (pow( 10., (f * .05))); */
		x->threshold = f;
	}
}

void codepend_version(void)
{
	fftease_version(OBJECT_NAME);
}

void codepend_bypass(t_codepend *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void codepend_mute(t_codepend *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
	//  post("mute set to %f, %d",toggle,x->mute);
}

void codepend_overlap(t_codepend *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	x->fft2->overlap = (int) f;
	
	codepend_init(x);
}

void codepend_winfac(t_codepend *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	x->fft2->winfac = (int) f;
	
	codepend_init(x);
}

void codepend_fftsize(t_codepend *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	x->fft2->N = (int) f;
	codepend_init(x);
}


void codepend_fftinfo( t_codepend *x )
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void codepend_free(t_codepend *x)
{
	dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
	fftease_free(x->fft2);
    sysmem_freeptr(x->fft);
    sysmem_freeptr(x->fft2);
}

void codepend_pad(t_codepend *x, t_floatarg pad)
{
	x->invert_pad = pad;
	codepend_invert(x,x->invert);//resubmit to invert
}

void codepend_invert(t_codepend *x, t_floatarg toggle)
{
	
	x->invert_nextstate = (short)toggle;
	x->invert_countdown = x->fft->overlap; // delay effect for "overlap" vectors
	
	if(x->invert_nextstate){ // lower gain immediately; delay going to invert
		x->fft->mult = (1. / (float) x->fft->N) * x->invert_pad;
	} else {
		x->invert = 0; //immediately turn off invert; delay raising gain
	}
	
}

void codepend_assist (t_codepend *x, void *b, long msg, long arg, char *dst)
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


void *codepend_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft, *fft2;
	t_codepend *x = (t_codepend *)object_alloc(codepend_class);
	dsp_setup((t_pxobject *)x,4);
	outlet_new((t_pxobject *)x, "signal");
	x->x_obj.z_misc |= Z_NO_INPLACE; // probably not needed

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	x->fft2 = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft2 = x->fft2;
	fft->initialized = 0;
	fft2->initialized = 0;
    fft->MSPVectorSize = sys_getblksize();
    fft2->MSPVectorSize = sys_getblksize();

	
	/* optional arguments: scaling exponent, threshold (now linear), overlap, winfac */
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
	
	/* if(! fftease_msp_sanity_check(fft, OBJECT_NAME)){
		return 0;
	} */
	attr_args_process(x, argc, argv);
	codepend_init(x);
	return x;
}

void codepend_init(t_codepend *x )
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
		x->fft->mult *= x->invert_pad;
    }
	x->x_obj.z_disabled = 0;
}

void do_codepend(t_codepend *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int i;
	int N2 = fft->N2;
	float a1, b1, a2, b2, threshold = 0.1;
	int even, odd;
	int invert = x->invert;
	double exponent = x->exponent;
	double *bufferOne = fft->buffer;
	double *bufferTwo = fft2->buffer;
	double *channelOne = fft->channel;
	//	float *channelTwo = fft2->channel;
	
	if(x->invert_countdown > 0){
		
		if(x->invert) { // we 
		} else {
		}  
		--(x->invert_countdown);
		if(! x->invert_countdown){ // countdown just ended
			if(x->invert_nextstate){ // moving to invert (gain is already down)
				x->invert = x->invert_nextstate;
			} else { // invert is already off - now reset gain
				x->fft->mult = 1. / (float) x->fft->N;
			}
		}
	}
	if ( x->threshold != 0. )
		threshold = x->threshold;		
	
	fold(fft);
	fold(fft2);
	
	rdft(fft, 1);
	rdft(fft2, 1);
	
	if (invert) {
		
		for ( i = 0; i <= N2; i++ ) {
			
			float mag_1, mag_2;
			
			odd = ( even = i<<1 ) + 1;
			
			a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
			b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
			
			a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
			b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
			
			/* complex division */	
			
			mag_1 = hypot( a1, b1 );
			mag_2 = hypot( a2, b2 );
			
			if ( mag_2 > threshold )
				*(channelOne+even) =  mag_1 / mag_2;
			
			else
				*(channelOne+even) =  mag_1 / threshold;
			
			if ( mag_1 != 0. && mag_2 != 0. )
				*(channelOne+odd) = atan2( b2, a2 ) - atan2( b1, a1 );
			
			else 
				*(channelOne+odd) = 0.;
			
			/* raise resulting magnitude to a desired power */
			
			*(channelOne+even) = pow( *(channelOne+even), exponent );
		}  
	}
	
	
	else {
		
		for ( i = 0; i <= N2; i++ ) {
			
			float f_real, f_imag;
			
			odd = ( even = i<<1 ) + 1;
			
			a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
			b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
			
			a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
			b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
			
			/* complex multiply */
			
			f_real = (a1 * a2) - (b1 * b2);
			f_imag = (a1 * b2) + (b1 * a2);	
			
			*(channelOne+even) = hypot( f_real, f_imag );
			*(channelOne+odd) = -atan2( f_imag, f_real );
			
			/* raise resulting magnitude to a desired power */
			
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
	
	rdft(fft, -1);
	overlapadd(fft);
}

void codepend_perform64(t_codepend *x, t_object *dsp64, double **ins,
                          long numins, double **outs,long numouts, long vectorsize,
                          long flags, void *userparam)
{
	int i, j;
	/* get our inlets and outlets */
	
	double *MSPInputVector1 = ins[0];
	double *MSPInputVector2 = ins[1];
	double *vec_exponent = ins[2];
	double *vec_threshold = ins[3];
	double *MSPOutputVector = outs[0];
	
	short *connected = x->connected;
	if(connected[2])
		x->exponent = *vec_exponent;
	if(connected[3]){
		x->threshold = *vec_threshold;
	}
	
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
	float mult = fft->mult;	
	
	/* save some CPUs if muted */
	if(x->mute || x->x_obj.z_disabled){
        for(i=0; i < vectorsize; i++){ MSPOutputVector[i] = 0.0; }
	}
	if (x->bypass) {
		for( j = 0; j < MSPVectorSize; j++) {
			*MSPOutputVector++ = *MSPInputVector1++ * FFTEASE_BYPASS_GAIN;
		}
	}
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){	
        sysmem_copyptr(inputOne + D, inputOne, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector1, inputOne + (Nw - D), D * sizeof(t_double));
        sysmem_copyptr(inputTwo + D, inputTwo, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector2, inputTwo + (Nw - D), D * sizeof(t_double));
		do_codepend(x);
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
			do_codepend(x);
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
			do_codepend(x);
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}		

t_max_err get_fftsize(t_codepend *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_codepend *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		x->fft2->N = (int) val;
		codepend_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_codepend *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_codepend *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		x->fft2->overlap = (int) val;
		codepend_init(x);
	}
	return MAX_ERR_NONE;
}

void codepend_dsp64(t_codepend *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    int i;
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    if(! samplerate){
        return;
    }
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
	}
    if(reset_required){
        codepend_init(x);
    }
    object_method(dsp64, gensym("dsp_add64"),x,codepend_perform64,0,NULL);
}

