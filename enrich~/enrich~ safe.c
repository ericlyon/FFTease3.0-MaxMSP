#include "fftease.h"

static t_class *enrich_class;

#define OBJECT_NAME "enrich~"

typedef struct _enrich
{
	t_pxobject x_obj;
	t_fftease *fft;
	t_symbol *buffername;
	t_buffer_ref *tablebuf_ref;
	double lofreq;
	double hifreq;
	int lo_bin;
	int hi_bin;
	double topfreq;
	int bypass;
	short connected[3];
	short mute;
	long fftsize_attr;
	long overlap_attr;
} t_enrich;

void *enrich_new(t_symbol *s, int argc, t_atom *argv);
void enrich_assist(t_enrich *x, void *b, long m, long a, char *s);
void enrich_bypass(t_enrich *x, t_floatarg state);
void enrich_float(t_enrich *x, double f);
void enrich_free(t_enrich *x);
void enrich_mute(t_enrich *x, t_floatarg tog);
void enrich_init(t_enrich *x);
void enrich_lowfreq(t_enrich *x, t_floatarg f);
void enrich_highfreq(t_enrich *x, t_floatarg f);
//void enrich_overlap(t_enrich *x, t_floatarg o);
void enrich_winfac(t_enrich *x, t_floatarg f);
void enrich_fftinfo(t_enrich *x);
void enrich_setbuf(t_enrich *x, t_symbol *newbufname);
//void enrich_fftsize(t_enrich *x, t_floatarg f);
t_max_err set_fftsize(t_enrich *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_enrich *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_enrich *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_enrich *x, void *attr, long *ac, t_atom **av);
void enrich_perform64(t_enrich *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam);
void enrich_dsp64(t_enrich *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);


int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.enrich~", (method)enrich_new, (method)enrich_free, sizeof(t_enrich),0,A_GIMME,0);
	
	class_addmethod(c,(method)enrich_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)enrich_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)enrich_bypass,"bypass",A_DEFFLOAT,0);
	class_addmethod(c,(method)enrich_mute,"mute",A_DEFFLOAT,0);
	class_addmethod(c,(method)enrich_lowfreq,"lowfreq",A_DEFFLOAT,0);
	class_addmethod(c,(method)enrich_highfreq,"highfreq",A_DEFFLOAT,0);
	class_addmethod(c,(method)enrich_fftinfo,"fftinfo",0);
//	class_addmethod(c,(method)enrich_overlap, "overlap",  A_DEFFLOAT, 0);
	class_addmethod(c,(method)enrich_winfac, "winfac",  A_DEFFLOAT, 0);
//	class_addmethod(c,(method)enrich_fftsize, "fftsize",  A_FLOAT, 0);
	class_addmethod(c,(method)enrich_setbuf, "setbuf",  A_SYM, 0);
//	class_addmethod(c,(method)enrich_bufinfo, "bufinfo", 0);
	class_addmethod(c,(method)enrich_float,"float",A_FLOAT,0);
	
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_enrich, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_enrich, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	enrich_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

void enrich_mute(t_enrich *x, t_floatarg tog)
{
	x->mute = (short)tog;
}

void enrich_fftsize(t_enrich *x, t_floatarg f)
{	
	t_fftease *fft = x->fft;
	fft->N = (int) f;
	enrich_init(x);
}

void enrich_overlap(t_enrich *x, t_floatarg f)
{

	x->fft->overlap = (int) f;
	enrich_init(x);
}

void enrich_winfac(t_enrich *x, t_floatarg f)
{

	x->fft->winfac = (int) f;
	enrich_init(x);
}

void enrich_fftinfo(t_enrich *x)
{
	fftease_fftinfo( x->fft, OBJECT_NAME );
}

void enrich_free(t_enrich *x ){
	dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
}

void enrich_assist (t_enrich *x, void *b, long msg, long arg, char *dst)
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

void enrich_highfreq(t_enrich *x, t_floatarg f)
{
	t_fftease *fft = x->fft;

	float curfreq;
	
	if(f < x->lofreq){
		error("current minimum is %f",x->lofreq);
		return;
	}
	if(f > fft->R/2 ){
		f = fft->R/2;
	}	
	x->hifreq = f;
	fft->hi_bin = 1;  
	curfreq = 0;
	while(curfreq < x->hifreq) {
		++(fft->hi_bin);
		curfreq += fft->c_fundamental;
	}
}

