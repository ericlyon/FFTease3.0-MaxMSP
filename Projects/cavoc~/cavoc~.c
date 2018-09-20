#include "fftease.h"

static t_class *cavoc_class;

#define OBJECT_NAME "cavoc~"

typedef struct _cavoc
{
	t_pxobject x_obj;

	t_fftease *fft;
	double frame_duration;
	int max_bin;
	
	double fundamental;
	short left;
	short right;
	short center;
	short *rule;
	
	double start_breakpoint;
	int hold_frames;
	int frames_left;
	int set_count;
	void *list_outlet;
	t_atom *list_data;
	short mute;
	short external_trigger;
	short trigger_value; // set to 1 when a bang is received
	
	double topfreq; // highest to synthesize - Nyquist by default
	double bottomfreq;
	double *freqs;
	double *amps;
	double *cavoc;
	long fftsize_attr;
	long overlap_attr;
    double density; // treat as attribute
    double hold_time; // treat as attribute
} t_cavoc;

void *cavoc_new(t_symbol *msg, short argc, t_atom *argv);
void cavoc_dsp(t_cavoc *x, t_signal **sp, short *count);
void cavoc_assist(t_cavoc *x, void *b, long m, long a, char *s);
void cavoc_free( t_cavoc *x );
int cavoc_apply_rule( short left, short right, short center, short *rule);
float cavoc_randf(float min, float max);
void cavoc_rule (t_cavoc *x, t_symbol *msg, short argc, t_atom *argv);
void cavoc_retune (t_cavoc *x, t_floatarg min, t_floatarg max);
void cavoc_mute (t_cavoc *x, t_floatarg toggle);
void cavoc_external_trigger(t_cavoc *x, t_floatarg toggle);
void cavoc_init(t_cavoc *x);
//void cavoc_overlap(t_cavoc *x, t_floatarg f);
void cavoc_winfac(t_cavoc *x, t_floatarg f);
void cavoc_fftinfo(t_cavoc *x);
//void cavoc_fftsize(t_cavoc *x, t_floatarg f);
void cavoc_bang(t_cavoc *x);
void cavoc_topfreq(t_cavoc *x, t_floatarg tf);
void cavoc_oscbank(t_cavoc *x, t_floatarg flag);
void build_spectrum(t_cavoc *x, float min, float max);
void cavoc_bottomfreq(t_cavoc *x, t_floatarg bf);
t_max_err get_fftsize(t_cavoc *x, void *attr, long *ac, t_atom **av);
t_max_err set_fftsize(t_cavoc *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_cavoc *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_cavoc *x, void *attr, long ac, t_atom *av);
t_max_err get_density(t_cavoc *x, void *attr, long *ac, t_atom **av);
t_max_err set_density(t_cavoc *x, void *attr, long ac, t_atom *av);
t_max_err get_holdtime(t_cavoc *x, void *attr, long *ac, t_atom **av);
t_max_err set_holdtime(t_cavoc *x, void *attr, long ac, t_atom *av);

void cavoc_perform64(t_cavoc *x, t_object *dsp64, double **ins,
                     long numins, double **outs,long numouts, long vectorsize,
                     long flags, void *userparam);
void cavoc_dsp64(t_cavoc *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);

int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.cavoc~",(method)cavoc_new,(method)dsp_free, sizeof(t_cavoc), 0,A_GIMME,0);
	
	class_addmethod(c,(method)cavoc_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)cavoc_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)cavoc_rule,"rule",A_GIMME,0);
	class_addmethod(c,(method)cavoc_mute,"mute",A_FLOAT,0);
	class_addmethod(c,(method)cavoc_external_trigger,"external_trigger",A_FLOAT,0);
	class_addmethod(c,(method)cavoc_bang,"bang",0);
	class_addmethod(c,(method)cavoc_retune,"retune",A_FLOAT,A_FLOAT,0);
	class_addmethod(c,(method)cavoc_topfreq,"topfreq",A_FLOAT,0);
	class_addmethod(c,(method)cavoc_bottomfreq,"bottomfreq",A_FLOAT,0);
//	class_addmethod(c,(method)cavoc_overlap,"overlap",A_FLOAT,0);
	class_addmethod(c,(method)cavoc_winfac,"winfac",A_FLOAT,0);
