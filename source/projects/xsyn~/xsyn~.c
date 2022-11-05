#include "fftease.h"

static t_class *xsyn_class;

#define OBJECT_NAME "xsyn~"

typedef struct _xsyn
{
	t_pxobject x_obj;
	t_fftease *fft;
	t_fftease *fft2;
	short mute;
	short bypass;
	long fftsize_attr;
	long overlap_attr;
} t_xsyn;

void *xsyn_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
void xsyn_assist(t_xsyn *x, void *b, long m, long a, char *s);
void xsyn_dsp_free( t_xsyn *x );
void xsyn_init(t_xsyn *x);
void xsyn_mute(t_xsyn *x, t_floatarg toggle);
void xsyn_bypass(t_xsyn *x, t_floatarg toggle);
void xsyn_fftinfo(t_xsyn *x);
// void xsyn_overlap(t_xsyn *x, t_floatarg f);
void xsyn_winfac(t_xsyn *x, t_floatarg f);
// void xsyn_fftsize(t_xsyn *x, t_floatarg f);
t_max_err set_fftsize(t_xsyn *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_xsyn *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_xsyn *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_xsyn *x, void *attr, long *ac, t_atom **av);
void xsyn_dsp64(t_xsyn *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void xsyn_perform64(t_xsyn *x, t_object *dsp64, double **ins,
                    long numins, double **outs,long numouts, long vectorsize,
                    long flags, void *userparam);

int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.xsyn~", (method)xsyn_new, (method)xsyn_dsp_free, sizeof(t_xsyn),0,A_GIMME,0);
	
	class_addmethod(c,(method)xsyn_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)xsyn_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)xsyn_mute,"mute",A_FLOAT,0);
	class_addmethod(c,(method)xsyn_bypass,"bypass", A_FLOAT, 0);
	class_addmethod(c,(method)xsyn_winfac,"winfac",A_DEFFLOAT,0);
    //	class_addmethod(c,(method)xsyn_fftsize,"fftsize",A_FLOAT,0);
	class_addmethod(c,(method)xsyn_fftinfo,"fftinfo",0);
    
	CLASS_ATTR_LONG(c, "fftsize", 0, t_xsyn, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");
	
	CLASS_ATTR_LONG(c, "overlap", 0, t_xsyn, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	xsyn_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}


void xsyn_dsp_free( t_xsyn *x )
{
	dsp_free( (t_pxobject *) x);
    
	fftease_free(x->fft);

     sysmem_freeptr(x->fft);
     
     fftease_free(x->fft2);
     sysmem_freeptr(x->fft2);
}

void xsyn_mute(t_xsyn *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void xsyn_bypass(t_xsyn *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}
/*
 void xsyn_fftsize(t_xsyn *x, t_floatarg f)
 {
 x->fft->N = (int) f;
 x->fft2->N = (int) f;
 xsyn_init(x);
 }
 
 */
/*
 void xsyn_overlap(t_xsyn *x, t_floatarg f)
 {
 x->fft->overlap = (int) f;
 x->fft2->overlap = (int) f;
 xsyn_init(x);
 }
 */
void xsyn_winfac(t_xsyn *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	x->fft2->winfac = (int) f;
	xsyn_init(x);
}

void xsyn_fftinfo(t_xsyn *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}


void xsyn_assist (t_xsyn *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal) Input 1 ");
				break;
			case 1:
				sprintf(dst,"(signal) Input 2 ");
				break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output ");
	}
}
/*
 void *xsyn_new(t_symbol *s, int argc, t_atom *argv)
 {
 t_fftease *fft, *fft2;
 t_xsyn *x = (t_xsyn *)object_alloc(xsyn_class);
 dsp_setup((t_pxobject *)x,2);
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
 xsyn_init(x);
 return x;
 }
 */
