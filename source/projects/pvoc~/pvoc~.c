#include "fftease.h"

static t_class *pvoc_class;


#define OBJECT_NAME "pvoc~"

typedef struct _pvoc
{
	t_pxobject x_obj;
	t_fftease *fft;
	t_double lofreq;
	t_double hifreq;
	int lo_bin;
	int hi_bin;
	t_double topfreq;
	int bypass;
	int pitch_connected;
	int synt_connected;
	short mute;
	// attributes
	long fftsize_attr;
	long overlap_attr;
} t_pvoc;

void *pvoc_new(t_symbol *s, int argc, t_atom *argv);
void pvoc_assist(t_pvoc *x, void *b, long m, long a, char *s);
void pvoc_bypass(t_pvoc *x, t_floatarg state);
void pvoc_float(t_pvoc *x, double f);
void pvoc_free(t_pvoc *x);
void pvoc_mute(t_pvoc *x, t_floatarg tog);
void pvoc_init(t_pvoc *x);
void pvoc_winfac(t_pvoc *x, t_floatarg f);
void pvoc_fftinfo(t_pvoc *x);
//void pvoc_fftinfo(t_pvoc *x);
void pvoc_version(void);
t_max_err set_fftsize(t_pvoc *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_pvoc *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_pvoc *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_pvoc *x, void *attr, long *ac, t_atom **av);
t_max_err set_lowfreq(t_pvoc *x, void *attr, long ac, t_atom *av);
t_max_err set_highfreq(t_pvoc *x, void *attr, long ac, t_atom *av);
t_max_err get_lowfreq(t_pvoc *x, void *attr, long *ac, t_atom **av);
t_max_err get_highfreq(t_pvoc *x, void *attr, long *ac, t_atom **av);

void do_pvoc(t_pvoc *x, t_double *transp, t_double *synth_thresh );
void pvoc_perform64(t_pvoc *x, t_object *dsp64, double **ins,
                     long numins, double **outs,long numouts, long vectorsize,
                     long flags, void *userparam);
void pvoc_dsp64(t_pvoc *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);

int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.pvoc~", (method)pvoc_new, (method)pvoc_free, sizeof(t_pvoc), 
				  0,A_GIMME, 0);
	class_addmethod(c,(method)pvoc_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)pvoc_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)pvoc_bypass,"bypass",A_DEFFLOAT,0);
	class_addmethod(c,(method)pvoc_mute,"mute",A_DEFFLOAT,0);

	class_addmethod(c,(method)pvoc_fftinfo,"fftinfo",0);
	class_addmethod(c,(method)pvoc_winfac, "winfac",  A_DEFFLOAT, 0);
	class_addmethod(c,(method)pvoc_version, "version", 0);
	class_addmethod(c,(method)pvoc_float, "float",  A_FLOAT, 0);
	
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_pvoc, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_pvoc, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	
	CLASS_ATTR_FLOAT(c, "lowfreq", 0, t_pvoc, lofreq);
	CLASS_ATTR_ACCESSORS(c, "lowfreq", (method)get_lowfreq, (method)set_lowfreq);
	CLASS_ATTR_LABEL(c, "lowfreq", 0, "Minimum Frequency");	

	CLASS_ATTR_FLOAT(c, "highfreq", 0, t_pvoc, hifreq);
	CLASS_ATTR_ACCESSORS(c, "highfreq",(method)get_highfreq, (method)set_highfreq);
	CLASS_ATTR_LABEL(c, "highfreq", 0, "Maximum Frequency");	
	

	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	CLASS_ATTR_ORDER(c, "lowfreq",    0, "3");
	CLASS_ATTR_ORDER(c, "highfreq",    0, "4");

	class_dspinit(c);
	class_register(CLASS_BOX, c);
	pvoc_class = c;	
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

t_max_err get_fftsize(t_pvoc *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_pvoc *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		pvoc_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_pvoc *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_pvoc *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		pvoc_init(x);
	}
	return MAX_ERR_NONE;
}


t_max_err get_lowfreq(t_pvoc *x, void *attr, long *ac, t_atom **av)
{
	if (ac && av) {
		char alloc;
		if (atom_alloc(ac, av, &alloc)) {
			return MAX_ERR_GENERIC;
		}
		// x->overlap_attr = x->fft->overlap;
		atom_setlong(*av, x->lofreq);
	}
	return MAX_ERR_NONE;
}

t_max_err set_lowfreq(t_pvoc *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		t_fftease *fft = x->fft;
		float f = atom_getfloat(av);
		if(f > x->hifreq){
			error("%s: minimum cannot exceed current maximum: %f",OBJECT_NAME,x->hifreq);
			return MAX_ERR_NONE;
		}
		if(f < 0 ){
			f = 0;
		}	
		x->lofreq = f;
		fftease_oscbank_setbins(fft,x->lofreq, x->hifreq);
	}
	return MAX_ERR_NONE;
}

t_max_err get_highfreq(t_pvoc *x, void *attr, long *ac, t_atom **av)
{
	if (ac && av) {
		char alloc;
		if (atom_alloc(ac, av, &alloc)) {
			return MAX_ERR_GENERIC;
		}
		// x->overlap_attr = x->fft->overlap;
		atom_setlong(*av, x->hifreq);
	}
	return MAX_ERR_NONE;
}

