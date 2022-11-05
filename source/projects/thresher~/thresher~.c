#include "fftease.h"

static t_class *thresher_class;

#define OBJECT_NAME "thresher~"

#define DEFAULT_HOLD (40.0)


typedef struct _thresher
{

	t_pxobject x_obj;
	t_fftease *fft;
	double move_threshold;
	double *composite_frame ;
	int *frames_left;
	int max_hold_frames;
	double max_hold_time;
	int first_frame;
	double damping_factor ;
	short thresh_connected;
	short damping_connected;
	short mute;
	short bypass;
	double tadv;
	long fftsize_attr;
	long overlap_attr;
} t_thresher;

void *thresher_new(t_symbol *s, int argc, t_atom *argv);
void thresher_dsp(t_thresher *x, t_signal **sp, short *count);
void thresher_assist(t_thresher *x, void *b, long m, long a, char *s);
void thresher_float(t_thresher *x, double f);
void thresher_mute(t_thresher *x, t_floatarg f);
void thresher_bypass(t_thresher *x, t_floatarg f);
void thresher_free( t_thresher *x );
void thresher_winfac(t_thresher *x, t_floatarg f);
void thresher_fftinfo(t_thresher *x);
void thresher_init(t_thresher *x);
void thresher_transpose(t_thresher *x, t_floatarg tf);
void thresher_synthresh(t_thresher *x, t_floatarg thresh);
void thresher_oscbank(t_thresher *x, t_floatarg flag);
t_max_err set_fftsize(t_thresher *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_thresher *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_thresher *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_thresher *x, void *attr, long *ac, t_atom **av);
void thresher_dsp64(t_thresher *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void thresher_perform64(t_thresher *x, t_object *dsp64, double **ins,
                    long numins, double **outs,long numouts, long vectorsize,
                    long flags, void *userparam);

int C74_EXPORT main(void)
{

	t_class *c;
	c = class_new("fftz.thresher~", (method)thresher_new, (method)thresher_free, sizeof(t_thresher),0,A_GIMME,0);
	
	class_addmethod(c,(method)thresher_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)thresher_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)thresher_mute,"mute",A_FLOAT,0);
	class_addmethod(c,(method)thresher_bypass,"bypass",A_FLOAT,0);
	class_addmethod(c,(method)thresher_winfac,"winfac",A_DEFFLOAT,0);
	class_addmethod(c,(method)thresher_fftinfo,"fftinfo",0);
	class_addmethod(c,(method)thresher_oscbank,"oscbank",A_FLOAT,0);
	class_addmethod(c,(method)thresher_transpose,"transpose",A_FLOAT,0);
	class_addmethod(c,(method)thresher_synthresh,"synthresh",A_FLOAT,0);

	class_addmethod(c,(method)thresher_float, "float",  A_FLOAT, 0);
	CLASS_ATTR_LONG(c, "fftsize", 0, t_thresher, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_LONG(c, "overlap", 0, t_thresher, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	thresher_class = c;
	
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}


void thresher_fftsize(t_thresher *x, t_floatarg f)
{
	x->fft->N = (int) f;
	thresher_init(x);
}

void thresher_transpose(t_thresher *x, t_floatarg tf)
{
	x->fft->P = tf;
}

void thresher_synthresh(t_thresher *x, t_floatarg thresh)
{
	x->fft->synt = thresh;
}

void thresher_oscbank(t_thresher *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}

void thresher_overlap(t_thresher *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	thresher_init(x);
}

void thresher_winfac(t_thresher *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	thresher_init(x);
}

void thresher_fftinfo(t_thresher *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}


void thresher_free(t_thresher *x){
	dsp_free( (t_pxobject *) x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
	sysmem_freeptr(x->composite_frame);
	
}
void thresher_mute(t_thresher *x, t_floatarg f){
	x->mute = (short)f;
}

void thresher_bypass(t_thresher *x, t_floatarg f){
	x->bypass = (short)f;
}

void thresher_assist (t_thresher *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal) Input");
				break;
			case 1:
				sprintf(dst,"(signal/float) Threshold");
				break;
			case 2:
				sprintf(dst,"(signal/float) Damping Factor");
				break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}


void *thresher_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_thresher *x = (t_thresher *)object_alloc(thresher_class);

	dsp_setup((t_pxobject *)x,3);
	outlet_new((t_pxobject *)x, "signal");

	x->fft = (t_fftease *) sysmem_newptr(sizeof(t_fftease) );
	fft = x->fft;
	fft->initialized = 0;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();	
	
	x->move_threshold = 0.001;
	x->damping_factor = 0.99;
	atom_arg_getdouble(&x->move_threshold , 0, argc, argv);
	atom_arg_getdouble(&x->damping_factor , 1, argc, argv);	
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;	
	
	attr_args_process(x, argc, argv);
	thresher_init(x);
	return x;
}

