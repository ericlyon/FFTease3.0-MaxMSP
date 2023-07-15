#include "fftease.h"

static t_class *cavoc27_class;

#define OBJECT_NAME "cavoc27~"

/* NOTE THIS IS A MORE COMPLEX CA WITH 3 DIFFERENT STATES  */

typedef struct _cavoc27
{
	t_pxobject x_obj;
	t_fftease *fft;
	double *ichannel; //for interpolation
	double *tmpchannel; // for spectrum capture
	double frame_duration;
	int max_bin;
	double fundamental;
	double *last_frame;
	short left;
	short right;
	short center;
	short *rule;
	
	double start_breakpoint;
	int hold_frames;
	int frames_left;
	int set_count;
	short interpolate_flag;
	short capture_flag;
	short capture_lock;
	void *list_outlet;
	t_atom *list_data;
	short mute;
	
	short external_trigger;
	short trigger_value; // set to 1 when a bang is received
	double topfreq; // highest to synthesize - Nyquist by default
	double bottomfreq;
	double *freqs;
	double *amps;
	short manual_mode; // respond to outside
	short freeze; // flag to maintain current spectrum
	long fftsize_attr;
	long overlap_attr;
    double density;
    double hold_time; //hold time in ms
} t_cavoc27;

void *cavoc27_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
void cavoc27_assist(t_cavoc27 *x, void *b, long m, long a, char *s);
void cavoc27_free( t_cavoc27 *x);
int cavoc27_apply_rule( short left, short right, short center, short *rule);
void cavoc27_rule (t_cavoc27 *x, t_symbol *msg, short argc, t_atom *argv);
void cavoc27_interpolate (t_cavoc27 *x, t_floatarg interpolate);
void cavoc27_capture_spectrum (t_cavoc27 *x, t_floatarg flag );
void cavoc27_capture_lock (t_cavoc27 *x, t_floatarg toggle );
void cavoc27_retune (t_cavoc27 *x, t_floatarg min, t_floatarg max);
void cavoc27_mute (t_cavoc27 *x, t_floatarg toggle);
void cavoc27_init(t_cavoc27 *x);
void cavoc27_rand_set_spectrum(t_cavoc27 *x);
void cavoc27_rand_set_rule(t_cavoc27 *x);
void cavoc27_fftinfo(t_cavoc27 *x);
void cavoc27_winfac(t_cavoc27 *x, t_floatarg f);
void cavoc27_oscbank(t_cavoc27 *x, t_floatarg flag);
void cavoc27_transpose (t_cavoc27 *x, t_floatarg pfac);
void cavoc27_noalias(t_cavoc27 *x, t_floatarg flag);
void cavoc27_manual(t_cavoc27 *x, t_floatarg tog);
void cavoc27_trigger(t_cavoc27 *x);
void cavoc27_freeze(t_cavoc27 *x, t_floatarg tog);
void cavoc27_version(void);
t_max_err set_fftsize(t_cavoc27 *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_cavoc27 *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_cavoc27 *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_cavoc27 *x, void *attr, long *ac, t_atom **av);
t_max_err get_density(t_cavoc27 *x, void *attr, long *ac, t_atom **av);
t_max_err set_density(t_cavoc27 *x, void *attr, long ac, t_atom *av);
t_max_err get_holdtime(t_cavoc27 *x, void *attr, long *ac, t_atom **av);
t_max_err set_holdtime(t_cavoc27 *x, void *attr, long ac, t_atom *av);

void cavoc27_perform64(t_cavoc27 *x, t_object *dsp64, double **ins,
                         long numins, double **outs,long numouts, long vectorsize,
                         long flags, void *userparam);
void cavoc27_dsp64(t_cavoc27 *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);

