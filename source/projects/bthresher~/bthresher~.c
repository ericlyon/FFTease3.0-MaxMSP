#include "fftease.h"

static t_class *bthresher_class;

#define OBJECT_NAME "bthresher~"

typedef struct _bthresher
{
	t_pxobject x_obj;
	t_fftease *fft;
	/* bthresher vars */
	double *move_threshold;
	double *composite_frame ;
	int *frames_left;
	int max_hold_frames;
	double max_hold_time;
	int first_frame;
	double *damping_factor ;
	double thresh_scalar;
	double damp_scalar;
	short thresh_connected;
	short damping_connected;
	void *list_outlet;
	void *misc_outlet;
	t_atom *list_data;
	short mute;
	short bypass;
	double init_thresh;
	double init_damping;
	double tadv;
	short inf_hold;
	// attributes
	long fftsize_attr;
	long overlap_attr;
} t_bthresher;

// will fix old memory calls

void *bthresher_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
void bthresher_perform(t_bthresher *x, t_object *dsp64, double **ins,
                       long numins, double **outs,long numouts, long vectorsize,
                       long flags, void *userparam);
void bthresher_dsp64(t_bthresher *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void bthresher_assist(t_bthresher *x, void *b, long m, long a, char *s);
void bthresher_float(t_bthresher *x, double f);
void bthresher_mute(t_bthresher *x, t_double f);
void bthresher_bypass(t_bthresher *x, t_double f);
//void bthresher_overlap(t_bthresher *x, t_double f);
void bthresher_winfac(t_bthresher *x, t_double f);
void bthresher_fftinfo(t_bthresher *x);
void bthresher_free(t_bthresher *x);
void bthresher_bin(t_bthresher *x, t_double bin_num, t_double threshold, t_double damper);
void bthresher_rdamper(t_bthresher *x, t_double min, t_double max );
void bthresher_rthreshold(t_bthresher *x, t_double min, t_double max);
void bthresher_dump(t_bthresher *x );
void bthresher_list (t_bthresher *x, t_symbol *msg, short argc, t_atom *argv);
void bthresher_init(t_bthresher *x);
double bthresher_boundrand(double min, double max);
void bthresher_allthresh(t_bthresher *x, t_double f);
void bthresher_alldamp(t_bthresher *x, t_double f);
void bthresher_inf_hold(t_bthresher *x, t_double f);
void bthresher_max_hold(t_bthresher *x, t_double f);
void do_bthresher(t_bthresher *x);
//void bthresher_fftsize(t_bthresher *x, t_double f);
void bthresher_oscbank(t_bthresher *x, t_double flag);
void bthresher_synthresh(t_bthresher *x, t_double thresh);
void bthresher_transpose(t_bthresher *x, t_double tf);
void bthresher_version(void);
t_max_err set_fftsize(t_bthresher *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_bthresher *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_bthresher *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_bthresher *x, void *attr, long *ac, t_atom **av);


int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.bthresher~", (method)bthresher_new, (method)bthresher_free, sizeof(t_bthresher), 0,A_GIMME, 0);

	class_addmethod(c,(method)bthresher_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)bthresher_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)bthresher_version,"version",0);
	class_addmethod(c,(method)bthresher_mute,"mute",A_FLOAT,0);
	class_addmethod(c,(method)bthresher_bypass,"bypass",A_FLOAT,0);
	class_addmethod(c,(method)bthresher_winfac,"winfac",A_FLOAT,0);
	class_addmethod(c,(method)bthresher_oscbank,"oscbank",A_FLOAT,0);
	class_addmethod(c,(method)bthresher_transpose,"transpose",A_FLOAT,0);
	class_addmethod(c,(method)bthresher_synthresh,"synthresh",A_FLOAT,0);
	class_addmethod(c,(method)bthresher_fftinfo,"fftinfo",0);
	class_addmethod(c,(method)bthresher_bin, "bin", A_FLOAT, A_FLOAT, A_FLOAT, 0);
	class_addmethod(c,(method)bthresher_rdamper, "rdamper", A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addmethod(c,(method)bthresher_rthreshold, "rthreshold", A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addmethod(c,(method)bthresher_dump,"dump",0);
	class_addmethod(c,(method)bthresher_list,"list",A_GIMME,0);
	class_addmethod(c,(method)bthresher_alldamp,"alldamp",A_FLOAT,0);
	class_addmethod(c,(method)bthresher_allthresh,"allthresh",A_FLOAT,0);
	class_addmethod(c,(method)bthresher_inf_hold,"inf_hold",A_FLOAT,0);
	class_addmethod(c,(method)bthresher_max_hold,"max_hold",A_FLOAT,0);
	class_addmethod(c,(method)bthresher_float,"float",A_FLOAT,0);

	CLASS_ATTR_LONG(c, "fftsize", 0, t_bthresher, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");
    
	CLASS_ATTR_LONG(c, "overlap", 0, t_bthresher, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	

	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	bthresher_class = c;
	
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}


t_max_err get_fftsize(t_bthresher *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_bthresher *x, void *attr, long ac, t_atom *av)
{
	if (ac && av) {
		long val = atom_getlong(av);
        val = fftease_fft_size(val);
		x->fft->N = (int) val;
		bthresher_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_bthresher *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_bthresher *x, void *attr, long ac, t_atom *av)
{
    int test_overlap;
	if (ac && av) {
		long val = atom_getlong(av);
        test_overlap = fftease_overlap(val);
        if(test_overlap > 0){
            x->fft->overlap = (int) val;
            bthresher_init(x);
        }
	}
	return MAX_ERR_NONE;
}


void bthresher_fftinfo( t_bthresher *x )
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void bthresher_free( t_bthresher *x ){
	dsp_free( (t_pxobject *) x);
	t_fftease *fft = x->fft;
	fftease_free(fft);
	/* external-specific memory */
    sysmem_freeptr(x->composite_frame);
    sysmem_freeptr(x->frames_left);
    sysmem_freeptr(x->move_threshold);
    sysmem_freeptr(x->damping_factor);
    sysmem_freeptr(x->list_data);
}

void bthresher_max_hold(t_bthresher *x, t_double f)
{
	if(f<=0)
		return;
	x->max_hold_time = f * .001;
	x->max_hold_frames = x->max_hold_time / x->tadv;
}

void bthresher_inf_hold(t_bthresher *x, t_double f)
{
	x->inf_hold = (int)f;
}

void bthresher_allthresh(t_bthresher *x, t_double f)
{
	int i;
	t_fftease *fft = x->fft;
    for(i=0;i < fft->N2+1;i++){
        x->move_threshold[i] = f;
    }
}

void bthresher_alldamp(t_bthresher *x, t_double f)
{
	int i;
	t_fftease *fft = x->fft;

    for(i=0;i < fft->N2+1;i++){
        x->damping_factor[i] = f;
    }
}


void bthresher_winfac(t_bthresher *x, t_double f)
{
	x->fft->winfac = (int) f;
	bthresher_init(x);
}

void bthresher_mute(t_bthresher *x, t_double f){
	x->mute = f;
}
void bthresher_bypass(t_bthresher *x, t_double f){
	x->bypass = f;
}

void bthresher_assist (t_bthresher *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:sprintf(dst,"(signal) Input");break;
			case 1:sprintf(dst,"(signal/double) Threshold Scalar");break;
			case 2:sprintf(dst,"(signal/double) Damping Factor Scalar");break;
		}
	} else if (msg==2) {
		switch (arg) {
			case 0:sprintf(dst,"(signal) Output");break;
			case 1:sprintf(dst,"(list) Current State");break;
		}
	}
}

void bthresher_list (t_bthresher *x, t_symbol *msg, short argc, t_atom *argv) {
	int i, bin, idiv;
	double fdiv;
	double *damping_factor = x->damping_factor;
	double *move_threshold = x->move_threshold;
	
	idiv = fdiv = (double) argc / 3.0 ;
	if( fdiv - idiv > 0.0 ) {
		post("list must be in triplets");
		return;
	}
	
	for( i = 0; i < argc; i += 3 ) {
		bin = atom_getintarg(i,argc,argv);
		damping_factor[bin] = atom_getfloatarg(i+1,argc,argv);
		move_threshold[bin] = atom_getfloatarg(i+2,argc,argv);
		
	}
}

void bthresher_dump (t_bthresher *x) {
	
	t_atom *list_data = x->list_data;
	double *damping_factor = x->damping_factor;
	double *move_threshold = x->move_threshold;
	
	int i,j, count;

	for( i = 0, j = 0; i < x->fft->N2 * 3 ; i += 3, j++ ) {

		atom_setlong(list_data+i,j);
		atom_setfloat(list_data+(i+1),damping_factor[j]);
		atom_setfloat(list_data+(i+2),move_threshold[j]);
	}
	
	count = x->fft->N2 * 3;
	outlet_list(x->list_outlet,0,count,list_data);
	
	return;
}

void *bthresher_new(t_symbol *s, int argc, t_atom *argv)
{
    t_fftease *fft;
	t_bthresher *x = (t_bthresher *)object_alloc(bthresher_class);
	x->list_outlet = listout((t_pxobject *)x);
	dsp_setup((t_pxobject *)x,3);
	outlet_new((t_pxobject *)x, "signal");
	x->x_obj.z_misc |= Z_NO_INPLACE;
	
	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	
	fft = x->fft;
	fft->initialized = 0;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();

	if( fft->R <= 0 || fft->R > 1000000 || fft->MSPVectorSize <= 0 ){
		error("%s is concerned that perhaps no audio driver has been loaded",OBJECT_NAME);
		return 0;
	}
	x->init_thresh = 0.1;
	x->init_damping = 0.99;

	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;

	attr_args_process(x, argc, argv);
    
    // need below line to avoid crash if no attributes set:
    bthresher_init(x);
	return x;
}

void bthresher_transpose(t_bthresher *x, t_double tf)
{
	x->fft->P = (double) tf;
}

void bthresher_synthresh(t_bthresher *x, t_double thresh)
{
	x->fft->synt = (double) thresh;
}

void bthresher_init(t_bthresher *x)
{
	int i; 
	t_fftease *fft = x->fft;
	x->x_obj.z_disabled = 1;

	short initialized = fft->initialized;
	fftease_init(fft);	
	
	if(!initialized){
		x->first_frame = 1;
		x->max_hold_time = 60.0 ;
		x->thresh_connected = 0;
		x->damping_connected = 0;
		x->thresh_scalar = 1;
		x->damp_scalar = 1;
		x->mute = 0;
		x->bypass = 0;
		x->inf_hold = 0;
        
		x->composite_frame = (double *) sysmem_newptrclear((fft->N + 2) * sizeof(double));
		x->frames_left = (int *) sysmem_newptrclear((fft->N + 2) * sizeof(int));
		
		// TRIPLETS OF bin# damp_factor threshold
		x->list_data = (t_atom *) sysmem_newptrclear(((fft->N2 + 1) * 3) * sizeof(t_atom));
		x->move_threshold = (double *) sysmem_newptrclear((fft->N2+1) * sizeof(double));
		x->damping_factor = (double *) sysmem_newptrclear((fft->N2+1) * sizeof(double));
	
		for(i = 0; i < fft->N2+1; i++) {
			x->move_threshold[i] = x->init_thresh;
			x->damping_factor[i] = x->init_damping;
		}
	} else {
        x->composite_frame = (double *) sysmem_resizeptrclear((void *)x->composite_frame, (fft->N + 2) * sizeof(double));
        x->frames_left = (int *) sysmem_resizeptrclear((void *)x->frames_left, (fft->N + 2) * sizeof(int));
		x->list_data = (t_atom *) sysmem_resizeptrclear((void *)x->list_data, (fft->N2 + 1) * 3 * sizeof(t_atom));
		x->move_threshold = (double *) sysmem_resizeptrclear((void *)x->move_threshold, (fft->N2+1) * sizeof(double));
		x->damping_factor = (double *) sysmem_resizeptrclear((void *)x->damping_factor, (fft->N2+1) * sizeof(double));
        for(i = 0; i < fft->N2+1; i++) {
            x->move_threshold[i] = x->init_thresh;
            x->damping_factor[i] = x->init_damping;
        }
	}

	x->tadv = (double) fft->D / (double) fft->R;
	x->max_hold_frames = x->max_hold_time / x->tadv;
	x->x_obj.z_disabled = 0;
	
}

void bthresher_version(void)
{
	fftease_version(OBJECT_NAME);
}

void bthresher_rdamper(t_bthresher *x,  t_double min, t_double max)
{
	int i;	
	
	for( i = 0; i < x->fft->N2; i++ ) {
		x->damping_factor[i] = bthresher_boundrand(min, max);
	}
}

void bthresher_rthreshold( t_bthresher *x,  t_double min, t_double max )
{
	int i;
	for( i = 0; i < x->fft->N2; i++ ) {
		x->move_threshold[i] = bthresher_boundrand(min, max);
	}
}


void bthresher_bin(t_bthresher *x, t_double bin_num, t_double damper, t_double threshold)
{
	int bn = (int) bin_num;
	if( bn >= 0 && bn < x->fft->N2 ){
		//    post("setting %d to %f %f",bn,threshold,damper);
		x->move_threshold[bn] = threshold;
		x->damping_factor[bn] = damper;
	} else {
		post("bthresher~: %d is out of range", bn);
	}
}


void do_bthresher(t_bthresher *x)
{
	t_fftease *fft = x->fft;

	int N = fft->N;

	double *channel = fft->channel;
	double *damping_factor = x->damping_factor;
	double *move_threshold = x->move_threshold;
	double *composite_frame = x->composite_frame;
	int max_hold_frames = x->max_hold_frames;
	int *frames_left = x->frames_left;
	double thresh_scalar = x->thresh_scalar;
	double damp_scalar = x->damp_scalar;
	short inf_hold = x->inf_hold;
	int i, j;
	
	fold(fft);
	rdft(fft,1);			
	convert(fft);	
	if( x->first_frame ){
		for ( i = 0; i < N+2; i++ ){
			composite_frame[i] = channel[i];
			x->frames_left[i] = max_hold_frames;
		}
		x->first_frame = 0;
	} 
	else {
		if( thresh_scalar < .999 || thresh_scalar > 1.001 || damp_scalar < .999 || damp_scalar > 1.001 ) {
			for(i = 0, j = 0; i < N+2; i += 2, j++ ){
				if( fabs( composite_frame[i] - channel[i] ) > move_threshold[j] * thresh_scalar|| frames_left[j] <= 0 ){
					composite_frame[i] = channel[i];
					composite_frame[i+1] = channel[i+1];
					frames_left[j] = max_hold_frames;
				} 
				else {
					if(!inf_hold){
						--(frames_left[j]);
					}
					composite_frame[i] *= damping_factor[j] * damp_scalar; /* denormals protection */
					if( composite_frame[i] < 0.000001 )
						composite_frame[i] = 0.0;
				}
			}
			
		} 
		else {
			for( i = 0, j = 0; i < N+2; i += 2, j++ ){
				if( fabs( composite_frame[i] - channel[i] ) > move_threshold[j] || frames_left[j] <= 0 ){
					composite_frame[i] = channel[i];
					composite_frame[i+1] = channel[i+1];
					frames_left[j] = max_hold_frames;
				} else {
					if(!inf_hold){
						--(frames_left[j]);
					}
					// composite_frame[i] *= damping_factor[j]; // was a bug here ??
					composite_frame[i] *= damping_factor[j] * damp_scalar;
					if( composite_frame[i] < 0.000001 )  /* denormals protection */
						composite_frame[i] = 0.0;
				}
			}
		}
	}
// use memcopy
	for(i = 0; i < N+2; i++){
		channel[i] = composite_frame[i];
	}
	if(fft->obank_flag){
		oscbank(fft);
	} else {
		unconvert(fft);
		rdft(fft,-1);
		overlapadd(fft);
	}
}

void bthresher_oscbank(t_bthresher *x, t_double flag)
{
	x->fft->obank_flag = (short) flag;
}

// t_int *bthresher_perform(t_int *w)

void bthresher_perform(t_bthresher *x, t_object *dsp64, double **ins,
                    long numins, double **outs,long numouts, long vectorsize,
                    long flags, void *userparam)
{	
	int	i,j;
	t_double *MSPInputVector = ins[0];
	t_double *inthresh = ins[1];
	t_double *damping = ins[2];
	t_double *MSPOutputVector = outs[0];
	t_fftease *fft = x->fft;
	
	int MSPVectorSize = fft->MSPVectorSize;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	double *internalInputVector = fft->internalInputVector;
	double *internalOutputVector = fft->internalOutputVector;	
	double *input = fft->input;
	double *output = fft->output;
	int Nw = fft->Nw;
	double mult = fft->mult;
	int D = fft->D;
	
	if(x->mute || x->x_obj.z_disabled) {
        for(i=0; i < vectorsize; i++){
            MSPOutputVector[i] = 0.0;
        }
        return;
	}  
	if (x->bypass) {
		for( j = 0; j < MSPVectorSize; j++) {
			*MSPOutputVector++ = *MSPInputVector++ * FFTEASE_BYPASS_GAIN;
		}
        return;
	} 

	if( x->thresh_connected ) {
		x->thresh_scalar = *inthresh;
	}
	if( x->damping_connected ) {
		x->damp_scalar = *damping;
	}

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){	
        // shift over samples in buffer
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        // shift in new samples from MSP
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_bthresher(x);
        
		// write out samples to MSP
		for ( j = 0; j < D; j++ ){
			*MSPOutputVector++ = output[j] * mult;
        }
        // shift samples in output buffer
        sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
        // zero out last part of buffer
        for(j = (Nw-D); j < Nw; j++){
            output[j] = 0.0;
        }
    }
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(MSPInputVector + (D*i), input + (Nw-D), D * sizeof(t_double));
            
			do_bthresher(x);
			
			for ( j = 0; j < D; j++ ){
				*MSPOutputVector++ = output[j] * mult;
			}
            sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){
                output[j] = 0.0;
            }
		}
	} 
	else if( fft->bufferStatus == BIGGER_THAN_MSP_VECTOR ) {
        sysmem_copyptr(MSPInputVector,internalInputVector + (operationCount * MSPVectorSize),MSPVectorSize * sizeof(t_double));
        sysmem_copyptr(internalOutputVector + (operationCount * MSPVectorSize),MSPOutputVector,MSPVectorSize * sizeof(t_double));
		operationCount = (operationCount + 1) % operationRepeat;
		
		if( operationCount == 0 ) {
			
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(internalInputVector, input + (Nw - D), D * sizeof(t_double));
            
			do_bthresher(x);
			
			for ( j = 0; j < D; j++ ){
				internalOutputVector[j] = output[j] * mult;
			}
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){
                output[j] = 0.0;
            }
		}
		fft->operationCount = operationCount;
	}    
}


void bthresher_float(t_bthresher *x, double f) // Look at doubles at inlets
{
	long inlet = proxy_getinlet((t_object*) x);
	if (inlet == 1)
    {
		x->thresh_scalar = f;
    } else if (inlet == 2)  {
		x->damp_scalar = f;
    }
}

void bthresher_dsp64(t_bthresher *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    // zero sample rate, die
    if(!samplerate)
        return;
	x->thresh_connected = count[1];
	x->damping_connected = count[2];
	t_fftease *fft = x->fft;
	fft->MSPVectorSize = maxvectorsize;
	fftease_set_fft_buffers(fft);
	if(fft->R != samplerate){
		fft->R = samplerate;
		bthresher_init(x);
	}
    object_method(dsp64, gensym("dsp_add64"),x,bthresher_perform,0,NULL);
}

double bthresher_boundrand( double min, double max) {
	double frand;
	frand = (double) (rand() % 32768)/ 32768.0;
	return (min + frand * (max-min) );
}


