#include "fftease.h"

static t_class *cross_class;

#define OBJECT_NAME "cross~"

typedef struct _cross
{

	t_pxobject x_obj;
	t_fftease *fft;
	t_fftease *fft2; 
	double threshie;
	short thresh_connected;
	short mute;//flag
	short bypass;
	short autonorm;// for self gain regulation
	long fftsize_attr;
	long overlap_attr;
} t_cross;

void *cross_new(t_symbol *s, int argc, t_atom *argv);
t_int *cross_perform(t_int *w);
void cross_assist(t_cross *x, void *b, long m, long a, char *s);
void cross_float(t_cross *x, double f);
void *cross_new(t_symbol *s, int argc, t_atom *argv);
void cross_init(t_cross *x);
//void cross_overlap(t_cross *x, t_floatarg o);
void cross_winfac(t_cross *x, t_floatarg o);
void cross_fftinfo(t_cross *x);
void cross_mute(t_cross *x, t_floatarg toggle);
void cross_autonorm(t_cross *x, t_floatarg toggle);
void cross_free(t_cross *x);
//void cross_fftsize(t_cross *x, t_floatarg f);
void cross_bypass(t_cross *x, t_floatarg toggle);
//void cross_version(void);
t_max_err set_fftsize(t_cross *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_cross *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_cross *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_cross *x, void *attr, long *ac, t_atom **av);
void cross_perform64(t_cross *x, t_object *dsp64, double **ins,
                     long numins, double **outs,long numouts, long vectorsize,
                     long flags, void *userparam);
void cross_dsp64(t_cross *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);

int C74_EXPORT main(void)
{	
	t_class *c;
	c = class_new("fftz.cross~", (method)cross_new, (method)dsp_free, sizeof(t_cross),0,A_GIMME,0);
	
	class_addmethod(c,(method)cross_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)cross_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)cross_mute,"mute",A_DEFFLOAT,0);
	class_addmethod(c,(method)cross_bypass,"bypass", A_FLOAT, 0);
	class_addmethod(c,(method)cross_winfac, "winfac",  A_DEFFLOAT, 0);
	class_addmethod(c,(method)cross_fftinfo, "fftinfo", 0);
	class_addmethod(c,(method)cross_autonorm, "autonorm",  A_DEFFLOAT, 0);
	class_addmethod(c,(method)cross_float,"float",A_FLOAT,0);
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_cross, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_cross, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	cross_class = c;
	
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

void cross_autonorm(t_cross *x, t_floatarg toggle)
{
	x->autonorm = (short) toggle;
}

void cross_assist (t_cross *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal) Driver Sound");
				break;
			case 1:
				sprintf(dst,"(signal) Filter Sound");
				break;
			case 2:
				sprintf(dst,"(float/signal) Cross Synthesis Threshold");
				break;
				
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}

void cross_fftsize(t_cross *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	x->fft2->N = (int) f;
	cross_init(x);
}

void cross_overlap(t_cross *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	x->fft2->overlap = (int) f;
	cross_init(x);
}

void cross_winfac(t_cross *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	x->fft2->winfac = (int) f;
	cross_init(x);
}