int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.cavoc27~", (method)cavoc27_new, (method)cavoc27_free, sizeof(t_cavoc27),0,A_GIMME,0);
	class_addmethod(c,(method)cavoc27_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)cavoc27_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)cavoc27_rule,"rule",A_GIMME,0);
	class_addmethod(c,(method)cavoc27_interpolate,"interpolate",A_FLOAT,0);
	class_addmethod(c,(method)cavoc27_retune,"retune",A_FLOAT,A_FLOAT,0);
	class_addmethod(c,(method)cavoc27_capture_spectrum,"capture_spectrum",A_FLOAT,0);
	class_addmethod(c,(method)cavoc27_mute,"mute",A_FLOAT,0);
	class_addmethod(c,(method)cavoc27_noalias,"noalias",A_FLOAT,0);
	class_addmethod(c,(method)cavoc27_manual,"manual",A_FLOAT,0);
	class_addmethod(c,(method)cavoc27_freeze,"freeze",A_FLOAT,0);
	class_addmethod(c,(method)cavoc27_trigger,"trigger",0);
	// common calls
	class_addmethod(c,(method)cavoc27_winfac,"winfac",A_FLOAT,0);
	class_addmethod(c,(method)cavoc27_fftinfo,"fftinfo",0);
	class_addmethod(c,(method)cavoc27_oscbank,"oscbank",A_FLOAT,0);
	class_addmethod(c,(method)cavoc27_transpose,"transpose",A_FLOAT,0);
    
	CLASS_ATTR_LONG(c, "fftsize", 0, t_cavoc27, fftsize_attr);
//	CLASS_ATTR_DEFAULT_SAVE(c, "fftsize", 0, "1024");
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_LONG(c, "overlap", 0, t_cavoc27, overlap_attr);
//	CLASS_ATTR_DEFAULT_SAVE(c, "overlap", 0, "8");
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	

	CLASS_ATTR_DOUBLE(c, "density", 0, t_cavoc27, density);
	CLASS_ATTR_ACCESSORS(c, "density", (method)get_density, (method)set_density);
	CLASS_ATTR_LABEL(c, "density", 0, "Density");
    
 	CLASS_ATTR_DOUBLE(c, "holdtime", 0, t_cavoc27, hold_time);
	CLASS_ATTR_ACCESSORS(c, "holdtime", (method)get_holdtime, (method)set_holdtime);
	CLASS_ATTR_LABEL(c, "holdtime", 0, "Hold Time");

    
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
    CLASS_ATTR_ORDER(c, "density",    0, "3");
    CLASS_ATTR_ORDER(c, "holdtime",    0, "4");
    
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	cavoc27_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

void cavoc27_rand_set_rule(t_cavoc27 *x)
{
	int i;
	float rval;
	for( i = 0; i < 27; i++ ){
		rval = fftease_randf(0.0,1.0);
		if( rval < .333 )
			x->rule[i] = 0;
		else if(rval < .666 )
			x->rule[i] = 1;
		else x->rule[i] = 2;
	}
}

void cavoc27_freeze(t_cavoc27 *x, t_floatarg tog)
{
	x->freeze = (short) tog;
}

void cavoc27_manual(t_cavoc27 *x, t_floatarg tog)
{
	x->manual_mode = (short) tog;
}
void cavoc27_trigger(t_cavoc27 *x)
{
	x->external_trigger = 1;
}


void cavoc27_retune(t_cavoc27 *x, t_floatarg min, t_floatarg max)
{
	int i;
	t_fftease *fft = x->fft;
	double *tmpchannel = x->tmpchannel;
	double *last_frame = x->last_frame;
	if( max <= 0 || min <= 0 || min > max ){
		error("bad values for min and max multipliers");
		return;
	}
    if( min < .1 ){
        min = 0.1;
    }
    if( max > 2.0 ){
        max = 2.0;
    }
	for( i = 1; i < fft->N + 1; i += 2 ){
		 last_frame[i] = tmpchannel[i] = fft->c_fundamental * (float) (i / 2) * fftease_randf(min, max);
	}
	
}

void cavoc27_transpose (t_cavoc27 *x, t_floatarg pfac)
{
	t_fftease *fft = x->fft;
	fft->P = (float) pfac;
	fft->pitch_increment = fft->P*fft->L/fft->R;
}

void cavoc27_mute (t_cavoc27 *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}


void cavoc27_interpolate(t_cavoc27 *x, t_floatarg flag)
{
	x->interpolate_flag = (short) flag;
}

void cavoc27_capture_spectrum(t_cavoc27 *x, t_floatarg flag )
{
	x->capture_lock = (short)flag; 
}

