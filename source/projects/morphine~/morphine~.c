#include "fftease.h"
/*
 This external links to qsortE, so unlike others in this collection, morphine~ is covered under the GNU GPL.
 */
static t_class *morphine_class;

#define OBJECT_NAME "morphine~"

typedef struct _pickme {
	
	int		bin;
	float		value;
	
} pickme;


typedef struct _morphine
{

	t_pxobject x_obj;
	t_fftease *fft;
	t_fftease *fft2;
    pickme *picks;
    pickme *mirror;
    double morphIndex;
    double exponScale;	
	short connected[3];
	short mute;
	short bypass;
	long fftsize_attr;
	long overlap_attr;
} t_morphine;


/* msp function prototypes */

void *morphine_new(t_symbol *s, int argc, t_atom *argv);
void morphine_assist(t_morphine *x, void *b, long m, long a, char *s);
void morphine_float(t_morphine *x, double f);
void morphine_bypass(t_morphine *x, t_floatarg toggle);
int sortIncreasing( const void *a, const void *b );
int qsortE (char *base_ptr, int total_elems, int size, int (*cmp)(const void *a, const void *b));
void morphine_transition(t_morphine *x, t_floatarg f);
void morphine_free(t_morphine *x);
void morphine_mute(t_morphine *x, t_floatarg toggle);
void morphine_fftinfo(t_morphine *x);
void morphine_tilde_setup(void);
void morphine_winfac(t_morphine *x, t_floatarg o);
void morphine_init(t_morphine *x);
t_max_err set_fftsize(t_morphine *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_morphine *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_morphine *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_morphine *x, void *attr, long *ac, t_atom **av);
void morphine_dsp64(t_morphine *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void morphine_perform64(t_morphine *x, t_object *dsp64, double **ins,
                        long numins, double **outs,long numouts, long vectorsize,
                        long flags, void *userparam);

int sortIncreasing( const void *a, const void *b )
{
	
	if ( ((pickme *) a)->value > ((pickme *) b)->value )
		return 1;
	
	if ( ((pickme *) a)->value < ((pickme *) b)->value )
		return -1;
	
	return 0;
}


int C74_EXPORT main(void)
{	
	t_class *c;
	c = class_new("fftz.morphine~", (method)morphine_new, (method)morphine_free, sizeof(t_morphine),0,A_GIMME,0);
    class_addmethod(c,(method)morphine_dsp64, "dsp64", A_CANT, 0);
    class_addmethod(c,(method)morphine_assist,"assist",A_CANT,0);    
	class_addmethod(c,(method)morphine_bypass,"bypass", A_FLOAT, 0);
    class_addmethod(c,(method)morphine_transition,"transition", A_FLOAT, 0);
	class_addmethod(c,(method)morphine_mute,"mute", A_FLOAT, 0);
	class_addmethod(c,(method)morphine_fftinfo,"fftinfo", 0);
	class_addmethod(c,(method)morphine_winfac,"winfac",A_FLOAT, 0);
	class_addmethod(c,(method)morphine_float,"float",A_FLOAT,0);
	CLASS_ATTR_LONG(c, "fftsize", 0, t_morphine, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	CLASS_ATTR_LONG(c, "overlap", 0, t_morphine, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	morphine_class = c;
	return 0;
}

void morphine_float(t_morphine *x, double f)
{
	int inlet = x->x_obj.z_in;
	
	if(inlet == 2)
		x->morphIndex = f;
}

/* diagnostic messages for Max */

void morphine_assist (t_morphine *x, void *b, long msg, long arg, char *dst)
{
	
	if (msg == 1) {
		
		switch (arg) {
				
			case 0:		sprintf(dst,"(signal) Input One"); break;
			case 1:		sprintf(dst,"(signal) Input Two"); break;
			case 2:		sprintf(dst,"(signal/float) Morph Index"); break;
		}
	}
	
	else {
		
		if (msg == 2)
			sprintf(dst,"(signal) output");
		
	}
}


void morphine_transition(t_morphine *x, t_floatarg f)
{	
	x->exponScale = f;
}

void *morphine_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft, *fft2;	
	t_morphine *x = (t_morphine *)object_alloc(morphine_class);
	dsp_setup((t_pxobject *)x,3);
	outlet_new((t_pxobject *)x, "signal");
	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	x->fft2 = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft2 = x->fft2;
	fft->initialized = 0;
	fft2->initialized = 0;
	x->exponScale = -5.0;	
	atom_arg_getdouble(&x->exponScale, 0, argc, argv);
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	fft2->N = FFTEASE_DEFAULT_FFTSIZE;
	fft2->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft2->winfac = FFTEASE_DEFAULT_WINFAC;	
	fft2->R = fft->R = sys_getsr();
	fft2->MSPVectorSize = fft->MSPVectorSize = sys_getblksize();
	attr_args_process(x, argc, argv);
	morphine_init(x);
	return x;
}

void morphine_init(t_morphine *x)
{
//	int i;
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	short initialized = fft->initialized;
    
	x->x_obj.z_disabled = 1;
    
	fftease_init(fft);
	fftease_init(fft2);

	if(!initialized){
		x->morphIndex = 0.;
		x->mute = 0;
		x->picks = (pickme *) sysmem_newptrclear((fft->N2+1) * sizeof(pickme));
		x->mirror = (pickme *) sysmem_newptrclear((fft->N2+1) * sizeof(pickme));
		
	} else if(x->fft->initialized == 1) {
		x->picks = (pickme *) sysmem_resizeptrclear(x->picks, (fft->N2+1) * sizeof(pickme));
		x->mirror = (pickme *) sysmem_resizeptrclear(x->mirror, (fft->N2+1) * sizeof(pickme));
//        x->fft->input = (double*)sysmem_resizeptrclear(x->fft->input, fft->Nw * sizeof(double));
//        x->fft2->input = (double*)sysmem_resizeptrclear(x->fft2->input, fft->Nw * sizeof(double));
//        x->fft2->output = (double*)sysmem_resizeptrclear(x->fft2->output,fft->Nw *sizeof(double));
	}

	x->x_obj.z_disabled = 0;	
}

void do_morphine(t_morphine *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int	i;
	int lookupIndex,even, odd;
	double mult,
	morphIndex,
	exponScale,
	a1, b1,
	a2, b2;

	double *bufferOne = fft->buffer;
	double *bufferTwo = fft2->buffer;
	double *channelOne = fft->channel;
	double *channelTwo = fft2->channel;
	int N2 = fft->N2;
	pickme	*picks = x->picks;
	pickme *mirror = x->mirror;
	mult = fft->mult;	
	morphIndex = x->morphIndex;
	exponScale = x->exponScale;

	fold(fft);
	fold(fft2);
	
	/* do an fft */ 
	
	rdft(fft,1);
	rdft(fft2,1);
	
	/* convert to polar coordinates from complex values */
	
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
		
		
		/* find amplitude differences between home and visitors */
		
		(picks+i)->value = fabs( *(channelOne+even) - 
								*(channelTwo+even) );
		(picks+i)->bin = i;  
    }
	
	/* sort our differences in ascending order */
	
	
	qsortE( (char *) picks, (int) N2+1, (int) sizeof(pickme),
		   sortIncreasing );
	
	/* now we create an effective mirror of the sorted distribution.
	 we will assure that the initial transition will be made from
	 small spectral differences (when the sort behavior is increasing)
	 and the ending transition will also be made from small spectral
	 differences */
	
	for ( i=0; i <= N2; i += 2 ) {
        (mirror+(i/2))->bin = (picks+i)->bin;
        (mirror+(i/2))->value = (picks+i)->value;
	}
	
	for ( i=1; i <= N2; i += 2 ) {
        (mirror+(N2-(i/2)))->bin = (picks+i)->bin;
        (mirror+(N2-(i/2)))->value = (picks+i)->value;
	}
	
	
	/* calculate our morphIndex from an exponential function based on exponScale */
	
	if (exponScale == 0.) 
		lookupIndex = (int) (( (float) N2 ) * morphIndex);
	
	else {
		
     	if ( morphIndex < .5 ) {
			
			lookupIndex = (int) ( ((float) N2) * ((
												   (1. - exp( exponScale * morphIndex * 2. )) /
												   (1. - exp( exponScale )) ) * .5) );
    	}
		
    	else {
			
			lookupIndex = (int) ( ((float) N2) * ( .5 +  
												  (( (1. - exp( -exponScale * (morphIndex - .5) * 2. )) /
													(1. - exp( -exponScale )) ) * .5) ) );
    	}			 
		
	}
	
	
	//      post("%d", lookupIndex);
	
	/* choose the bins that are least different first */
	
    for ( i=0; i <= lookupIndex; i++ ) {
		
		even = ((mirror+i)->bin)<<1,
		odd = (((mirror+i)->bin)<<1) + 1;	
		
		*(channelOne+even) = *(channelTwo+even);
		*(channelOne+odd) = *(channelTwo+odd);
    }
	
	/* convert back to complex form, read for the inverse fft */
	
	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		*(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
		
		if ( i != N2 )
			*(bufferOne+odd) = -(*(channelOne+even)) * sin( *(channelOne+odd) );
	}
	rdft(fft,-1);
	overlapadd(fft);
}

void morphine_perform64(t_morphine *x, t_object *dsp64, double **ins,
                        long numins, double **outs,long numouts, long vectorsize,
                        long flags, void *userparam)
{

	int i, j;
	double *MSPInputVector1 = ins[0];
	double *MSPInputVector2 = ins[1];
	double *vec_morphIndex = ins[2];
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
	double morphIndex = x->morphIndex;
	
	short *connected = x->connected;
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
	morphIndex = connected[2] ? *vec_morphIndex : x->morphIndex;
	if ( morphIndex < 0 )
		morphIndex = 0.;
	else {
		if ( morphIndex > 1. )
			morphIndex = 1.;
	}
	x->morphIndex = morphIndex;
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(inputOne + D, inputOne, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector1, inputOne + (Nw - D), D * sizeof(t_double));
        sysmem_copyptr(inputTwo + D, inputTwo, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector2, inputTwo + (Nw - D), D * sizeof(t_double));
		do_morphine(x);
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
			do_morphine(x);
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
			do_morphine(x);
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}

void morphine_free(t_morphine *x)
{
	dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
	fftease_free(x->fft2);
    sysmem_freeptr(x->fft);
    sysmem_freeptr(x->fft2);
	sysmem_freeptr(x->picks);
	sysmem_freeptr(x->mirror);
}

void morphine_fftsize(t_morphine *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	x->fft2->N = (int) f;
	morphine_init(x);
}


void morphine_fftinfo( t_morphine *x )
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}


void morphine_overlap(t_morphine *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	x->fft2->overlap = (int) f;
	morphine_init(x);
}

void morphine_winfac(t_morphine *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	x->fft2->winfac = (int) f;
	morphine_init(x);
}

void morphine_bypass(t_morphine *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void morphine_mute(t_morphine *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

t_max_err get_fftsize(t_morphine *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_morphine *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		x->fft2->N = (int) val;
		morphine_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_morphine *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_morphine *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		x->fft2->overlap = (int) val;
		morphine_init(x);
	}
	return MAX_ERR_NONE;
}



void morphine_dsp64(t_morphine *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    int i;
    t_fftease *fft = x->fft;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
	for( i = 0; i < 3; i++ ){
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
        morphine_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,morphine_perform64,0,NULL);
}