//	class_addmethod(c,(method)cavoc_fftsize,"fftsize",A_FLOAT,0);
	class_addmethod(c,(method)cavoc_oscbank,"oscbank",A_FLOAT,0);
	class_addmethod(c,(method)cavoc_fftinfo,"fftinfo",0);
	
	CLASS_ATTR_LONG(c, "fftsize", 0, t_cavoc, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");
	
	CLASS_ATTR_LONG(c, "overlap", 0, t_cavoc, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");

	CLASS_ATTR_DOUBLE(c, "density", 0, t_cavoc, density);
	CLASS_ATTR_ACCESSORS(c, "density", (method)get_density, (method)set_density);
	CLASS_ATTR_LABEL(c, "density", 0, "Density");

 	CLASS_ATTR_DOUBLE(c, "holdtime", 0, t_cavoc, hold_time);
	CLASS_ATTR_ACCESSORS(c, "holdtime", (method)get_holdtime, (method)set_holdtime);
	CLASS_ATTR_LABEL(c, "holdtime", 0, "Hold Time");
    
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
    CLASS_ATTR_ORDER(c, "density",    0, "3");
    CLASS_ATTR_ORDER(c, "holdtime",    0, "4");
    
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	cavoc_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

t_max_err get_fftsize(t_cavoc *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_cavoc *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		cavoc_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_cavoc *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_cavoc *x, void *attr, long ac, t_atom *av)
{
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		cavoc_init(x);
	}
	return MAX_ERR_NONE;
}




t_max_err get_density(t_cavoc *x, void *attr, long *ac, t_atom **av)
{
	if (ac && av) {
		char alloc;
		
		if (atom_alloc(ac, av, &alloc)) {
			return MAX_ERR_GENERIC;
		}
		// x->fftsize_attr = x->fft->N;
		atom_setfloat(*av, x->density);
	}
	return MAX_ERR_NONE;
	
}

t_max_err set_density(t_cavoc *x, void *attr, long ac, t_atom *av)
{
    int i;
    t_fftease *fft = x->fft;
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
            if( cavoc_randf(0.0, 1.0) > x->start_breakpoint ){
                x->amps[ i ] = 1;
                ++(x->set_count);
            } else {
                x->amps[ i ] = 0;
            }
        }
	}
	return MAX_ERR_NONE;
}



t_max_err get_holdtime(t_cavoc *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_holdtime(t_cavoc *x, void *attr, long ac, t_atom *av)
{
    double f;
	if (ac && av) {
		f = (double) atom_getfloat(av);
        if(f <= 0)
            return 0;
        x->hold_time = f;
        x->hold_frames = (int) ((x->hold_time/1000.0) / x->frame_duration);
        if( x->hold_frames < 1 )
            x->hold_frames = 1;
        x->frames_left = x->hold_frames;
	}
	return MAX_ERR_NONE;
}



void cavoc_fftinfo( t_cavoc *x )
{
t_fftease *fft = x->fft;
//	int i;
	fftease_fftinfo( fft, OBJECT_NAME );
    /*
	post("frames left %d top freq %f", x->frames_left, x->topfreq);
	for(i=0; i < fft->N2; i++){
		post("%d: %f %f",i, fft->channel[i*2], fft->channel[i*2 + 1]);
	}
    */
}

void cavoc_overlap(t_cavoc *x, t_floatarg f)
{
	x->fft->overlap = (int)f;
	cavoc_init(x);
}

void cavoc_winfac(t_cavoc *x, t_floatarg f)
{
	x->fft->winfac = (int)f;
	cavoc_init(x);
}

void cavoc_fftsize(t_cavoc *x, t_floatarg f)
{
	x->fft->N = (int) f;
	cavoc_init(x);
}

void cavoc_external_trigger(t_cavoc *x, t_floatarg toggle)
{
	x->external_trigger = (short)toggle;
}

void cavoc_mute (t_cavoc *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void cavoc_retune(t_cavoc *x, t_floatarg min, t_floatarg max)
{	
	if( max <= 0 || min <= 0 || min > max ){
		error("bad values for min and max multipliers");
		return;
	}
	if( min < .1 )
		min = 0.1;
	if( max > 2.0 )
		max = 2.0;
	build_spectrum(x, (float)min, (float)max);
}



void cavoc_bang(t_cavoc *x)
{
	if(x->external_trigger)
		x->trigger_value = 1;
}

void cavoc_oscbank(t_cavoc *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}
/*
void cavoc_hold_time(t_cavoc *x, t_floatarg f)
{
	
	if(f <= 0)
		return;
	x->hold_time = f;
	x->hold_frames = (int) ((x->hold_time/1000.0) / x->frame_duration);
	if( x->hold_frames < 1 )
		x->hold_frames = 1;
	x->frames_left = x->hold_frames;
	
}
*/
void cavoc_rule (t_cavoc *x, t_symbol *msg, short argc, t_atom *argv)
{
	int i;
	short *rule = x->rule;
	if( argc != 8 ){
		error("the rule must be size 8");
		return;
	}
	
	for( i = 0; i < 8; i++ ){
		rule[i] = (short) atom_getfloatarg(i, argc, argv);
		// post("%d",rule[i]);
	}
}

void cavoc_free( t_cavoc *x ){
	dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
	sysmem_freeptr(x->amps);
	sysmem_freeptr(x->freqs);
	sysmem_freeptr(x->rule);  
}

void cavoc_assist (t_cavoc *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Trigger");break;
		}
	} else if (msg==2) {
		switch (arg) {
			case 0:sprintf(dst,"(signal) Output"); break;
		}
	}
}