void cavoc27_capture_lock(t_cavoc27 *x, t_floatarg flag )
{
	x->capture_lock = (short)flag; 
}

void cavoc27_rule (t_cavoc27 *x, t_symbol *msg, short argc, t_atom *argv)
{
	int i;
	short *rule = x->rule;
	if( argc != 27 ){
		error("the rule must be size 18");
		return;
	}
	
	for( i = 0; i < 27; i++ ){
		rule[i] = (short) atom_getfloatarg( i, argc, argv);
	}
}

void cavoc27_free( t_cavoc27 *x ){
	dsp_free( (t_pxobject *) x);
    fftease_free(x->fft);
//    sysmem_freeptr(x->fft);
	sysmem_freeptr(x->ichannel);
	sysmem_freeptr(x->tmpchannel);
	sysmem_freeptr(x->last_frame);
	sysmem_freeptr(x->rule);
}

void cavoc27_assist (t_cavoc27 *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"unused(signal)");
				break;
		}
	} else if (msg==2) {
		switch (arg) {
			case 0:
				sprintf(dst,"output(signal)"); break;
		}
	}
}

void *cavoc27_new(t_symbol *s, int argc, t_atom *argv)
{
t_fftease *fft;

	t_cavoc27 *x = (t_cavoc27 *)object_alloc(cavoc27_class);
	dsp_setup((t_pxobject *)x,1);
	outlet_new((t_pxobject *)x, "signal");

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();
	fft->initialized = 0;
	x->hold_time = 1000.0;
	x->density = 0.1;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	x->freeze = 0;
	x->start_breakpoint = 1.0 - x->density;
	if(!x->hold_time)
		x->hold_time = 0.15;
	fft->obank_flag = 0;
    cavoc27_init(x);
	attr_args_process(x, argc, argv);
	
	return x;
}

void cavoc27_noalias(t_cavoc27 *x, t_floatarg flag)
{
	x->fft->noalias = (short) flag;
}

void cavoc27_oscbank(t_cavoc27 *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}

void cavoc27_fftsize(t_cavoc27 *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	cavoc27_init(x);
}

void cavoc27_overlap(t_cavoc27 *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	cavoc27_init(x);
}

void cavoc27_winfac(t_cavoc27 *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	cavoc27_init(x);
}

void cavoc27_fftinfo(t_cavoc27 *x)
{
	fftease_fftinfo( x->fft, OBJECT_NAME );	
	post("frames left %d", x->frames_left);
	post("no alias: %d", x->fft->noalias);
}


void cavoc27_init(t_cavoc27 *x)
{
	int i;
	t_fftease *fft = x->fft;
	x->x_obj.z_disabled = 1;	
	short initialized = fft->initialized;
	fftease_init(fft);
    fft->lo_bin = 0;
    fft->hi_bin = fft->N2 - 1;
	if(! fft->R ){
		error("cavoc27~: zero sampling rate!");
		return;
	}
	x->frame_duration = (float)fft->D/(float) fft->R;
    if(x->hold_time <= 0.0){
        x->hold_time = 150.0;
    }
	x->hold_frames = (int) ((x->hold_time * 0.001) / x->frame_duration);
	x->frames_left = x->hold_frames;
	x->trigger_value = 0;
	x->set_count = 0;
    x->topfreq = fft->R / 2.0;
    x->bottomfreq = 0.0;
    
	if(!initialized){
		srand(time(0));
		x->interpolate_flag = 0;
		x->capture_lock = 0;
		x->mute = 0;

		x->ichannel = (double *) sysmem_newptrclear((fft->N+2) * sizeof(double));
		x->tmpchannel = (double *) sysmem_newptrclear((fft->N+2) * sizeof(double));
		x->last_frame = (double *) sysmem_newptrclear((fft->N+2) * sizeof(double));
		x->rule = (short *) sysmem_newptrclear(27 * sizeof(short));
        cavoc27_rand_set_rule(x);
        cavoc27_rand_set_spectrum(x);
	} else {
		x->ichannel = (double *)sysmem_resizeptrclear((void *)x->ichannel,(fft->N+2)*sizeof(double));
		x->tmpchannel = (double *)sysmem_resizeptrclear((void *)x->tmpchannel,(fft->N+2)*sizeof(double));
		x->last_frame = (double *)sysmem_resizeptrclear((void *)x->last_frame,(fft->N+2)*sizeof(double));
	}

	for( i = 0; i < fft->N+2; i++ ){
		x->last_frame[i] = fft->channel[i];
	}
	x->x_obj.z_disabled = 0;
}

