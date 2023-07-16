#include "fftease.h"

static t_class *vacancy_class;

#define OBJECT_NAME "vacancy~"

typedef struct _vacancy
{
	t_pxobject x_obj;
	t_fftease *fft;
	t_fftease *fft2;
    int invert;
    int useRms;
    int swapPhase;
	short connected[8];
	short mute;
	short bypass;
	double threshold;
	long fftsize_attr;
	long overlap_attr;
} t_vacancy;


/* msp function prototypes */

void *vacancy_new(t_symbol *s, int argc, t_atom *argv);
void vacancy_assist(t_vacancy *x, void *b, long m, long a, char *s);
void vacancy_float(t_vacancy *x, double f);
void vacancy_rms(t_vacancy *x, t_floatarg f);
void vacancy_invert(t_vacancy *x, t_floatarg f);
void vacancy_swapphase(t_vacancy *x, t_floatarg f);
void vacancy_bypass(t_vacancy *x, t_floatarg toggle);
void vacancy_free(t_vacancy *x);
void vacancy_mute(t_vacancy *x, t_floatarg toggle);
void vacancy_fftinfo(t_vacancy *x);
void vacancy_tilde_setup(void);
// void vacancy_winfac(t_vacancy *x, t_floatarg o);
void vacancy_init(t_vacancy *x);
t_max_err set_fftsize(t_vacancy *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_vacancy *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_vacancy *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_vacancy *x, void *attr, long *ac, t_atom **av);
void vacancy_dsp64(t_vacancy *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void vacancy_perform64(t_vacancy *x, t_object *dsp64, double **ins,
                    long numins, double **outs,long numouts, long vectorsize,
                    long flags, void *userparam);
int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.vacancy~", (method)vacancy_new, (method)vacancy_free, sizeof(t_vacancy),0,A_GIMME,0);
	
    class_addmethod(c,(method)vacancy_dsp64, "dsp64", A_CANT, 0);
    class_addmethod(c,(method)vacancy_assist,"assist",A_CANT,0);    
    class_addmethod(c,(method)vacancy_rms,"rms", A_FLOAT, 0);
    class_addmethod(c,(method)vacancy_invert,"invert", A_FLOAT, 0);
	class_addmethod(c,(method)vacancy_swapphase,"swapphase", A_FLOAT, 0);
	class_addmethod(c,(method)vacancy_mute,"mute", A_FLOAT, 0);
	class_addmethod(c,(method)vacancy_bypass,"bypass", A_FLOAT, 0);
	class_addmethod(c,(method)vacancy_fftinfo,"fftinfo", 0);
	class_addmethod(c,(method)vacancy_float,"float",A_FLOAT,0);
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_vacancy, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_vacancy, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	vacancy_class = c;
	return 0;
}

void vacancy_float(t_vacancy *x, double f)
{
	int inlet = x->x_obj.z_in;
	
	if(inlet == 2){
		x->threshold = (float) f;
	}
}


void vacancy_version(void)
{
	fftease_version(OBJECT_NAME);
}

void vacancy_rms(t_vacancy *x, t_floatarg f)
{
	x->useRms = (int) f;
}

void vacancy_invert(t_vacancy *x, t_floatarg f)
{
	x->invert = (int) f;
}

void vacancy_swapphase(t_vacancy *x, t_floatarg f)
{
	x->swapPhase = (int) f;
}


void vacancy_assist (t_vacancy *x, void *b, long msg, long arg, char *dst)
{
	
	if (msg == 1) {
		
		switch (arg) {
				
			case 0:		sprintf(dst,"(signal) Input One"); break;
			case 1:		sprintf(dst,"(signal) Input Two"); break;
			case 2:		sprintf(dst,"(signal) Mixing Threshold"); break;
		}
	}
	
	else {
		
		if (msg == 2)
			sprintf(dst,"(signal) Output");
		
	}
}

void vacancy_fftsize(t_vacancy *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	x->fft2->N = (int) f;
	vacancy_init(x);
}

void vacancy_fftinfo( t_vacancy *x )
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void vacancy_mute(t_vacancy *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void vacancy_bypass(t_vacancy *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}


void vacancy_overlap(t_vacancy *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	x->fft2->overlap = (int) f;
	vacancy_init(x);
}

void vacancy_winfac(t_vacancy *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	x->fft2->winfac = (int) f;
	vacancy_init(x);
}

void *vacancy_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft, *fft2;	

	t_vacancy *x = (t_vacancy *)object_alloc(vacancy_class);

	dsp_setup((t_pxobject *)x,3);
	outlet_new((t_pxobject *)x, "signal");
	x->x_obj.z_misc |= Z_NO_INPLACE;

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
	attr_args_process(x, argc, argv);
	vacancy_init(x);
	
	return x;
}

