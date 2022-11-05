#include "fftease.h"

static t_class *mindwarp_class;

#define OBJECT_NAME "mindwarp~"

#define MAX_WARP 16.0

/* 12.11.05 fixed divide-by-zero bug */

typedef struct _mindwarp
{
	t_pxobject x_obj;
	t_fftease *fft;
    int	warpConnected;
    int widthConnected;
    double warpFactor;
    double shapeWidth;
	double *newChannel;
    double *newAmplitudes;
	short connected[8];
	short mute;
	short bypass;
	long fftsize_attr;
	long overlap_attr;
} t_mindwarp;

void *mindwarp_new(t_symbol *s, int argc, t_atom *argv);
void mindwarp_float(t_mindwarp *x, double myFloat);
void mindwarp_assist(t_mindwarp *x, void *b, long m, long a, char *s);
void mindwarp_dest(t_mindwarp *x, double f);
void mindwarp_init(t_mindwarp *x);
void mindwarp_free(t_mindwarp *x);
void mindwarp_mute(t_mindwarp *x, t_floatarg toggle);
void mindwarp_fftinfo(t_mindwarp *x);
//void mindwarp_overlap(t_mindwarp *x, t_floatarg o);
void mindwarp_winfac(t_mindwarp *x, t_floatarg o);
//void mindwarp_fftsize(t_mindwarp *x, t_floatarg f);
void mindwarp_bypass(t_mindwarp *x, t_floatarg toggle);
void mindwarp_tilde_setup(void);
t_max_err set_fftsize(t_mindwarp *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_mindwarp *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_mindwarp *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_mindwarp *x, void *attr, long *ac, t_atom **av);
void mindwarp_dsp64(t_mindwarp *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void mindwarp_perform64(t_mindwarp *x, t_object *dsp64, double **ins,
                        long numins, double **outs,long numouts, long vectorsize,
                        long flags, void *userparam);

void mindwarp_float( t_mindwarp *x, double df )
{
	float myFloat = (float)df;
	
	int inlet = x->x_obj.z_in;
	
	//post("float input to mindwarp: %f",myFloat);
	
	if ( inlet == 1 ) {
		
		x->warpFactor = myFloat;
		
		if ( x->warpFactor > MAX_WARP )
			x->warpFactor = MAX_WARP;
		
		if ( x->warpFactor < (1. / MAX_WARP) )
			x->warpFactor = (1. / MAX_WARP);
//		post("warp factor %f", x->warpFactor);
	}	
	
	if ( inlet == 2 ) {
		
		if ( myFloat >= 1. && myFloat <= (double) x->fft->N )
			x->shapeWidth = myFloat;
//		post("shape width %f", x->shapeWidth);

	}
	
}

int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.mindwarp~", (method)mindwarp_new, (method)mindwarp_free, sizeof(t_mindwarp),0,A_GIMME,0);
    class_addmethod(c,(method)mindwarp_dsp64, "dsp64", A_CANT, 0);
    class_addmethod(c,(method)mindwarp_assist,"assist",A_CANT,0);
    class_addmethod(c,(method)mindwarp_mute,"mute", A_FLOAT, 0);
	class_addmethod(c,(method)mindwarp_bypass,"bypass", A_FLOAT, 0);
//	class_addmethod(c,(method)mindwarp_overlap,"overlap", A_FLOAT, 0);
	class_addmethod(c,(method)mindwarp_winfac,"winfac", A_FLOAT, 0);
//	class_addmethod(c,(method)mindwarp_fftsize,"fftsize", A_FLOAT, 0);
	class_addmethod(c,(method)mindwarp_fftinfo,"fftinfo", 0); 
	class_addmethod(c,(method)mindwarp_float,"float",A_FLOAT,0); 
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_mindwarp, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_mindwarp, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	mindwarp_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

/* diagnostic messages for Max */

void mindwarp_assist (t_mindwarp *x, void *b, long msg, long arg, char *dst)
{
	
	if (msg == 1) {
		
		switch (arg) {
				
			case 0:		sprintf(dst,"(signal) Formant Input");
				break;
				
			case 1:		sprintf(dst,"(signal/float) Warp Factor");
				break;
				
			case 2:		sprintf(dst,"(signal/float) Shape Width");
				break;		
		}
	}
	
	else {
		
		if (msg == 2)
			sprintf(dst,"(signal) Mindwarp Output");
		
	}
}
void mindwarp_version(void)
{
	fftease_version(OBJECT_NAME);
}

void *mindwarp_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_mindwarp *x = (t_mindwarp *)object_alloc(mindwarp_class);
	dsp_setup((t_pxobject *)x, 3);
	outlet_new((t_pxobject *)x, "signal");
	x->fft = (t_fftease *) malloc(sizeof(t_fftease));
	fft = x->fft;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();		
	
	/* args: warpfactor, shape width, overlap, window factor */
	
  	x->warpFactor = 1.0;
  	x->shapeWidth = 3.0;
	
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;

	attr_args_process(x, argc, argv);
	mindwarp_init(x);
	
	return x;
	
}