void cavoc27_rand_set_spectrum(t_cavoc27 *x)
{
	int i;
	float rval;
	t_fftease *fft = x->fft;
	double *channel = x->tmpchannel;
	//set spectrum

	for( i = 0; i < fft->N2; i++ ){
		if( fftease_randf(0.0, 1.0) > x->start_breakpoint){
			rval = fftease_randf(0.0, 1.0);
			if( rval < 0.5 ){
				channel[ i * 2 ] = 1;
			}
			else {
				channel[ i * 2 ] = 2;
			}
			++(x->set_count);
		} else {
			channel[ i * 2 ] = 0;
		}
		channel[ i * 2 + 1 ] = fft->c_fundamental * (float) i * fftease_randf(.9,1.1);
	}
}

void do_cavoc27(t_cavoc27 *x)
{
	t_fftease *fft = x->fft;
	int i;
	int frames_left = x->frames_left;

	int N = fft->N;

	double *tmpchannel = x->tmpchannel;
	double *ichannel = x->ichannel;

	int hold_frames = x->hold_frames;
	short *rule = x->rule;
	short left = x->left;
	short right = x->right;
	short center = x->center;
	double *last_frame = x->last_frame;
	double frak;
	short manual_mode = x->manual_mode;
	short trigger;
	short interpolate_flag = x->interpolate_flag;
	double *channel = fft->channel;
	
	if( manual_mode ){
		trigger = x->external_trigger;
	} else {
		trigger = 0;
	}
	
	if( x->capture_flag || (x->capture_lock && ! x->freeze)) {
		
		fold(fft);
		rdft(fft,1);
		convert(fft);
		for( i = 1; i < fft->N+1; i += 2){
			tmpchannel[i] = channel[i];
		}
	}
	if( ! manual_mode ){
		if( --frames_left <= 0 ){
			trigger = 1;
		}
	}
	if(trigger && ! x->freeze){
		for( i = 0; i < fft->N+1; i++ ){
			last_frame[i] = tmpchannel[i];
		}
		frames_left = hold_frames;
		for( i = 2; i < fft->N; i+=2 ){
			left = last_frame[ i - 2 ];
			center = last_frame[i] ;
			right = last_frame[i+2];
			tmpchannel[i] = cavoc27_apply_rule(left, right, center, rule );
		}
		// boundary cases 
		center = last_frame[0];
		right = last_frame[2];
		left = last_frame[N];
		tmpchannel[0] = cavoc27_apply_rule(left, right, center, rule );
		
		center = last_frame[N];
		right = last_frame[0];
		left = last_frame[N - 2];
		tmpchannel[N] = cavoc27_apply_rule(left, right, center, rule );
		x->external_trigger = trigger = 0 ;
	}
	if( interpolate_flag && ! x->freeze){
		frak = 1.0 - ((float) frames_left / (float) hold_frames);

		for( i = 0; i <N+2; i += 2 ){
			ichannel[i] = last_frame[i] + frak * ( tmpchannel[i] - last_frame[i] ); 
			ichannel[i+1] = last_frame[i+1];
		}	
		for( i = 0; i < N+2; i++ ){
			channel[i] = ichannel[i];
		}	
	} else {
		for( i = 0; i < N+2; i++){
			channel[i] = tmpchannel[i];
		}
	}	
	if(x->freeze){
		for( i = 0; i < N+2; i++){
			channel[i] = tmpchannel[i];
		}
	}
	
	if(fft->obank_flag){
		oscbank(fft);
	} else {
		unconvert(fft);
		rdft(fft, -1);
		overlapadd(fft);
	}

	x->frames_left = frames_left;
	
}

