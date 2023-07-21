#include "fftease.h"

static t_class *shapee_class;

#define OBJECT_NAME "shapee~"

typedef struct _shapee
{
	t_pxobject x_obj;
	t_fftease *fft,*fft2;
    int widthConnected;    
    double shapeWidth;
	short mute;  
	short bypass;
	long fftsize_attr;
	long overlap_attr;
} t_shapee;


/* msp function prototypes */

void *shapee_new(t_symbol *s, int argc, t_atom *argv);
void shapee_float(t_shapee *x, double myFloat);
void shapee_assist(t_shapee *x, void *b, long m, long a, char *s);
void shapee_init(t_shapee *x);
void shapee_mute(t_shapee *x, t_floatarg state);
void shapee_bypass(t_shapee *x, t_floatarg toggle);
void shapee_free(t_shapee *x);
void shapee_fftinfo(t_shapee *x);
t_max_err set_fftsize(t_shapee *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_shapee *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_shapee *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_shapee *x, void *attr, long *ac, t_atom **av);
void shapee_dsp64(t_shapee *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void shapee_perform64(t_shapee *x, t_object *dsp64, double **ins,
                    long numins, double **outs,long numouts, long vectorsize,
                    long flags, void *userparam);
/* first calling */

/* float input handling routine for shape width */

void shapee_float( t_shapee *x, double myFloat )
{
	if ( x->x_obj.z_in == 2 ) {
		
		if ( myFloat >= 1. && myFloat <= (double) x->fft->N )
			x->shapeWidth = myFloat;
	}	
}

int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.shapee~", (method)shapee_new, (method)shapee_free, sizeof(t_shapee),0,A_GIMME,0);
	
    class_addmethod(c,(method)shapee_dsp64, "dsp64", A_CANT, 0);
    class_addmethod(c,(method)shapee_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)shapee_float,"float",A_FLOAT,0);
    class_addmethod(c,(method)shapee_mute,"mute",A_FLOAT,0);
	class_addmethod(c,(method)shapee_bypass,"bypass", A_FLOAT, 0);	
	class_addmethod(c,(method)shapee_fftinfo,"fftinfo",0);
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_shapee, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_shapee, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	shapee_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}


/* diagnostic messages for Max */

void shapee_assist (t_shapee *x, void *b, long msg, long arg, char *dst)
{
	
	if (msg == 1) {
		switch (arg) {
			case 0:		sprintf(dst,"(signal) Frequency Reference");break;
			case 1:		sprintf(dst,"(signal) Amplitude Reference");break;
			case 2:		sprintf(dst,"(signal/float) Shape Width"); break;		
		}
	}
	
	else {
		
		if (msg == 2)
			sprintf(dst,"(signal) Output");
	}
}


void *shapee_new(t_symbol *s, int argc, t_atom *argv)
{

	t_fftease *fft, *fft2;		
	t_shapee *x = (t_shapee *)object_alloc(shapee_class);
	dsp_setup((t_pxobject *)x, 3);
	outlet_new((t_pxobject *)x, "signal");

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	x->fft2 = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft2 = x->fft2;
	fft->initialized = 0;
	fft2->initialized = 0;
	/* INITIALIZATIONS */
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	fft2->N = FFTEASE_DEFAULT_FFTSIZE;
	fft2->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft2->winfac = FFTEASE_DEFAULT_WINFAC;
	
	fft2->R = fft->R = sys_getsr();
	fft2->MSPVectorSize = fft->MSPVectorSize = sys_getblksize();
	x->shapeWidth = 2.0;
	attr_args_process(x, argc, argv);
	shapee_init(x); 
	return x;
}

void shapee_init(t_shapee *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	short initialized = fft->initialized;
	x->x_obj.z_disabled = 1;
	
	fftease_init(fft);
	fftease_init(fft2);
	if(!initialized){
		x->mute = 0;
	}
	x->x_obj.z_disabled = 0;
	
}