void mindwarp_init(t_mindwarp *x)
{
	x->x_obj.z_disabled = 1;
	short initialized = x->fft->initialized;
	
	fftease_init(x->fft);
	
	if(!initialized){
		x->mute = 0;
		x->bypass = 0;
		x->newAmplitudes = (double *)sysmem_newptrclear (((x->fft->N2 + 1) * 16)* sizeof(double));
		x->newChannel = (double *) sysmem_newptrclear ((x->fft->N + 1)* sizeof(double));
	}
    else if(initialized == 1) {
		x->newAmplitudes = (double *)sysmem_resizeptrclear(x->newAmplitudes, ((x->fft->N2 + 1) * 16) * sizeof(float));
		x->newChannel = (double *)sysmem_resizeptrclear (x->newChannel, (x->fft->N + 1) * sizeof(double));
	}
	x->x_obj.z_disabled = 0;
	
}

void mindwarp_free(t_mindwarp *x)
{

	dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
	sysmem_freeptr(x->newAmplitudes);
	sysmem_freeptr(x->newChannel);
	
}

void do_mindwarp(t_mindwarp *x)
{
	double *newChannel = x->newChannel;
	
	int		
	i,j,
	bindex,
	N,
	N2,
	Nw,
	shapeWidth = (int) x->shapeWidth,
	remainingWidth,
	newLength;
float
	cutoff,
	filterMult,

	interpIncr,
	interpPhase;
	double warpFactor;
	t_fftease *fft = x->fft;
	double *newAmplitudes = x->newAmplitudes;
//	float *bufferOne = fft->buffer;
	double *channelOne = fft->channel;
	
	N = fft->N;
	N2 = fft->N2;
	
	Nw = fft->Nw;
	warpFactor = x->warpFactor;
	
	cutoff = (double) N2 * .9;
	filterMult = .00001;
	
	fold(fft);
	rdft(fft,FFT_FORWARD);
	leanconvert(fft);
	
	if(warpFactor <= 0){
		error("bad warp, resetting");
		warpFactor = 1.0;
	}
	
	newLength = (int) ((double) N2 / warpFactor);
	
	if(newLength <= 0){
		error("bad length: resetting");
		newLength = 1.0;
	}
	
	interpIncr = (double) N2 / (double) newLength;
	
	interpPhase = 0.;
	
	
	// do simple linear interpolation on magnitudes
	
	for ( bindex=0; bindex < newLength; bindex++ ) {
		
		int		localbindex = ((int) interpPhase) << 1;
		
		double	lower = *(channelOne + localbindex),
		upper = *(channelOne + localbindex + 2),
		diff = interpPhase - ( (double) ( (int) interpPhase ) );
		
		*(newAmplitudes+bindex) = lower + ( ( upper - lower ) * diff );
		
		interpPhase += interpIncr;
	}
	
	
	
	// replace magnitudes with warped values 
	
	if (warpFactor > 1.) {
		
		int	until = (int) ( cutoff / warpFactor );
		
		for ( bindex=0; bindex < until; bindex++ ) {
			register int	amp = bindex<<1;
			
			*(newChannel+amp) = *(newAmplitudes+bindex);
		}
		
		
		// filter remaining spectrum as spectral envelope has shrunk 
		
		for ( bindex=until; bindex < N2; bindex++ ) {
			register int	amp = bindex<<1;
			
			*(newChannel+amp) *= filterMult;
		}
	}
	
	
	//OK
	
	// spectral envelope has enlarged, no post filtering is necessary
	
	else {
		
		for ( bindex=0; bindex <= N2; bindex++ ) {
			register int	amp = bindex<<1;
			
			*(newChannel+amp) = *(newAmplitudes+bindex);
		}
	}
	
	
	
	// constrain our shapeWidth value
	
	if ( shapeWidth > N2 )
		shapeWidth = N2;
  	
	if ( shapeWidth < 1 )
		shapeWidth = 1;
	
	// lets just shape the entire signal by the shape width 
	
	
	for ( i=0; i < N; i += shapeWidth << 1 ) {
		
		double       amplSum = 0.,
		freqSum = 0.,
		factor = 1.0;
		
		for ( j = 0; j < shapeWidth << 1; j += 2 ) {
			
			amplSum += *(newChannel+i+j);
			freqSum += *(channelOne+i+j);
		}
		
		if (amplSum < 0.000000001)
			factor = 0.000000001;	
		
		// this can happen, crashing external; now fixed.
		
		if( freqSum <= 0 ){
			//		error("bad freq sum, resetting");
			freqSum = 1.0;
		}
		else	
			factor = amplSum / freqSum;
		
		for ( j = 0; j < shapeWidth << 1; j += 2 )
			*(channelOne+i+j) *= factor;
	}
	
	// copy remaining magnitudes (fixed shadowed variable warning by renaming bindex)
	
	if ( (remainingWidth = N2 % shapeWidth) ) {
		
		int			lbindex = (N2 - remainingWidth) << 1;
		
		
		double       amplSum = 0.,
		freqSum = 0.,
		factor;
		
		for ( j = 0; j < remainingWidth << 1; j += 2 ) {
			
			amplSum += *(newChannel+lbindex+j);
			freqSum += *(channelOne+lbindex+j);
		}
		
		if (amplSum < 0.000000001)
			factor = 0.000000001;	
		
		else	
			factor = amplSum / freqSum;
		
		for ( j = 0; j < remainingWidth << 1; j += 2 )
			*(channelOne+bindex+j) *= factor;
	}

	leanunconvert(fft);

	rdft(fft,FFT_INVERSE);
	overlapadd(fft);
	
}