void thresher_init(t_thresher *x)
{
	t_fftease  *fft = x->fft;
	short initialized = fft->initialized;
	x->x_obj.z_disabled = 1;
	fftease_init(fft);
	x->tadv = (double) fft->D / (double) fft->R ;
	//post("tadv %f",x->tadv);
	if(!initialized){
		x->mute = 0;
		x->bypass = 0;
		if(!x->damping_factor){
			x->damping_factor = .95;
		}
		x->first_frame = 1;
		x->move_threshold = .00001 ;
		x->max_hold_time = DEFAULT_HOLD ;
		x->max_hold_frames = x->max_hold_time / x->tadv;
		x->composite_frame = (double *) sysmem_newptrclear( (fft->N+2) * sizeof(double));
		x->frames_left = (int *) sysmem_newptrclear( (fft->N+2) * sizeof(int) );
		
	} else if(initialized == 1){
		x->composite_frame = (double *) sysmem_resizeptrclear(x->composite_frame, (fft->N+2) * sizeof(double) );
		x->frames_left = (int *) sysmem_resizeptrclear(x->frames_left, (fft->N+2) * sizeof(int) );
	}
	x->x_obj.z_disabled = 0;
}

void do_thresher(t_thresher *x)
{
	int i;
	
	t_fftease *fft = x->fft;
	double *channel = fft->channel;
	double damping_factor = x->damping_factor;
	int max_hold_frames = x->max_hold_frames;
	int *frames_left = x->frames_left;
	double *composite_frame = x->composite_frame;
	int N = fft->N;
	double move_threshold = x->move_threshold;

	fold(fft);
	rdft(fft,FFT_FORWARD);
	convert(fft);
	
	if( x->first_frame ){
		for ( i = 0; i < N+2; i++ ){
			composite_frame[i] = channel[i];
			frames_left[i] = max_hold_frames;
		}
		x->first_frame = 0;
	} else {
		for( i = 0; i < N+2; i += 2 ){
			if(fabs( composite_frame[i] - channel[i] ) > move_threshold || frames_left[i] <= 0 ){
				composite_frame[i] = channel[i];
				composite_frame[i+1] = channel[i+1];
				frames_left[i] = max_hold_frames;
			} else {
				--(frames_left[i]);
				composite_frame[i] *= damping_factor;
			}
		}
	}
	// try memcpy here
	for ( i = 0; i < N+2; i++ ){
		channel[i] = composite_frame[i];
	}
	if(fft->obank_flag){
		oscbank(fft);
	} else {
		unconvert(fft);
		rdft(fft,FFT_INVERSE);
		overlapadd(fft);
	}	
}

void thresher_perform64(t_thresher *x, t_object *dsp64, double **ins,
                        long numins, double **outs,long numouts, long vectorsize,
                        long flags, void *userparam)
{
	int		    i,j;
	double *MSPInputVector = ins[0];
	double *inthresh = ins[1];
	double *damping = ins[2];
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
	
    if( x->thresh_connected ) {
		x->move_threshold = *inthresh;
    }
    if( x->damping_connected ) {
		x->damping_factor = *damping;
    }
    //do_thresher(x);
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_thresher(x);
        
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
            
			do_thresher(x);
			
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
            
			do_thresher(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}

void thresher_float(t_thresher *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	
	if (inlet == 1)
    {
		x->move_threshold = f;
    } else if (inlet == 2)  {
		x->damping_factor = f;
    }
}

t_max_err get_fftsize(t_thresher *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_thresher *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		thresher_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_thresher *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_thresher *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		thresher_init(x);
	}
	return MAX_ERR_NONE;
}

void thresher_dsp64(t_thresher *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    t_fftease *fft = x->fft;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
	x->thresh_connected = count[1];
	x->damping_connected = count[2];
	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
	}
    if(reset_required){
        thresher_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,thresher_perform64,0,NULL);
}


