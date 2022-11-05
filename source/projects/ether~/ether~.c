#include "fftease.h"

static t_class *ether_class;

#define OBJECT_NAME "ether~"


/* Added a new inlet for the composite index */

typedef struct _ether
{
	t_pxobject x_obj;
	t_fftease *fft;
	t_fftease *fft2;	
    int invert;
    double threshMult;
	short connected[3];
	short mute;
	short bypass;
	long fftsize_attr;
	long overlap_attr;
} t_ether;


void *ether_new(t_symbol *s, int argc, t_atom *argv);
void ether_assist(t_ether *x, void *b, long m, long a, char *s);
void ether_float(t_ether *x, double f);
void ether_invert(t_ether *x, t_floatarg toggle);
void ether_init(t_ether *x);
void ether_free(t_ether *x);
void ether_mute(t_ether *x, t_floatarg toggle);
void ether_fftinfo(t_ether *x);
void ether_winfac(t_ether *x, t_floatarg f);
//void ether_overlap(t_ether *x, t_floatarg o);
void ether_tilde_setup(void);
//void ether_fftsize(t_ether *x, t_floatarg f);
void ether_bypass(t_ether *x, t_floatarg toggle);
t_max_err set_fftsize(t_ether *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_ether *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_ether *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_ether *x, void *attr, long *ac, t_atom **av);
void ether_dsp64(t_ether *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void ether_perform64(t_ether *x, t_object *dsp64, double **ins,
                     long numins, double **outs,long numouts, long vectorsize,
                     long flags, void *userparam);

int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.ether~", (method)ether_new, (method)ether_free, sizeof(t_ether),0,A_GIMME,0);
	
    class_addmethod(c,(method)ether_dsp64, "dsp64", A_CANT, 0);
    class_addmethod(c,(method)ether_assist,"assist",A_CANT,0);    
	class_addmethod(c,(method)ether_float,"float",A_FLOAT,0);  
    class_addmethod(c,(method)ether_invert,"invert", A_FLOAT, 0);
    class_addmethod(c,(method)ether_mute,"mute", A_FLOAT, 0);
	class_addmethod(c,(method)ether_bypass,"bypass", A_FLOAT, 0);
	class_addmethod(c,(method)ether_winfac,"winfac", A_FLOAT, 0);
	class_addmethod(c,(method)ether_fftinfo,"fftinfo",  0);
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_ether, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_ether, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	ether_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

void ether_float(t_ether *x, double f)
{
	x->threshMult = (float) f;	
}

/* diagnostic messages for Max */


void ether_assist (t_ether *x, void *b, long msg, long arg, char *dst)
{
	
	if (msg == 1) {
		
		switch (arg) {
				
			case 0:		sprintf(dst,"(signal) Input One");break;
			case 1:		sprintf(dst,"(signal) Input Two"); break;
			case 2:		sprintf(dst,"(signal) Composite Index"); break;
		}
	}
	
	else {
		
		if (msg == 2)
			sprintf(dst,"(signal) Output");
		
	}
}

void ether_version(void)
{
	fftease_version(OBJECT_NAME);
}

void ether_free(t_ether *x)
{
	dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
	fftease_free(x->fft2);
    sysmem_freeptr(x->fft);
    sysmem_freeptr(x->fft2);
}

void ether_fftsize(t_ether *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	x->fft2->N = (int) f;
	ether_init(x);
}

void ether_overlap(t_ether *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	x->fft2->overlap = (int) f;
	ether_init(x);
}

void ether_winfac(t_ether *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	x->fft2->winfac = (int) f;
	ether_init(x);
}

void ether_fftinfo( t_ether *x )
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
	fftease_fftinfo(x->fft2, OBJECT_NAME);
}


void *ether_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft, *fft2;

	t_ether *x = (t_ether *)object_alloc(ether_class);
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
	attr_args_process(x, argc, argv);
	ether_init(x);
	return x;
}

void ether_init(t_ether *x)
{

	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	
	short initialized = fft->initialized;
	
	x->x_obj.z_disabled = 1;
	
	fftease_init(fft);
	fftease_init(fft2);

	if(!initialized){
		x->mute = 0;
		x->invert = 0;
		x->threshMult = 0.;
	} else {
        // clear memory
        
        x->fft->input = (double *) sysmem_resizeptrclear(fft->input,fft->Nw * sizeof(double));
        x->fft2->input = (double *) sysmem_resizeptrclear(fft2->input,fft2->Nw * sizeof(double));
        x->fft->output = (double *) sysmem_resizeptrclear(fft->output,fft->Nw * sizeof(double));

	}
	x->x_obj.z_disabled = 0;
}

void do_ether(t_ether *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int i;
	int N2 = fft->N2;
	float a1, b1, a2, b2;
	int even, odd;
	int invert = x->invert;
	double threshMult = x->threshMult;
	double *bufferOne = fft->buffer;
	double *bufferTwo = fft2->buffer;
	double *channelOne = fft->channel;
	double *channelTwo = fft2->channel;
	
	fold(fft);
	fold(fft2);
	rdft(fft,1);
	rdft(fft2,1);
	
	if (invert) {	
		
		
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
						
			if ( *(channelOne+even) > *(channelTwo+even) * threshMult )
				*(channelOne+even) = *(channelTwo+even);
			
			if ( *(channelOne+odd) == 0. )
				*(channelOne+odd) = *(channelTwo+odd);	 
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
			
			
			if ( *(channelOne+even) < *(channelTwo+even) * threshMult )
				*(channelOne+even) = *(channelTwo+even);
			
			if ( *(channelOne+odd) == 0. )
				*(channelOne+odd) = *(channelTwo+odd);	 
		}  
	}
	

	for ( i = 0; i <= N2; i++ ) {
		odd = ( even = i<<1 ) + 1;
		
		*(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
		
		if ( i != N2 )
			*(bufferOne+odd) = -(*(channelOne+even)) * sin( *(channelOne+odd) );
	}
	rdft(fft, -1);
	overlapadd(fft);
}

void ether_perform64(t_ether *x, t_object *dsp64, double **ins,
                     long numins, double **outs,long numouts, long vectorsize,
                     long flags, void *userparam)
{
	int i,j;
	double *MSPInputVector1 = ins[0];
	double *MSPInputVector2 = ins[1];
	double *vec_threshMult = ins[2];
	double *MSPOutputVector = outs[0];	
	short *connected = x->connected;
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
	
	if(connected[2]){
		x->threshMult = *vec_threshMult;
	}
	if ( x->threshMult == 0. ){
		x->threshMult = 0.0001;
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
	//do_ether(x);
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(inputOne + D, inputOne, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector1, inputOne + (Nw - D), D * sizeof(t_double));
        sysmem_copyptr(inputTwo + D, inputTwo, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector2, inputTwo + (Nw - D), D * sizeof(t_double));
		do_ether(x);
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
			do_ether(x);
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
			do_ether(x);
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}		

void ether_bypass(t_ether *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void ether_mute(t_ether *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void ether_invert(t_ether *x, t_floatarg toggle)
{
	x->invert = (int)toggle;
}

t_max_err get_fftsize(t_ether *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_ether *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		x->fft2->N = (int) val;
		ether_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_ether *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_ether *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		x->fft2->overlap = (int) val;
		ether_init(x);
	}
	return MAX_ERR_NONE;
}

void ether_dsp64(t_ether *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
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
        ether_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,ether_perform64,0,NULL);
}