void enrich_lowfreq(t_enrich *x, t_floatarg f)
{
	t_fftease *fft = x->fft;
	float curfreq;
	
	if(f > x->hifreq){
		error("current maximum is %f",x->lofreq);
		return;
	}
	if(f < 0 ){
		f = 0;
	}	
	x->lofreq = f;
	fft->lo_bin = 0;  
	curfreq = 0;
	while( curfreq < x->lofreq ) {
		++(fft->lo_bin);
		curfreq += fft->c_fundamental ;
	}
}


void enrich_init(t_enrich *x)
{
	x->x_obj.z_disabled = 1;
	fftease_init(x->fft);
	fftease_oscbank_setbins(x->fft, x->lofreq, x->hifreq);	
	x->x_obj.z_disabled = 0;
}

void enrich_setbuf(t_enrich *x, t_symbol *newbufname)
{
	x->buffername = newbufname;
 //   buffer_ref_set(x->tablebuf_ref, x->buffername);
}

void *enrich_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_enrich *x = (t_enrich *)object_alloc(enrich_class);
	dsp_setup((t_pxobject *)x,3);
	outlet_new((t_pxobject *)x, "signal");

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();	
	fft->initialized = 0;
	x->lofreq = 0;
	x->hifreq = fft->R / 2.0;
	atom_arg_getsym(&x->buffername,0,argc,argv);
	atom_arg_getdouble(&x->lofreq,1,argc,argv);
	atom_arg_getdouble(&x->hifreq,2,argc,argv);

	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	attr_args_process(x, argc, argv);
	enrich_init(x);
	return x;
}

void do_enrich(t_enrich *x)
{
	t_fftease *fft = x->fft;
    fold(fft);   
    rdft(fft,1);
    convert(fft);
	oscbank(fft);
}

void enrich_perform64(t_enrich *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam)
{

	double *MSPInputVector = ins[0];
	double *transpose = ins[1];
	double *synt = ins[2];
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
	t_buffer_obj *dbuf;
    long b_nchans;
    long b_frames;
    float *b_samples;
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

    if(! x->tablebuf_ref){
        x->tablebuf_ref = buffer_ref_new((t_object*)x, x->buffername);
    } else {
        buffer_ref_set(x->tablebuf_ref, x->buffername);
    }
    dbuf = buffer_ref_getobject(x->tablebuf_ref);
    // bad buffer
    if(dbuf == NULL){
        post("enrich~: nonexistent buffer");
        return;
    }
    b_nchans = buffer_getchannelcount(dbuf);
    b_frames = buffer_getframecount(dbuf);
    
    // empty buffer
    if(b_frames < fft->L || b_nchans != 1){
        post("enrich~: table too small or not mono");
        return;
    }
    b_samples = buffer_locksamples(dbuf);
	//  user table goes from -1 to +1 so scale mult by N
	mult *= fft->N;
	
	// copy buffer to internal table (try more efficient means later)
	for(i = 0; i < fft->L; i++){
		fft->table[i] = b_samples[i];
	}

	if(connected[1]) {
		fft->P  = *transpose;
	}
	if (connected[2]) {
		fft->synt = *synt;
	}

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_enrich(x);
        
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
            
			do_enrich(x);
			
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
            
			do_enrich(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    buffer_unlocksamples(dbuf);
}

void enrich_bypass(t_enrich *x, t_floatarg state)
{
	x->bypass = state;	
}

void enrich_float(t_enrich *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	t_fftease *fft = x->fft;
	
	if (inlet == 1)
    {
		fft->P = (float)f;
    }
	else if (inlet == 2)
    {
		fft->synt = (float)f;
    }
	
}

t_max_err get_fftsize(t_enrich *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_enrich *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		enrich_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_enrich *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_enrich *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		enrich_init(x);
	}
	return MAX_ERR_NONE;
}

void enrich_dsp64(t_enrich *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    int i;
    t_fftease *fft = x->fft;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
	for(i = 0; i < 3; i++){
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
        enrich_init(x);
    }
    object_method(dsp64, gensym("dsp_add64"),x,enrich_perform64,0,NULL);
}