t_max_err set_highfreq(t_pvoc *x, void *attr, long ac, t_atom *av)
{	
	if(! sys_getdspstate() ){
		post("blocking oscbank setbins, DACs are off");
		return MAX_ERR_NONE;
	}
	if (ac && av) {
		t_fftease *fft = x->fft;
		int R = fft->R;
		float f = atom_getfloat(av);
		if(f < x->lofreq){
			error("%s: maximum cannot go below current minimum: %f",OBJECT_NAME,x->lofreq);
			return MAX_ERR_NONE;
		}
		if(f > R/2 ){
			f = R/2;
		}	
		x->hifreq = f;
		// post("lowfreq %f hi freq %f",x->lofreq, x->hifreq);
		fftease_oscbank_setbins(fft,x->lofreq, x->hifreq);
	}
	return MAX_ERR_NONE;
}

void pvoc_version(void)
{
	fftease_version(OBJECT_NAME);
}

void pvoc_mute(t_pvoc *x, t_floatarg tog)
{
	x->mute = (short)tog;
    post("mute status: %d", x->mute);
}

void pvoc_winfac(t_pvoc *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	pvoc_init(x);
}

void pvoc_fftinfo(t_pvoc *x)
{
t_fftease *fft = x->fft;
	fftease_fftinfo( fft, OBJECT_NAME );	
}

void pvoc_free(t_pvoc *x ){
	dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
}

void pvoc_assist (t_pvoc *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input"); break;
			case 1: sprintf(dst,"(signal/float) Pitch Modification Factor"); break;
			case 2: sprintf(dst,"(signal/float) Synthesis Threshold"); break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}


void pvoc_init(t_pvoc *x)
{
	float curfreq;
	t_fftease *fft = x->fft;
	
	if(fft->initialized == -1){
		return;
	}
	
	x->x_obj.z_disabled = 1;
	
	fftease_init(fft);

	if( x->hifreq < fft->c_fundamental ) {
        post("default hi frequency of 10000 Hz");
		x->hifreq = 10000.0 ;
	}
	x->fft->hi_bin = 1;  
	curfreq = 0;
	while( curfreq < x->hifreq ) {
		++(x->fft->hi_bin);
		curfreq += fft->c_fundamental ;
	}
	
	x->fft->lo_bin = 0;  
	curfreq = 0;
	while( curfreq < x->lofreq ) {
		++(x->fft->lo_bin);
		curfreq += fft->c_fundamental;
	}
	x->x_obj.z_disabled = 0;
		
}

void *pvoc_new(t_symbol *s, int argc, t_atom *argv)
{
    t_fftease *fft;
	t_pvoc *x = (t_pvoc *)object_alloc(pvoc_class);
	dsp_setup((t_pxobject *)x,3);
	outlet_new((t_pxobject *)x, "signal");

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();
	fft->initialized = -1;// block init
	
	x->lofreq = 0;
	x->hifreq = 10000;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;


	x->mute = 0;
	x->bypass = 0;		
	if(x->lofreq <0 || x->lofreq>= fft->R / 2)
		x->lofreq = 0;
	if(x->hifreq <50 || x->hifreq >= fft->R / 2)
		x->hifreq = 4000;

	attr_args_process(x, argc, argv);
	fft->initialized = 0;// prepare for init in DSP routine
	return x;
}


void do_pvoc(t_pvoc *x, t_double *transp, t_double *synth_thresh )
{
	t_fftease *fft = x->fft;
    fold(fft);   
	rdft(fft, 1);
	convert(fft);
	oscbank(fft);	
}


void pvoc_perform64(t_pvoc *x, t_object *dsp64, double **ins,
    long numins, double **outs,long numouts, long vectorsize,
    long flags, void *userparam)
{
    
	t_double *MSPInputVector = ins[0];
	t_double *transp = ins[1];
	t_double *synth_thresh = ins[2];
	t_double *MSPOutputVector = outs[0];
    
	t_fftease *fft = x->fft;
	int i, j;
	int D = fft->D;
	int Nw = fft->Nw;
	t_double mult = fft->mult;
	t_double *input = fft->input;
	t_double *output = fft->output;
	int MSPVectorSize = fft->MSPVectorSize;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	t_double *internalInputVector = fft->internalInputVector;
	t_double *internalOutputVector = fft->internalOutputVector;
	
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
	if(x->pitch_connected) {
		fft->P  = *transp;
		fft->pitch_increment = fft->P*fft->L/fft->R;
	}
	if (x->synt_connected) {
		fft->synt = *synth_thresh;
	}
	
	// HERE IS THE GOOD STUFF
	
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_pvoc( x, transp, synth_thresh );
        
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
            
			do_pvoc( x, transp, synth_thresh );
			
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

			do_pvoc( x, transp, synth_thresh );
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}

void pvoc_bypass(t_pvoc *x, t_floatarg state)
{
	x->bypass = state;	
}


void pvoc_float(t_pvoc *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	t_fftease *fft = x->fft;
	
	if (inlet == 1)
    {
		fft->P = (float)f;
		fft->pitch_increment = fft->P*fft->L/fft->R;
    }
	else if (inlet == 2)
    {
		fft->synt = (float)f;
    }
	
}

void pvoc_dsp64(t_pvoc *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    t_fftease *fft = x->fft;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
	x->pitch_connected = count[1];
	x->synt_connected = count[2];
	
	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
	}
    if(reset_required){
        pvoc_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,pvoc_perform64,0,NULL);
}