void *cavoc_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_cavoc *x = (t_cavoc *)object_alloc(cavoc_class);
	dsp_setup((t_pxobject *)x,1);
	outlet_new((t_pxobject *)x, "signal");
	x->x_obj.z_misc |= Z_NO_INPLACE;

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	x->fft->R = sys_getsr();
	x->fft->MSPVectorSize = sys_getblksize();
    x->fft->initialized = 0;
    // these should be attributes:
    
	x->density = 0.1;
	x->hold_time = 500.0; // convert from ms

	x->fft->obank_flag = 0;
	x->fft->N = FFTEASE_DEFAULT_FFTSIZE;
	x->fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	x->fft->winfac = FFTEASE_DEFAULT_WINFAC;
	
	cavoc_init(x);
    attr_args_process(x, argc, argv);
	return x;
}

void cavoc_init(t_cavoc *x)
{
	t_fftease *fft = x->fft;
	x->x_obj.z_disabled = 1;
	short initialized = fft->initialized;
	fftease_init(fft);

	fft->lo_bin = 0; 
	fft->hi_bin = fft->N2 - 1; 
			
	if(! fft->R ){
		error("zero sampling rate!");
		return;
	}
	x->frame_duration = (float)fft->D/(float) fft->R;
	if(x->hold_time <= 0.0)
		x->hold_time = 150;
	x->hold_frames = (int) ((x->hold_time * 0.001) / x->frame_duration) ;
	x->frames_left = x->hold_frames;
	x->trigger_value = 0;
	x->topfreq = fft->R / 2.0;
	x->bottomfreq = 0.0;

	if(!initialized){
		
		srand(time(0));
		x->mute = 0;
		
		x->external_trigger = 0;
		if( x->density < 0.0 ){
			x->density = 0;
		} else if( x->density > 1.0 ){
			x->density = 1.0;
		}
		x->start_breakpoint = 1.0 - x->density;
		x->freqs = (double *) sysmem_newptrclear((fft->N2 + 1) * sizeof(double));
		x->amps = (double *) sysmem_newptrclear((fft->N2 + 1) * sizeof(double));
		x->cavoc = (double *) sysmem_newptrclear((fft->N + 2) * sizeof(double));
		x->rule = (short *)  sysmem_newptrclear(8 * sizeof(short));
		
		x->rule[2] = x->rule[3] = x->rule[5] = x->rule[6] = 1;
		x->rule[0] = x->rule[1] = x->rule[4] = x->rule[7] = 0;
		
		
	} else {    
		x->freqs = (double *) sysmem_resizeptrclear(x->freqs, (fft->N2 + 1) * sizeof(double));
		x->amps = (double *) sysmem_resizeptrclear(x->amps, (fft->N2 + 1) * sizeof(double));
	}

	build_spectrum(x, 0.9, 1.1);
	x->x_obj.z_disabled = 0;
}


void build_spectrum(t_cavoc *x, float min, float max)
{
t_fftease *fft = x->fft;
float basefreq;
int i;
	x->set_count = 0;
	for(i = 0; i < fft->N2 + 1; i++){
		if(cavoc_randf(0.0, 1.0) > x->start_breakpoint){
			x->amps[i] = 1;
			++(x->set_count);
		} else {
			x->amps[i] = 0;
		}
		basefreq = x->bottomfreq + (( (x->topfreq - x->bottomfreq) / (float) fft->N2 ) * (float) i );
		x->freqs[i] = basefreq * cavoc_randf(min,max);
	}
	//  post("turned on %d of a possible %d bins", x->set_count, x->N2+1 );
	
	for( i = 0; i < fft->N2 + 1; i++ ){
		fft->channel[i * 2] = x->amps[i];
		fft->channel[i * 2 + 1] = x->freqs[i];
//		post("%d: %f %f", i, x->amps[i], x->freqs[i]);
	}

}

