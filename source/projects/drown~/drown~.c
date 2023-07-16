#include "fftease.h"

static t_class *drown_class;

#define OBJECT_NAME "drown~"

typedef struct _drown
{

	t_pxobject x_obj;
	t_fftease *fft;
	double drownmult;
	short mute;
	short bypass;
	double threshold;
	short connected[8];
	short peakflag;
	long fftsize_attr;
	long overlap_attr;
} t_drown;

void *drown_new(t_symbol *s, int argc, t_atom *argv);
void drown_mute(t_drown *x, t_floatarg toggle);
void drown_rel2peak(t_drown *x, t_floatarg toggle);
void drown_assist(t_drown *x, void *b, long m, long a, char *s);
void drown_float(t_drown *x, double f);
// void drown_overlap(t_drown *x, t_floatarg o);
void drown_free(t_drown *x);
void drown_init(t_drown *x);
//void drown_overlap(t_drown *x, t_floatarg f);
void drown_winfac(t_drown *x, t_floatarg f);
void drown_fftinfo(t_drown *x);
//void drown_fftsize(t_drown *x, t_floatarg f);
void drown_bypass(t_drown *x, t_floatarg toggle);
t_max_err set_fftsize(t_drown *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_drown *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_drown *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_drown *x, void *attr, long *ac, t_atom **av);
void drown_dsp64(t_drown *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void drown_perform64(t_drown *x, t_object *dsp64, double **ins,
                     long numins, double **outs,long numouts, long vectorsize,
                     long flags, void *userparam);

int C74_EXPORT main(void)
{
	
	t_class *c;
	c = class_new("fftz.drown~", (method)drown_new, (method)drown_free, sizeof(t_drown),0,A_GIMME,0);
	class_addmethod(c,(method)drown_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)drown_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)drown_mute,"mute",A_FLOAT,0);
	class_addmethod(c,(method)drown_bypass,"bypass", A_FLOAT, 0);
	class_addmethod(c,(method)drown_rel2peak,"rel2peak",A_FLOAT,0);
//	class_addmethod(c,(method)drown_fftsize,"fftsize",A_FLOAT,0);
//	class_addmethod(c,(method)drown_overlap,"overlap",A_FLOAT,0);
	class_addmethod(c,(method)drown_winfac,"winfac",A_FLOAT,0);
	class_addmethod(c,(method)drown_fftinfo,"fftinfo",0);
	class_addmethod(c,(method)drown_float,"float",A_FLOAT,0);
	CLASS_ATTR_LONG(c, "fftsize", 0, t_drown, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	CLASS_ATTR_LONG(c, "overlap", 0, t_drown, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	drown_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}


void drown_float(t_drown *x, double f) // Look at floats at inlets
{
	
	int inlet = x->x_obj.z_in;

	if (inlet == 1)
    {
		x->threshold = f;
    }
	if (inlet == 2)
    {
		x->drownmult = f;
    }
}

void drown_fftsize(t_drown *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	drown_init(x);
}

void drown_overlap(t_drown *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	drown_init(x);
}

void drown_winfac(t_drown *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	drown_init(x);
}

void drown_fftinfo(t_drown *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void drown_rel2peak(t_drown *x, t_floatarg toggle)
{
	x->peakflag = (short)toggle;
}

void drown_mute(t_drown *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void drown_bypass(t_drown *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void drown_assist (t_drown *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input"); break;
			case 1: sprintf(dst,"(signal/float) Threshold Generator"); break;
			case 2: sprintf(dst,"(signal/float) Multiplier for Weak Bins"); break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}

void *drown_new(t_symbol *s, int argc, t_atom *argv)
{
//	t_fftease *fft;

	t_drown *x = (t_drown *)object_alloc(drown_class);
	dsp_setup((t_pxobject *)x,3);
	outlet_new((t_pxobject *)x, "signal");
	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	//fft = x->fft;
	x->fft->R = sys_getsr();
	x->fft->MSPVectorSize = sys_getblksize();
	x->fft->initialized = 0;
	x->threshold = 0.001;
	x->drownmult = 0.1;
	x->mute = 0;
	x->peakflag = 1;

	x->fft->N = FFTEASE_DEFAULT_FFTSIZE;
	x->fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	x->fft->winfac = FFTEASE_DEFAULT_WINFAC;
    // drown_init(x);
    fftease_init(x->fft);
	attr_args_process(x, argc, argv);
	return x;
}

void drown_init(t_drown *x)
{

	x->x_obj.z_disabled = 1;
	// t_fftease  *fft = x->fft;
	fftease_init(x->fft);
	x->x_obj.z_disabled = 0;
}

void drown_free(t_drown *x)
{
	dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
}

void do_drown(t_drown *x)
{
	int i;
	t_fftease *fft = x->fft;
	double *channel = fft->channel;
	double threshold = x->threshold;
	double drownmult = x->drownmult;
	double frame_peak = 0.0, local_thresh;
	int N = fft->N;
	
	fold(fft);	
	rdft(fft,1);
	leanconvert(fft);
	if(x->peakflag){
		for(i = 0; i < N; i += 2){	
			if(frame_peak < channel[i])
				frame_peak = channel[i];
		}
		local_thresh = frame_peak * threshold;
	} else {
		local_thresh = threshold;
	}
	for(i = 0; i < N; i += 2){	
		if(channel[i] < local_thresh)
			channel[i]  *= drownmult;
	}  	
	leanunconvert(fft);
	rdft(fft,-1);
	overlapadd(fft);
}

void drown_perform64(t_drown *x, t_object *dsp64, double **ins,
                     long numins, double **outs,long numouts, long vectorsize,
                     long flags, void *userparam)
{
	int	i,j;
	double *MSPInputVector = ins[0];
	double *threshold = ins[1];
	double *drownmult = ins[2];
	double *MSPOutputVector = outs[0];
	t_fftease *fft = x->fft;
	int D = fft->D;
	int Nw = fft->Nw;
	double *input = fft->input;
	double *output = fft->output;
	double mult = fft->mult;
	int MSPVectorSize = fft->MSPVectorSize;
	double *internalInputVector = fft->internalInputVector;
	double *internalOutputVector = fft->internalOutputVector;		
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
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
	
	if(connected[1])
		x->threshold = *threshold;
	if(connected[2])
		x->drownmult = *drownmult;
    //do_drown(x);
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_drown(x);
        
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
            
			do_drown(x);
			
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
            
			do_drown(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}

t_max_err get_fftsize(t_drown *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_drown *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		drown_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_drown *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_drown *x, void *attr, long ac, t_atom *av)
{	
    int test_overlap;
    if (ac && av) {
        long val = atom_getlong(av);
        test_overlap = fftease_overlap(val);
        if(test_overlap > 0){
            x->fft->overlap = (int) val;
            drown_init(x);
        }
    }
    return MAX_ERR_NONE;
}


void drown_dsp64(t_drown *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    long i;
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
        drown_init(x);
    }
    object_method(dsp64, gensym("dsp_add64"),x,drown_perform64,0,NULL);
}