void vacancy_init(t_vacancy *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	short initialized = x->fft->initialized;
	x->x_obj.z_disabled = 1;
	fftease_init(fft);
	fftease_init(fft2);
	
	if(!initialized){
		x->mute = 0;
		x->bypass = 0;
		x->invert = 0;
		x->threshold = 0.01;
		x->useRms = 1;
		x->swapPhase = 0;
	}
	x->x_obj.z_disabled = 0;
}

void vacancy_free(t_vacancy *x)
{
	dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
    fftease_free(x->fft2);
    sysmem_freeptr(x->fft);
    sysmem_freeptr(x->fft2);
}



void do_vacancy(t_vacancy *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int	
	i,

	even, odd;
	float
	useme,
	rms = 0.,
	a1, b1,
	a2, b2;


	
	/* dereference structure  */	
	
	double *bufferOne = fft->buffer;
	double *bufferTwo = fft2->buffer;
	int N2 = fft->N2;
	int Nw = fft->Nw;
	double *channelOne = fft->channel;
	double *channelTwo = fft2->channel;
	double *inputOne = fft->input;
	int invert = x->invert;
	int useRms = x->useRms;
	int swapPhase = x->swapPhase;
	

	
	if (useRms) {
		
		rms = 0.;
		
		for ( i=0; i < Nw; i++ )
			rms += *(inputOne+i) * *(inputOne+i);
		
		rms = sqrt( rms / Nw );
		
		useme = rms * x->threshold;
	}
	
	else
		useme = x->threshold;
	
	fold(fft);
	fold(fft2);
	
	rdft(fft,FFT_FORWARD);
	rdft(fft2,FFT_FORWARD);
	

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
	}
	
	


	if (invert) {
		
		if (swapPhase) {
			
			for ( i=0; i < N2; i+=2 ) {
				if ( *(channelOne+i) > useme && *(channelTwo+i) < *(channelOne+i) ) {
					*(channelOne+i) = *(channelTwo+i);
					*(channelOne+i+1) = *(channelTwo+i+1);
				}
			}
		}
		
		else {
			
			for ( i=0; i < N2; i+=2 ) {
				if ( *(channelOne+i) > useme && *(channelTwo+i) < *(channelOne+i) ) {
					*(channelOne+i) = *(channelTwo+i);
					
					if ( *(channelOne+i+1) == 0. )
						*(channelOne+i+1) = *(channelTwo+i+1);
				}
			}
		}
	}
	
	else {
		
		if (swapPhase) {
			
			for ( i=0; i < N2; i+=2 ) {
				if ( *(channelOne+i) < useme && *(channelTwo+i) > *(channelOne+i) ) {
					*(channelOne+i) = *(channelTwo+i);
					*(channelOne+i+1) = *(channelTwo+i+1);
				}
			}
		}
		
		else {
			
			for ( i=0; i < N2; i+=2 ) {
				
				if ( *(channelOne+i) < useme && *(channelTwo+i) > *(channelOne+i) ) {
					*(channelOne+i) = *(channelTwo+i);
					
					if ( *(channelOne+i+1) == 0. )
						*(channelOne+i+1) = *(channelTwo+i+1);
				}
			}
		}
	}
	

	
	for ( i = 0; i <= N2; i++ ) {
		
		odd = ( even = i<<1 ) + 1;
		
		*(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
		
		if ( i != N2 )
			*(bufferOne+odd) = -(*(channelOne+even)) * sin( *(channelOne+odd) );
	}
	

	
	rdft(fft,FFT_INVERSE);
	overlapadd(fft);
	
}

void vacancy_perform64(t_vacancy *x, t_object *dsp64, double **ins,
                       long numins, double **outs,long numouts, long vectorsize,
                       long flags, void *userparam)
{
	
	int	
	i,j;
	
	/* get our inlets and outlets */
	
	double *MSPInputVector1 = ins[0];
	double *MSPInputVector2 = ins[1];
	double *vec_threshold = ins[2];
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
	
//	mult = 1.0 ; // diag only!!
	
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
	if(connected[2]) x->threshold = *vec_threshold;
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(inputOne + D, inputOne, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector1, inputOne + (Nw - D), D * sizeof(t_double));
        sysmem_copyptr(inputTwo + D, inputTwo, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector2, inputTwo + (Nw - D), D * sizeof(t_double));
		do_vacancy(x);
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
			do_vacancy(x);
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
			do_vacancy(x);
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}		


t_max_err get_fftsize(t_vacancy *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_vacancy *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		x->fft2->N = (int) val;
		vacancy_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_vacancy *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_vacancy *x, void *attr, long ac, t_atom *av)
{	
    int test_overlap;
    if (ac && av) {
        long val = atom_getlong(av);
        test_overlap = fftease_overlap(val);
        if(test_overlap > 0){
            x->fft->overlap = (int) val;
            x->fft2->overlap = (int) val;
            vacancy_init(x);
        }
    }
    return MAX_ERR_NONE;
}

void vacancy_dsp64(t_vacancy *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
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
	for( i = 0; i < 3; i++ ){
		x->connected[i] = count[i];
	}
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
        vacancy_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,vacancy_perform64,0,NULL);
}