void cavoc27_perform64(t_cavoc27 *x, t_object *dsp64, double **ins,
                         long numins, double **outs,long numouts, long vectorsize,
                         long flags, void *userparam)
{
	int	i,j;

	////////////
	t_fftease *fft = x->fft;
	
	double *MSPInputVector = ins[0];
	double *MSPOutputVector = outs[0];
	double *input = fft->input;
	int D = fft->D;
	int Nw = fft->Nw;
	double *output = fft->output;
	double mult = fft->mult ;
	int MSPVectorSize = fft->MSPVectorSize;
	double *internalInputVector = fft->internalInputVector;
	double *internalOutputVector = fft->internalOutputVector;		
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;	
	
	if(x->mute || x->x_obj.z_disabled){
        for(i=0; i < vectorsize; i++){ MSPOutputVector[i] = 0.0; }
	}
	if(fft->obank_flag){
		mult *= FFTEASE_OSCBANK_SCALAR;
	}
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
		do_cavoc27(x);
		for ( j = 0; j < D; j++ ){
			*MSPOutputVector++ = output[j] * mult;
        }
        sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
        for(j = (Nw-D); j < Nw; j++){
            output[j] = 0.0;
        }
    }
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(MSPInputVector + (D*i), input + (Nw-D), D * sizeof(t_double));
			do_cavoc27(x);
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
			do_cavoc27(x);
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

int cavoc27_apply_rule( short left, short right, short center, short *rule){
	
	if( left == 0 && center == 0 && right == 0 )
		return rule[0];
	if( left == 1 && center == 0 && right == 1 )
		return rule[1];
	if( left == 1 && center == 0 && right == 0 )
		return rule[2];
	if( left == 0 && center == 0 && right == 1 )
		return rule[3];
	if( left == 2 && center == 0 && right == 2 )
		return rule[4];
	if( left == 2 && center == 0 && right == 0 )
		return rule[5];
	if( left == 0 && center == 0 && right == 2 )
		return rule[6];
	if( left == 2 && center == 0 && right == 1 )
		return rule[7];
	if( left == 1 && center == 0 && right == 2 )
		return rule[8];
	
	if( left == 0 && center == 1 && right == 0 )
		return rule[9];
	if( left == 1 && center == 1 && right == 1 )
		return rule[10];
	if( left == 1 && center == 1 && right == 0 )
		return rule[11];
	if( left == 0 && center == 1 && right == 1 )
		return rule[12];
	if( left == 2 && center == 1 && right == 2 )
		return rule[13];
	if( left == 2 && center == 1 && right == 0 )
		return rule[14];
	if( left == 0 && center == 1 && right == 2 )
		return rule[15];
	if( left == 2 && center == 1 && right == 1 )
		return rule[16];
	if( left == 1 && center == 1 && right == 2 )
		return rule[17];
	
	if( left == 0 && center == 2 && right == 0 )
		return rule[18];
	if( left == 1 && center == 2 && right == 1 )
		return rule[19];
	if( left == 1 && center == 2 && right == 0 )
		return rule[20];
	if( left == 0 && center == 2 && right == 1 )
		return rule[21];
	if( left == 2 && center == 2 && right == 2 )
		return rule[22];
	if( left == 2 && center == 2 && right == 0 )
		return rule[23];
	if( left == 0 && center == 2 && right == 2 )
		return rule[24];
	if( left == 2 && center == 2 && right == 1 )
		return rule[25];
	if( left == 1 && center == 2 && right == 2 )
		return rule[26];
	return 0; //should never happen  
}

t_max_err get_fftsize(t_cavoc27 *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_cavoc27 *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		cavoc27_init(x);
	}
	return MAX_ERR_NONE;
}