void cross_fftinfo( t_cross *x )
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void cross_bypass(t_cross *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void cross_mute(t_cross *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void cross_free(t_cross *x)
{

	dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
	fftease_free(x->fft2);
    sysmem_freeptr(x->fft);
    sysmem_freeptr(x->fft2);
}


void cross_float(t_cross *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	
	if (inlet == 2)
    {
		x->threshie = f;
    }
}

void *cross_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft, *fft2;
	t_cross *x = (t_cross *)object_alloc(cross_class);

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
	cross_init(x);
	return x;
}

void cross_init(t_cross *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	short initialized;
	
	x->x_obj.z_disabled = 1;
	initialized = fft->initialized;
	
	fftease_init(fft);
	fftease_init(fft2);

	if(!initialized){
		x->threshie = .001 ;
		x->autonorm = 0;
        x->mute = 0;
	//	x->last_channel = (float *) calloc((x->fft->N+2), sizeof(float));
	} 
	x->x_obj.z_disabled = 0;
}

void do_cross(t_cross *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int i;
	int N2 = fft->N2;
	double a1, b1, a2, b2;
	double *buffer1 = fft->buffer;
	double *buffer2 = fft2->buffer;
	double *channel1 = fft->channel;
	short autonorm = x->autonorm;
	int N = fft->N;
	double mult = fft->mult;
	int even, odd;
	double gainer;
	double threshie = x->threshie;
	double ingain = 0;
	double outgain, rescale;
	double mymult;
		
	fold(fft);		
	fold(fft2);	
	rdft(fft,1);
	rdft(fft2,1);
	
	/* changing algorithm for window flexibility */
	if(autonorm){
		ingain = 0;
		for(i = 0; i < N; i+=2){
			ingain += hypot(buffer1[i], buffer1[i+1]);
		}
	}
	
	for ( i = 0; i <= N2; i++ ) {
		odd = ( even = i<<1 ) + 1;
		
		a1 = ( i == N2 ? *(buffer1+1) : *(buffer1+even) );
		b1 = ( i == 0 || i == N2 ? 0. : *(buffer1+odd) );
		a2 = ( i == N2 ? *(buffer2+1) : *(buffer2+even) );
		b2 = ( i == 0 || i == N2 ? 0. : *(buffer2+odd) );
		gainer = hypot(a2, b2);
		if( gainer > threshie ) 
			*(channel1+even) = hypot( a1, b1 ) * gainer;
		*(channel1+odd) = -atan2( b1, a1 );
		*(buffer1+even) = *(channel1+even) * cos( *(channel1+odd) );
		if ( i != N2 )
			*(buffer1+odd) = -(*(channel1+even)) * sin( *(channel1+odd) );
		
	}
	if(autonorm){
		outgain = 0;
		for(i = 0; i < N; i+=2){
			outgain += hypot(buffer1[i], buffer1[i+1]);
		}
		if(ingain <= .0000001){
			// post("gain emergency!");
			rescale = 1.0;
		} else {
			rescale = ingain / outgain;
		} 
		// post("ingain %f outgain %f rescale %f",ingain, outgain, rescale);
		mymult = mult * rescale;
	}  else {
		mymult = mult;
	}
	
	rdft(fft, -1);
	overlapadd(fft);
}

void cross_perform64(t_cross *x, t_object *dsp64, double **ins,
                       long numins, double **outs,long numouts, long vectorsize,
                       long flags, void *userparam)
{
	int i, j;
	double *MSPInputVector1 = ins[0];
	double *MSPInputVector2 = ins[1];
	double *threshold = ins[2];
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
	}
	if (x->bypass) {
		for( j = 0; j < MSPVectorSize; j++) {
			*MSPOutputVector++ = *MSPInputVector1++ * FFTEASE_BYPASS_GAIN;
		}
	}
		
	if( x->thresh_connected ){	
		x->threshie = *threshold;
	}
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(inputOne + D, inputOne, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector1, inputOne + (Nw - D), D * sizeof(t_double));
        sysmem_copyptr(inputTwo + D, inputTwo, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector2, inputTwo + (Nw - D), D * sizeof(t_double));
		do_cross(x);
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
			do_cross(x);
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
			do_cross(x);
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}		
t_max_err get_fftsize(t_cross *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_cross *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		x->fft2->N = (int) val;
		cross_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_cross *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_cross *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		x->fft2->overlap = (int) val;
		cross_init(x);
	}
	return MAX_ERR_NONE;
}

void cross_dsp64(t_cross *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    x->thresh_connected = count[2];
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
		fft2->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft2);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
	}
	if(! fftease_msp_sanity_check(fft, OBJECT_NAME)){
		return;
	}
    if(reset_required){
        cross_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)){
        object_method(dsp64, gensym("dsp_add64"),x,cross_perform64,0,NULL);
    }
}