void *xsyn_new(t_symbol *s, int argc, t_atom *argv)
{
    //	t_fftease *fft, *fft2;
	t_xsyn *x = (t_xsyn *)object_alloc(xsyn_class);
	dsp_setup((t_pxobject *)x,2);
	outlet_new((t_pxobject *)x, "signal");
    
	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	x->fft2 = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	//fft = x->fft;
	//fft2 = x->fft2;
	x->fft->initialized = 0;
	x->fft2->initialized = 0;
	x->fft2->R = x->fft->R = sys_getsr();
	x->fft2->MSPVectorSize = x->fft->MSPVectorSize = sys_getblksize();
	x->fft->N = FFTEASE_DEFAULT_FFTSIZE;
	x->fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	x->fft->winfac = FFTEASE_DEFAULT_WINFAC;
	x->fft2->N = FFTEASE_DEFAULT_FFTSIZE;
	x->fft2->overlap = FFTEASE_DEFAULT_OVERLAP;
	x->fft2->winfac = FFTEASE_DEFAULT_WINFAC;
	
    /*post("pre arg process: N %d R %d overlap %d winfac %d vecsize %d",
     x->fft->N, x->fft->R, x->fft->overlap, x->fft->winfac,x->fft->MSPVectorSize );*/
	attr_args_process(x, argc, argv);
    /*post("post arg process: N %d R %d overlap %d winfac %d vecsize %d",
     x->fft->N, x->fft->R, x->fft->overlap, x->fft->winfac,x->fft->MSPVectorSize );*/
	xsyn_init(x);
    /*post("post xsyn init: N %d R %d overlap %d winfac %d vecsize %d",
     x->fft->N, x->fft->R, x->fft->overlap, x->fft->winfac,x->fft->MSPVectorSize );*/
    
	return x;
}

void xsyn_init(t_xsyn *x)
{
	short initialized = x->fft->initialized;
	x->x_obj.z_disabled = 1;
	fftease_init(x->fft);
	fftease_init(x->fft2);
    
	if(!initialized){
		x->mute = 0;
	}
	x->x_obj.z_disabled = 0;
}

void do_xsyn(t_xsyn *x)
{
    
    int i;
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    double *channel1 = fft->channel;
    double *channel2 = fft2->channel;
    int N = fft->N;
    double maxamp;
    
	fold(fft);
	fold(fft2);
    
	rdft(fft,FFT_FORWARD);
	rdft(fft2,FFT_FORWARD);
    
	leanconvert(fft);
	leanconvert(fft2);
	
	maxamp = 0;
    
	for( i = 0; i < N; i+= 2 ) {
		if( channel2[i] > maxamp ) {
			maxamp = channel2[i];
		}
	}
    
	if( maxamp >  0.000001 ){
		for( i = 0; i < N; i+= 2 ) {
			channel1[i] *= (channel2[i] / maxamp );
		}
	}
	  
	leanunconvert(fft);

	rdft(fft,FFT_INVERSE);
    
	overlapadd(fft);
}

void xsyn_perform64(t_xsyn *x, t_object *dsp64, double **ins,
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
		do_xsyn(x);
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
			do_xsyn(x);
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
			do_xsyn(x);
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}

t_max_err get_fftsize(t_xsyn *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_xsyn *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
        
		x->fft->N = (int) val;
		x->fft2->N = (int) val;
		xsyn_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_xsyn *x, void *attr, long *ac, t_atom **av)
{
	if (ac && av) {
		char alloc;
		
		if (atom_alloc(ac, av, &alloc)) {
			return MAX_ERR_GENERIC;
		}
		x->overlap_attr = x->fft->overlap;
        // post("got overlap from object structure: %d", x->overlap_attr);
		atom_setlong(*av, x->overlap_attr);
	}
	return MAX_ERR_NONE;
}


t_max_err set_overlap(t_xsyn *x, void *attr, long ac, t_atom *av)
{
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		x->fft2->overlap = (int) val;
        // post("setting overlap to: %d", val);
		xsyn_init(x);
	}
	return MAX_ERR_NONE;
}


void xsyn_dsp64(t_xsyn *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    
	if(!samplerate)
        return;
    
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
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
        xsyn_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,xsyn_perform64,0,NULL);
}