t_max_err get_overlap(t_cavoc27 *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_cavoc27 *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		cavoc27_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_density(t_cavoc27 *x, void *attr, long *ac, t_atom **av)
{
	if (ac && av) {
		char alloc;
		
		if (atom_alloc(ac, av, &alloc)) {
			return MAX_ERR_GENERIC;
		}
		atom_setfloat(*av, x->density);
	}
	return MAX_ERR_NONE;
	
}

t_max_err set_density(t_cavoc27 *x, void *attr, long ac, t_atom *av)
{
    int i;
    t_fftease *fft = x->fft;
    double *channel =  x->tmpchannel;
    double density;
	if (ac && av) {
		density = (double)atom_getfloat(av);
        if( density < 0.0001 ){
            density = .0001;
        } else if( density > .9999 ){
            density = 1.0;
        }
        x->density = density;
        x->start_breakpoint = 1.0 - x->density;
        for( i = 0; i < fft->N2 + 1; i++ ){
            if( fftease_randf(0.0, 1.0) > x->start_breakpoint ){
                if( fftease_randf(0.0,1.0) > 0.5 ){
                    channel[ i * 2 ] = 1;
                }
                else {
                    channel[ i * 2 ] = 2;
                }
                ++(x->set_count);
            } else {
                channel[ i * 2 ] = 0;
            }
        }
        for( i = 0; i < fft->N+2; i++ ){
            x->last_frame[i] = channel[i];
        }
        /*
        if( density < 0.0001 ){
            density = .0001;
        } else if( density > .9999 ){
            density = 1.0;
        }
        x->density = density;
        x->start_breakpoint = 1.0 - x->density;
        for( i = 0; i < fft->N2 + 1; i++ ){
            if( cavoc_randf(0.0, 1.0) > x->start_breakpoint ){
                x->amps[ i ] = 1;
                ++(x->set_count);
            } else {
                x->amps[ i ] = 0;
            }
        }
        */
	}
	return MAX_ERR_NONE;
}

/*
 void cavoc27_density(t_cavoc27 *x, t_floatarg density)
 {
 int i;
 t_fftease *fft = x->fft;
 double *channel =  x->tmpchannel;
 
 if( density < 0.0001 ){
 density = .0001;
 } else if( density > .9999 ){
 density = 1.0;
 }
 x->density = density;
 x->start_breakpoint = 1.0 - x->density;
 for( i = 0; i < fft->N2 + 1; i++ ){
 if( fftease_randf(0.0, 1.0) > x->start_breakpoint ){
 if( fftease_randf(0.0,1.0) > 0.5 ){
 channel[ i * 2 ] = 1;
 }
 else {
 channel[ i * 2 ] = 2;
 }
 ++(x->set_count);
 } else {
 channel[ i * 2 ] = 0;
 }
 }
 for( i = 0; i < fft->N+2; i++ ){
 x->last_frame[i] = channel[i];
 }
 }
 */

t_max_err get_holdtime(t_cavoc27 *x, void *attr, long *ac, t_atom **av)
{
	if (ac && av) {
		char alloc;
		
		if (atom_alloc(ac, av, &alloc)) {
			return MAX_ERR_GENERIC;
		}
		atom_setfloat(*av, x->hold_time);
	}
	return MAX_ERR_NONE;
}


t_max_err set_holdtime(t_cavoc27 *x, void *attr, long ac, t_atom *av)
{
    double f;
	if (ac && av) {
		f = (double) atom_getfloat(av);
        if(f <= 0){
            return 0;
        }
        x->hold_time = f;
        if(! x->frame_duration){
            error("%s: zero frame duration",OBJECT_NAME);
            x->frame_duration = .15;
        }
        x->hold_frames = (int) ( (f/1000.0) / x->frame_duration);
        if( x->hold_frames < 1 )
            x->hold_frames = 1;
        x->frames_left = x->hold_frames;
	}
	return MAX_ERR_NONE;
}

/*
 void cavoc27_hold_time(t_cavoc27 *x, t_floatarg hold_time)
 {
 
 if(hold_time <= 0){
 post("illegal hold time %f",hold_time);
 return;
 }
 x->hold_time = hold_time * 0.001;
 if(! x->frame_duration){
 error("%s: zero frame duration",OBJECT_NAME);
 x->frame_duration = .15;
 }
 x->hold_frames = (int) ( (hold_time/1000.0) / x->frame_duration);
 if( x->hold_frames < 1 )
 x->hold_frames = 1;
 //  x->frames_left = x->hold_frames;
 
 }
 */


void cavoc27_dsp64(t_cavoc27 *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
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
        cavoc27_init(x);
    }
    object_method(dsp64, gensym("dsp_add64"),x,cavoc27_perform64,0,NULL);
}

