#include "fftease.h"

static t_class *pileup_class;

#define OBJECT_NAME "pileup~"


typedef struct _pileup
{
	t_pxobject x_obj;
	t_fftease *fft;
	/* pileup vars */
	double move_threshold;
	double *last_frame ;
	int *frames_left;
	double inverse_compensation_gain; // gain up inverse
	double persistence; // decay factor
	int mode;
	double tadv;
	short mute;
	short bypass;
	double hi_freq;
	double lo_freq;
	long fftsize_attr;
	long overlap_attr;
} t_pileup;

void *pileup_new(t_symbol *s, int argc, t_atom *argv);
void pileup_dsp(t_pileup *x, t_signal **sp, short *count);
void pileup_assist(t_pileup *x, void *b, long m, long a, char *s);
void pileup_float(t_pileup *x, double f);
void pileup_mute(t_pileup *x, t_floatarg f);
void pileup_bypass(t_pileup *x, t_floatarg f);
void pileup_free( t_pileup *x );
void pileup_clear( t_pileup *x );
//void pileup_overlap(t_pileup *x, t_floatarg f);
void pileup_winfac(t_pileup *x, t_floatarg f);
void pileup_fftinfo(t_pileup *x);
void pileup_init(t_pileup *x);
void pileup_mode(t_pileup *x, t_floatarg mode);
void pileup_inverse_gain(t_pileup *x, t_floatarg gain);
void pileup_persistence(t_pileup *x, t_floatarg persistence);
//void pileup_fftsize(t_pileup *x, t_floatarg f);
void pileup_transpose(t_pileup *x, t_floatarg tf);
void pileup_synthresh(t_pileup *x, t_floatarg thresh);
void pileup_oscbank(t_pileup *x, t_floatarg flag);
void pileup_highfreq(t_pileup *x, t_floatarg f);
void pileup_lowfreq(t_pileup *x, t_floatarg f);
t_max_err set_fftsize(t_pileup *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_pileup *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_pileup *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_pileup *x, void *attr, long *ac, t_atom **av);
void pileup_dsp64(t_pileup *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void pileup_perform64(t_pileup *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam);

int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.pileup~", (method)pileup_new, (method)pileup_free, sizeof(t_pileup),0,A_GIMME,0);
	class_addmethod(c,(method)pileup_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)pileup_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)pileup_mute,"mute",A_FLOAT,0);
	class_addmethod(c,(method)pileup_bypass,"bypass",A_FLOAT,0);
	class_addmethod(c,(method)pileup_clear,"clear", 0);
	class_addmethod(c,(method)pileup_mode,"mode", A_FLOAT, 0);
	class_addmethod(c,(method)pileup_inverse_gain,"inverse_gain", A_FLOAT, 0);
	class_addmethod(c,(method)pileup_persistence,"persistence", A_FLOAT, 0);
//	class_addmethod(c,(method)pileup_overlap,"overlap",A_DEFFLOAT,0);
	class_addmethod(c,(method)pileup_winfac,"winfac",A_DEFFLOAT,0);
//	class_addmethod(c,(method)pileup_fftsize,"fftsize",A_FLOAT,0);
	class_addmethod(c,(method)pileup_fftinfo,"fftinfo",0);
	class_addmethod(c,(method)pileup_oscbank,"oscbank",A_FLOAT,0);
	class_addmethod(c,(method)pileup_transpose,"transpose",A_FLOAT,0);
	class_addmethod(c,(method)pileup_synthresh,"synthresh",A_FLOAT,0);
	class_addmethod(c,(method)pileup_lowfreq,"lowfreq",A_FLOAT,0);
	class_addmethod(c,(method)pileup_highfreq,"highfreq",A_FLOAT,0);
	class_addmethod(c,(method)pileup_float,"float",A_FLOAT,0);
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_pileup, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_pileup, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	pileup_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
	
}

void pileup_fftsize(t_pileup *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	pileup_init(x);
}


void pileup_highfreq(t_pileup *x, t_floatarg f)
{
	double curfreq;
	t_fftease *fft = x->fft;
	
	if(f < x->lo_freq){
		error("current minimum is %f",x->lo_freq);
		return;
	}
	if(f > fft->R/2 ){
		f = fft->R/2;
	}	
	x->hi_freq = f;
	fft->hi_bin = 1;  
	curfreq = 0;
	while(curfreq < x->hi_freq) {
		++(fft->hi_bin);
		curfreq += fft->c_fundamental;
	}
}

void pileup_lowfreq(t_pileup *x, t_floatarg f)
{
	double curfreq;
	t_fftease *fft = x->fft;
	
	if(f > x->hi_freq){
		error("current maximum is %f",x->lo_freq);
		return;
	}
	if(f < 0 ){
		f = 0;
	}	
	x->lo_freq = f;
	fft->lo_bin = 0;  
	curfreq = 0;
	while( curfreq < x->lo_freq ) {
		++(fft->lo_bin);
		curfreq += fft->c_fundamental ;
	}
}


void pileup_transpose(t_pileup *x, t_floatarg tf)
{
	x->fft->P = tf;
}

void pileup_synthresh(t_pileup *x, t_floatarg thresh)
{
	x->fft->synt = thresh;
}

void pileup_oscbank(t_pileup *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}

void pileup_overlap(t_pileup *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	pileup_init(x);
}

void pileup_winfac(t_pileup *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	pileup_init(x);
}