void mindwarp_perform64(t_mindwarp *x, t_object *dsp64, double **ins,
                        long numins, double **outs,long numouts, long vectorsize,
                        long flags, void *userparam)
{	
	double *MSPInputVector = ins[0];
	double *vec_warpFactor = ins[1];
	double *vec_shapeWidth = ins[2];
	double *MSPOutputVector = outs[0];
	t_fftease *fft = x->fft;
	
	int i, j;
	int D = fft->D;
	int Nw = fft->Nw;
	double mult = fft->mult;	
	double *input = fft->input;
	double *output = fft->output;
	int MSPVectorSize = fft->MSPVectorSize;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	double *internalInputVector = fft->internalInputVector;
	double *internalOutputVector = fft->internalOutputVector;	
	short *connected = x->connected;
	
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
	if( connected[1] ) x->warpFactor = *vec_warpFactor;
	if( connected[2] ) x->shapeWidth =  *vec_shapeWidth;
	
	if(x->warpFactor <= 0.0){
		x->warpFactor = 0.1;
		error("%s: zero warp factor is illegal",OBJECT_NAME);
	}
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_mindwarp(x);
        
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
            
			do_mindwarp(x);
			
			for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
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
            
			do_mindwarp(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}	
	
void mindwarp_bypass(t_mindwarp *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void mindwarp_mute(t_mindwarp *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void mindwarp_fftsize(t_mindwarp *x, t_floatarg f)
{	
	t_fftease *fft = x->fft;
	fft->N = (int) f;
	mindwarp_init(x);
}


void mindwarp_overlap(t_mindwarp *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	mindwarp_init(x);
}

void mindwarp_winfac(t_mindwarp *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	mindwarp_init(x);
}

void mindwarp_fftinfo( t_mindwarp *x )
{
	fftease_fftinfo( x->fft, OBJECT_NAME );
}

t_max_err get_fftsize(t_mindwarp *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_mindwarp *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		mindwarp_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_mindwarp *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_mindwarp *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		mindwarp_init(x);
	}
	return MAX_ERR_NONE;
}

void mindwarp_dsp64(t_mindwarp *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
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
        mindwarp_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,mindwarp_perform64,0,NULL);
}


