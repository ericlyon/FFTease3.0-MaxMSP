#include "fftease.h"

static t_class *swinger_class;

#define OBJECT_NAME "swinger~"

typedef struct _swinger
{
	t_pxobject x_obj;
	t_fftease *fft;
	t_fftease *fft2;
	short mute;
	short bypass;
	long fftsize_attr;
	long overlap_attr;
} t_swinger;


/* msp function prototypes */

void *swinger_new(t_symbol *s, int argc, t_atom *argv);
void swinger_assist(t_swinger *x, void *b, long m, long a, char *s);
void swinger_mute(t_swinger *x, t_floatarg state);
void swinger_bypass(t_swinger *x, t_floatarg toggle);
void swinger_init(t_swinger *x);
void swinger_dsp_free(t_swinger *x);
// void swinger_overlap(t_swinger *x, t_floatarg o);
void swinger_winfac(t_swinger *x, t_floatarg o);
// void swinger_fftsize(t_swinger *x, t_floatarg f);
void swinger_fftinfo(t_swinger *x);
t_max_err set_fftsize(t_swinger *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_swinger *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_swinger *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_swinger *x, void *attr, long *ac, t_atom **av);
void swinger_dsp64(t_swinger *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void swinger_perform64(t_swinger *x, t_object *dsp64, double **ins,
                    long numins, double **outs,long numouts, long vectorsize,
                    long flags, void *userparam);
int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.swinger~", (method)swinger_new, (method)swinger_dsp_free, sizeof(t_swinger),0,A_GIMME,0);
	
    class_addmethod(c,(method)swinger_dsp64, "dsp64", A_CANT, 0);
    class_addmethod(c,(method)swinger_assist,"assist",A_CANT,0);
    class_addmethod(c,(method)swinger_mute,"mute",A_FLOAT,0);
	class_addmethod(c,(method)swinger_bypass,"bypass", A_FLOAT, 0);
	class_addmethod(c,(method)swinger_winfac,"winfac",A_FLOAT,0);
	class_addmethod(c,(method)swinger_fftinfo,"fftinfo",0);

	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_swinger, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_swinger, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	swinger_class = c;
	
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
	
}

/* diagnostic messages for Max */
void swinger_fftinfo(t_swinger *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void swinger_bypass(t_swinger *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void swinger_mute(t_swinger *x, t_floatarg state)
{
	x->mute = state;	
}

void swinger_assist (t_swinger *x, void *b, long msg, long arg, char *dst)
{
	
	if (msg == 1) {
		
		switch (arg) {
				
			case 0:		sprintf(dst,"(signal) Signal to be Phase Replaced ");
				break;
				
			case 1:		sprintf(dst,"(signal) Signal to Supply Phase Information ");
				break;
		}
	}
	
	else {
		
		if (msg == 2)
			sprintf(dst,"(signal) Swinger Output");
		
	}
}


void *swinger_new(t_symbol *s, int argc, t_atom *argv)
{

	t_fftease *fft, *fft2;	

	t_swinger *x = (t_swinger *)object_alloc(swinger_class);
	dsp_setup((t_pxobject *)x,2);
	outlet_new((t_pxobject *)x, "signal");
	/* make sure that signal inlets and outlets have their own memory */
	x->x_obj.z_misc |= Z_NO_INPLACE;
	
	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	x->fft2 = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft2 = x->fft2;
	fft->initialized = 0;
	fft2->initialized = 0;

	fft2->R = fft->R = sys_getsr();
	fft2->MSPVectorSize = fft->MSPVectorSize = sys_getblksize();

	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	fft2->N = FFTEASE_DEFAULT_FFTSIZE;
	fft2->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft2->winfac = FFTEASE_DEFAULT_WINFAC;
	
	attr_args_process(x, argc, argv);
	swinger_init(x); 
	return x;
}

void swinger_fftsize(t_swinger *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	x->fft2->N = (int) f;
	swinger_init(x);
}

void swinger_overlap(t_swinger *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	x->fft2->overlap = (int) f;
	swinger_init(x);
}

void swinger_winfac(t_swinger *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	x->fft2->winfac = (int) f;
	swinger_init(x);
}

void swinger_init(t_swinger *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	
	x->x_obj.z_disabled = 1;
	if(!fft->initialized){
		x->mute = 0;
	}	
	fftease_init(fft);
	fftease_init(fft2);

	x->x_obj.z_disabled = 0;
}

void do_swinger(t_swinger *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int		
	i,
	
	R,
	N,
	N2,
	D,
	Nw,
	
	even, odd;
	
	double
	a1, b1,
	a2, b2,
	*bufferOne,
	*bufferTwo,
	*channelOne,
	*channelTwo;
	bufferOne = fft->buffer;
	bufferTwo = fft2->buffer;
	R = fft->R;
	N = fft->N;
	N2 = fft->N2;
	D = fft->D;
	Nw = fft->Nw;
	channelOne = fft->channel;
	channelTwo = fft2->channel;
	
	
	/* apply hamming window and fold our window buffer into the fft buffer */ 
	
	fold(fft);
	fold(fft2);
	
	/* do an fft */ 
	
	rdft(fft,FFT_FORWARD);
	rdft(fft2,FFT_FORWARD);
	
	/* use redundant coding for speed, even though moving the invert variable
	 comparison outside of the for loop will give us only a minimal performance
	 increase (hypot and atan2 are the most intensive portions of this code).
	 consider adding a table lookup for atan2 instead.
	 */
	
	/* convert to polar coordinates from complex values */
	
	for ( i = 0; i <= N2; i++ ) {
		odd = ( even = i<<1 ) + 1;
		
		a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
		b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
		
		a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
		b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
		
		/* replace signal one's phases with those of signal two */
		
		*(channelOne+even) = hypot( a1, b1 );
		*(channelOne+odd) = -atan2( b2, a2 );        
	}
	
	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		*(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
		
		if ( i != N2 )
			*(bufferOne+odd) = -(*(channelOne+even)) * sin( *(channelOne+odd) );
	}
	
	
	/* do an inverse fft */
	
	rdft(fft,FFT_INVERSE);
	
	
	
	/* dewindow our result */
	
	overlapadd(fft);
	
	/* set our output and adjust our retaining output buffer */
	
	
}

void swinger_perform64(t_swinger *x, t_object *dsp64, double **ins,
                       long numins, double **outs,long numouts, long vectorsize,
                       long flags, void *userparam)
{
	
	int i,j;
	double *MSPInputVector1 = ins[0];
	double *MSPInputVector2 = ins[1];
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
	
	/* no computation if muted */
	
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
    //
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(inputOne + D, inputOne, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector1, inputOne + (Nw - D), D * sizeof(t_double));
        sysmem_copyptr(inputTwo + D, inputTwo, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector2, inputTwo + (Nw - D), D * sizeof(t_double));
		do_swinger(x);
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
			do_swinger(x);
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
			do_swinger(x);
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}	

void swinger_dsp_free( t_swinger *x )
{
	dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
	fftease_free(x->fft2);
    sysmem_freeptr(x->fft);
    sysmem_freeptr(x->fft2);
}

t_max_err get_fftsize(t_swinger *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_swinger *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		swinger_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_swinger *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_swinger *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		x->fft2->overlap = (int) val;
		swinger_init(x);
	}
	return MAX_ERR_NONE;
}

void swinger_dsp64(t_swinger *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
	
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
        swinger_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,swinger_perform64,0,NULL);
}