void cavoc_topfreq(t_cavoc *x, t_floatarg tf)
{
	t_fftease *fft = x->fft;
	if(tf < 100 || tf > fft->R / 2.0){
		error("%s: top frequency out of range: %f",OBJECT_NAME,  tf);
		return;
	}
	x->topfreq = (float) tf;
	build_spectrum(x, 0.9, 1.1);
}

void cavoc_bottomfreq(t_cavoc *x, t_floatarg bf)
{
//	t_fftease *fft = x->fft;
	if(bf < 0 && bf > x->topfreq){
		error("%s: bottom frequency out of range: %f",OBJECT_NAME,  bf);
		return;
	}
	x->bottomfreq = (float) bf;
	build_spectrum(x, 0.9, 1.1);
}

void do_cavoc(t_cavoc *x)
{
	int i;
	t_fftease *fft = x->fft;
	int N = fft->N;
	int N2 = fft->N2;
	double *channel = fft->channel;
	int hold_frames = x->hold_frames;
	short *rule = x->rule;
	short left = x->left;
	short right = x->right;
	short center = x->center;
	short external_trigger = x->external_trigger;
	short new_event = 0;
	double *amps = x->amps;
	double *freqs = x->freqs;
			
	if(external_trigger){// only accurate to within a vector because of FFT
		if(x->trigger_value){
			x->trigger_value = 0;
			new_event = 1;
		}
	} else if(--(x->frames_left) <= 0){
		x->frames_left = hold_frames;
		new_event = 1;
	}
	if(new_event){
		for( i = 1; i < N2; i++ ){
			left = amps[i - 1];
			center = amps[i] ;
			right = amps[i + 1];
			channel[i * 2] = cavoc_apply_rule(left, right, center, rule);
		}
		center = amps[0];
		right = amps[1];
		left = amps[N2];
		channel[0] = cavoc_apply_rule(left, right, center, rule);
		
		center = amps[N2];
		right = amps[0];
		left = amps[N2 - 1];
		channel[N] = cavoc_apply_rule(left, right, center, rule);
		for(i = 0; i < N2 + 1; i++){
			channel[(i*2) + 1] = freqs[i];
			amps[i] = channel[i * 2];
		}	
	}
		
	if(fft->obank_flag){
		for(i = 0; i < N2 + 1; i++){
			channel[(i*2) + 1] = freqs[i];
			channel[i * 2] = amps[i];
		}
		oscbank(fft);
	} else {
		unconvert(fft);
		rdft(fft, -1);
		overlapadd(fft);
	}		
}

void cavoc_perform64(t_cavoc *x, t_object *dsp64, double **ins,
                       long numins, double **outs,long numouts, long vectorsize,
                       long flags, void *userparam)
{
	int	i,j;
	double *MSPOutputVector = outs[0];
	t_fftease *fft = x->fft;
	int D = fft->D;
	int Nw = fft->Nw;
	double *output = fft->output;
	float mult = fft->mult ;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	double *internalOutputVector = fft->internalOutputVector;
    
	int MSPVectorSize = fft->MSPVectorSize;
	
    
	if( x->mute || x->x_obj.z_disabled ){
        for(i=0; i < vectorsize; i++){ MSPOutputVector[i] = 0.0; }
        return;
	}
	
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        
		do_cavoc(x);
        
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
			do_cavoc(x);
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
		memcpy(MSPOutputVector, internalOutputVector + (operationCount * MSPVectorSize), MSPVectorSize * sizeof(float));
		operationCount = (operationCount + 1) % operationRepeat;
		if( operationCount == 0 ) {
			do_cavoc(x);
			for ( j = 0; j < D; j++ ){
				internalOutputVector[j] = output[j] * mult;
			}
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){
                output[j] = 0.0;
            }
		}
	}
	fft->operationCount = operationCount;
}		

int cavoc_apply_rule(short left, short right, short center, short *rule){
	
	if( ! center ){
		if( ! left && ! right){
			return  rule[0];
		} else if ( ! left && right ){ 
			return rule[1];
		} else if ( left && ! right ) {
			return rule[2];
		} else if (left && right) {
			return rule[3];
		}
	} else {
		if( ! left && ! right){
			return rule[4];
		} else if ( ! left && right ){ 
			return rule[5];
		} else if ( left && ! right ) {
			return rule[6];
		} else if (left && right) {
			return rule[7];
		}
	}
	return 0;// never happens
}

float cavoc_randf(float min, float max)
{
	float randv;
	randv = (float) (rand() % 32768) / 32768.0 ;
	return (min + ((max-min) * randv))  ;
}

void cavoc_dsp64(t_cavoc *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
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
        cavoc_init(x);
    }
    object_method(dsp64, gensym("dsp_add64"),x,cavoc_perform64,0,NULL);
}