void shapee_fftinfo(t_shapee *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void shapee_mute(t_shapee *x, t_floatarg state)
{
	x->mute = (short)state;
}

void shapee_bypass(t_shapee *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void shapee_fftsize(t_shapee *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	x->fft2->N = (int) f;
	shapee_init(x);
}

void shapee_overlap(t_shapee *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	x->fft2->overlap = (int) f;
	shapee_init(x);
}

void shapee_winfac(t_shapee *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	x->fft2->winfac = (int) f;
	shapee_init(x);
	
}

void do_shapee(t_shapee *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int		
	i,j,

	R,
	N,
	N2,
	D,
	Nw,
	remainingWidth,
	even, odd;

	
	double
	a1, b1,
	a2, b2,
	*bufferOne,
	*bufferTwo,
	*channelOne,
	*channelTwo;
	

int	shapeWidth = (int) x->shapeWidth;

	bufferOne = fft->buffer;
	bufferTwo = fft2->buffer;
	R = fft->R;
	N = fft->N;
	N2 = fft->N2;
	D = fft->D;
	Nw = fft->Nw;

	channelOne = fft->channel;
	channelTwo = fft2->channel;

    
	
	if(shapeWidth < 1 || shapeWidth > N2)
		shapeWidth = 1;

	/* apply hamming window and fold our window buffer into the fft buffer */ 
	
	fold(fft);
	fold(fft2);
	
	/* do an fft */ 
	
	rdft(fft,FFT_FORWARD);
	rdft(fft2,FFT_FORWARD);
	
	/* convert to polar coordinates from complex values */ 
	
	for ( i = 0; i <= N2; i++ ) {
		odd = ( even = i<<1 ) + 1;
		
		a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
		b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
		
		a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
		b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
		
		/* replace signal one's phases with those of signal two */
		
		*(channelOne+even) = hypot( a1, b1 );
		*(channelOne+odd) = -atan2( b1, a1 );
		
		*(channelTwo+even) = hypot( a2, b2 );
		*(channelTwo+odd) = -atan2( b2, a2 );       
	}
	
	/* constrain our shapeWidth value */
	
	if ( shapeWidth > N2 )
		shapeWidth = N2;
  	
	if ( shapeWidth < 1 )
		shapeWidth = 1;
	
	
	/* lets just shape the entire signal by the shape width */
	
	for ( i=0; i < N; i += shapeWidth << 1 ) {
		
		float       amplSum = 0.,
		freqSum = 0.,
		factor;
		
		for ( j = 0; j < shapeWidth << 1; j += 2 ) {
            if(i+j < N+2){
                amplSum += *(channelTwo+i+j);
                freqSum += *(channelOne+i+j);
            }
		}
		if(freqSum <= 0.001){
			freqSum = 1.0;
		}
		if (amplSum < 0.000000001)
			factor = 0.000000001;	
		
		else	
			factor = amplSum / freqSum;
		
        for ( j = 0; j < shapeWidth * 2; j += 2 ){
            if(i+j < N+2){
                *(channelOne+i+j) *= factor;
            }
        }
	}
	
	/* copy remaining magnitudes */
	
	if ( (remainingWidth = N2 % shapeWidth) ) {
		
		int			bindex = (N2 - remainingWidth) << 1;
		
		
		float       amplSum = 0.,
		freqSum = 0.,
		factor;
		
		for ( j = 0; j < remainingWidth * 2; j += 2 ) {
			
	  		amplSum += *(channelTwo+bindex+j);
	  		freqSum += *(channelOne+bindex+j);
		}
		if(freqSum <= 0.00001){
			freqSum = 1.0;
		}
		if (amplSum < 0.000000001)
			factor = 0.000000001;	
		
		else	
			factor = amplSum / freqSum;
		
		for ( j = 0; j < remainingWidth * 2; j += 2 )
	  		*(channelOne+bindex+j) *= factor;
	}
	
	
	/* convert from polar to cartesian */	
	
	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		*(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
		
		if ( i != N2 )
			*(bufferOne+odd) = (*(channelOne+even)) * -sin( *(channelOne+odd) );
	}

	rdft(fft,FFT_INVERSE);
	overlapadd(fft);
}


void shapee_perform64(t_shapee *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam)
{
	int i,j;
	
	/* get our inlets and outlets */
	
	double *MSPInputVector1 = ins[0];
	double *MSPInputVector2 = ins[1];
	double *inShape = ins[2];
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
	/* get our shapeWidth -- from either a signal our float input */

	x->shapeWidth = x->widthConnected ?  *inShape :  x->shapeWidth;
//
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(inputOne + D, inputOne, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector1, inputOne + (Nw - D), D * sizeof(t_double));
        sysmem_copyptr(inputTwo + D, inputTwo, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector2, inputTwo + (Nw - D), D * sizeof(t_double));
		do_shapee(x);
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
			do_shapee(x);
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
			do_shapee(x);
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}	
	
void shapee_free( t_shapee *x )
{
	dsp_free( (t_pxobject *) x);
	fftease_free(x->fft);
	fftease_free(x->fft2);
    sysmem_freeptr(x->fft);
    sysmem_freeptr(x->fft2);

}
t_max_err get_fftsize(t_shapee *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_shapee *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		x->fft2->N = (int) val;
		shapee_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_shapee *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_shapee *x, void *attr, long ac, t_atom *av)
{	
    int test_overlap;
    if (ac && av) {
        long val = atom_getlong(av);
        test_overlap = fftease_overlap(val);
        if(test_overlap > 0){
            x->fft->overlap = (int) val;
            x->fft2->overlap = (int) val;
            shapee_init(x);
        }
    }
    return MAX_ERR_NONE;
}



void shapee_dsp64(t_shapee *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
	x->widthConnected = count[2];
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
        shapee_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,shapee_perform64,0,NULL);
}