void pileup_fftinfo(t_pileup *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void pileup_persistence(t_pileup *x, t_floatarg persistence)
{
	x->persistence = persistence; 
}

void pileup_clear(t_pileup *x)
{
    x->last_frame = (double*)sysmem_resizeptrclear(x->last_frame,(x->fft->N+2) * sizeof(double));
}

void pileup_mode(t_pileup *x, t_floatarg mode)
{
	if( mode >= 0 && mode <= 3)
		x->mode = (int) mode;
}

void pileup_inverse_gain(t_pileup *x, t_floatarg gain)
{
	x->inverse_compensation_gain = gain;
}


void pileup_free(t_pileup *x){
	dsp_free( (t_pxobject *) x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
	sysmem_freeptr(x->last_frame);
	sysmem_freeptr(x->frames_left);
}
void pileup_mute(t_pileup *x, t_floatarg f){
	x->mute = (short)f;
}

void pileup_bypass(t_pileup *x, t_floatarg f){
	x->bypass = (short)f;
}

void pileup_assist (t_pileup *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input");break;
			case 1:sprintf(dst,"(signal/float) Threshold");break;
			case 2:sprintf(dst,"(signal/float) Damping Factor");break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}


void *pileup_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_pileup *x = (t_pileup *)object_alloc(pileup_class);
	dsp_setup((t_pxobject *)x,1);
	outlet_new((t_pxobject *)x, "signal");

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();	
		
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	attr_args_process(x, argc, argv);
	pileup_init(x);
	return x;
}

void pileup_init(t_pileup *x)
{
	t_fftease  *fft = x->fft;
	short initialized = fft->initialized;
	x->x_obj.z_disabled = 1;

	fftease_init(fft);
	
	if(!initialized){
		x->mode = 0;
		x->inverse_compensation_gain = 4.0;
		x->mute = 0;
		x->move_threshold = .00001 ;

		x->last_frame = (double *) sysmem_newptrclear((fft->N+2) * sizeof(double));
		x->frames_left = (int *) sysmem_newptrclear((fft->N+2) * sizeof(int));
	} else {
		x->last_frame = (double *) sysmem_resizeptrclear(x->last_frame,(fft->N+2)*sizeof(double));
		x->frames_left = (int *) sysmem_resizeptrclear(x->frames_left, (fft->N+2) * sizeof(int));
        x->fft->input = (double*) sysmem_resizeptrclear(fft->input, fft->Nw * sizeof(double));
        x->fft->output = (double*) sysmem_resizeptrclear(fft->output, fft->Nw * sizeof(double));
        x->fft->c_lastphase_in = (double*)sysmem_resizeptrclear(fft->c_lastphase_in, (fft->N2+1) * sizeof(double));
        x->fft->c_lastphase_out = (double*)sysmem_resizeptrclear(fft->c_lastphase_out, (fft->N2+1) * sizeof(double));
	}
	x->tadv = (double) fft->D / (double)fft->R ;
	x->x_obj.z_disabled = 0;
}
// no workee - need to figure out persistence algorithm with trailoff
void do_pileup(t_pileup *x)
{
	int i;
	t_fftease *fft = x->fft;
	double *last_frame = x->last_frame;
//	short trailoff = x->trailoff; // flag to start trailing
//	int trailframes = x->trailframes; // howmany frames to trail for
	double persistence = x->persistence; // decay factor
	int N = fft->N;
	double *channel = fft->channel;

	fold(fft);
	rdft(fft,FFT_FORWARD);
	convert(fft);
	
	if( x->mode == 0 ){
		for( i = 0; i < N; i += 2 ){
			if( fabs( channel[i] ) < last_frame[i]  ){ // fabs?
				channel[i] = last_frame[i];
				channel[i + 1] = last_frame[i + 1];
			} else {
				last_frame[i] = fabs( channel[i] );
				last_frame[i + 1] = channel[i + 1];
			}
		}
	}
	else if( x->mode == 1) {
		for( i = 0; i < N; i += 2 ){
			if( fabs( channel[i] ) < last_frame[i]  ){ // fabs?
				channel[i] = last_frame[i];
			} else {
				last_frame[i] = fabs( channel[i] );
			}
		}
	}
	else if( x->mode == 2 ){
		for( i = 0; i < N; i += 2 ){
			if( fabs( channel[i] ) > last_frame[i]  ){ // fabs?
				channel[i] = last_frame[i] * x->inverse_compensation_gain;
				channel[i + 1] = last_frame[i + 1];
			} else {
				last_frame[i] = fabs( channel[i] );
				last_frame[i + 1] = channel[i + 1];
			}
		}
	}	
	if( persistence < 1.0){
		for( i = 0; i < N; i += 2 ){
			last_frame[i] *= persistence;
		}
	}

	if(fft->obank_flag){
		oscbank(fft);
	} else {
	    unconvert(fft);
		rdft(fft,FFT_INVERSE);
		overlapadd(fft);
	}
}

void pileup_perform64(t_pileup *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam)
{
	int		    i,j;
	double *MSPInputVector = ins[0];
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
	if( fft->obank_flag )
		mult *= FFTEASE_OSCBANK_SCALAR;
				
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_pileup(x);
        
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
            
			do_pileup(x);
			
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
            
			do_pileup(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}

void pileup_float(t_pileup *x, double f) // Look at floats at inlets
{
	//	int inlet = x->x_obj.z_in;
	
}

t_max_err get_fftsize(t_pileup *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_pileup *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		pileup_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_pileup *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_pileup *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		pileup_init(x);
	}
	return MAX_ERR_NONE;
}



void pileup_dsp64(t_pileup *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    t_fftease *fft = x->fft;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;

	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
	}
    if(reset_required){
        pileup_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,pileup_perform64,0,NULL);
}


