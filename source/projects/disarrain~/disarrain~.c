#include "fftease.h"

static t_class *disarrain_class;

#define OBJECT_NAME "disarrain~"

typedef struct _disarrain
{
	t_pxobject x_obj;
	t_fftease *fft;
	double *last_channel;
	double *composite_channel;
	int *shuffle_mapping;
	int *last_shuffle_mapping;
	int *shuffle_tmp; // work space for making a new distribution
	int shuffle_count;// number of bins to swap
	int last_shuffle_count;// ditto from last shuffle mapping
	int max_bin;
	void *list_outlet;
	t_atom *list_data;
	short mute;
	short bypass;
	double frame_duration; // duration in seconds of a single frame
	double interpolation_duration; // duration in seconds of interpolation
	int interpolation_frames; // number of frames to interpolate
	int frame_countdown; // keep track of position in interpolation
	
	int perform_method;// 0 for lean, 1 for full conversion
	float ival;
	short lock;// lock for switching mapping arrays, but not used now
	short force_fade; // new fadetime set regardless of situation
	short force_switch;// binds new distribution to change of bin count
	long fftsize_attr;
	long overlap_attr;
    double top_frequency;// for remapping spectrum (NOW AN ATTRIBUTE)
	short reset_flag; // call for reset 
	short switchcount_flag; // call for switch count
	int new_shuffle_count; // call to change switch count
} t_disarrain;

void *disarrain_new(t_symbol *msg, short argc, t_atom *argv);
void disarrain_assist(t_disarrain *x, void *b, long m, long a, char *s);
void disarrain_switch_count (t_disarrain *x, t_floatarg i);
// void disarrain_topfreq (t_disarrain *x, t_floatarg freq);
void disarrain_fadetime (t_disarrain *x, t_floatarg f);
void reset_shuffle( t_disarrain *x );
void disarrain_showstate( t_disarrain *x );
void disarrain_list (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv);
void disarrain_setstate (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv);
void disarrain_isetstate (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv);
int rand_index(int max);
void disarrain_mute(t_disarrain *x, t_floatarg toggle);
void disarrain_bypass(t_disarrain *x, t_floatarg toggle);
void copy_shuffle_array(t_disarrain *x);
void interpolate_frames_to_channel(t_disarrain *x);
void disarrain_killfade(t_disarrain *x);
void disarrain_forcefade(t_disarrain *x, t_floatarg toggle);
void disarrain_init(t_disarrain *x);
void disarrain_free(t_disarrain *x);
void disarrain_winfac(t_disarrain *x, t_floatarg o);
void disarrain_fftinfo(t_disarrain *x);
void disarrain_force_switch(t_disarrain *x, t_floatarg toggle);
t_max_err set_fftsize(t_disarrain *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_disarrain *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_disarrain *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_disarrain *x, void *attr, long *ac, t_atom **av);
t_max_err get_maxfreq(t_disarrain *x, void *attr, long *ac, t_atom **av);
t_max_err set_maxfreq(t_disarrain *x, void *attr, long ac, t_atom *av);
void disarrain_dsp64(t_disarrain *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void disarrain_perform64(t_disarrain *x, t_object *dsp64, double **ins,
                         long numins, double **outs,long numouts, long vectorsize,
                         long flags, void *userparam);

int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.disarrain~", (method)disarrain_new, (method)disarrain_free, sizeof(t_disarrain),0,A_GIMME,0);
	
	class_addmethod(c,(method)disarrain_dsp64, "dsp64", A_CANT, 0);

	class_addmethod(c,(method)disarrain_switch_count, "int", A_FLOAT, 0);
	class_addmethod(c,(method)reset_shuffle, "bang", 0);
	
	class_addmethod(c,(method)disarrain_showstate,"showstate",0);
	class_addmethod(c,(method)disarrain_list, "list", A_GIMME, 0);
	class_addmethod(c,(method)disarrain_setstate, "setstate", A_GIMME, 0);
	class_addmethod(c,(method)disarrain_isetstate, "isetstate", A_GIMME, 0);
	class_addmethod(c,(method)disarrain_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)disarrain_mute, "mute", A_FLOAT, 0);
//	class_addmethod(c,(method)disarrain_topfreq, "topfreq", A_FLOAT, 0);
	class_addmethod(c,(method)disarrain_fadetime, "fadetime", A_FLOAT, 0);
	class_addmethod(c,(method)disarrain_bypass, "bypass", A_FLOAT, 0);
	class_addmethod(c,(method)disarrain_forcefade, "forcefade", A_FLOAT, 0);
	class_addmethod(c,(method)disarrain_force_switch, "force_switch", A_FLOAT, 0);
	class_addmethod(c,(method)disarrain_switch_count, "switch_count", A_FLOAT, 0);
	class_addmethod(c,(method)disarrain_killfade, "killfade", 0);
	class_addmethod(c,(method)reset_shuffle, "reset_shuffle", 0);	
	class_addmethod(c,(method)disarrain_winfac, "winfac",  A_DEFFLOAT, 0);
	class_addmethod(c,(method)disarrain_fftinfo, "fftinfo", 0);
    
	CLASS_ATTR_LONG(c, "fftsize", 0, t_disarrain, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");
    
	CLASS_ATTR_LONG(c, "overlap", 0, t_disarrain, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");

 	CLASS_ATTR_DOUBLE(c, "maxfreq", 0, t_disarrain, top_frequency);
	CLASS_ATTR_ACCESSORS(c, "maxfreq", (method)get_maxfreq, (method)set_maxfreq);
	CLASS_ATTR_LABEL(c, "maxfreq", 0, "Maximum Frequency");
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
    CLASS_ATTR_ORDER(c, "maxfreq",    0, "3");
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	disarrain_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

void disarrain_free(t_disarrain *x)
{
    dsp_free((t_pxobject *)x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
	sysmem_freeptr(x->last_channel);
	sysmem_freeptr(x->composite_channel);
	sysmem_freeptr(x->shuffle_mapping);
	sysmem_freeptr(x->last_shuffle_mapping);
	sysmem_freeptr(x->shuffle_tmp);
	sysmem_freeptr(x->list_data);
}

void disarrain_init(t_disarrain *x)
{
	int i;
	double curfreq;
	
	t_fftease *fft = x->fft;

	double c_fundamental;
	x->x_obj.z_disabled = 1;
	short initialized = fft->initialized;
	
 	fftease_init(fft);

	int N2 = fft->N2;
	int N = fft->N;
	int D = fft->D;
	int R = fft->R;
	c_fundamental = fft->c_fundamental;
	if(initialized == 0){
		x->mute = 0;
		x->bypass = 0;
		x->force_fade = 0;
		x->interpolation_duration = 0.1; //seconds
		x->shuffle_mapping = (int *) sysmem_newptrclear( N2 * sizeof(int) ) ;
		x->last_shuffle_mapping = (int *) sysmem_newptrclear( N2 * sizeof(int) ) ;
		x->shuffle_tmp = (int *) sysmem_newptrclear( N2 * sizeof(int) ) ;
		x->list_data = (t_atom *) sysmem_newptrclear((N+2) * sizeof(t_atom) ) ;
		x->last_channel = (double *) sysmem_newptrclear(N+2 * sizeof(float));
		x->composite_channel = (double *) sysmem_newptrclear(N+2 * sizeof(float));
		x->reset_flag = 0;
		x->new_shuffle_count = 0;
	} else {
		x->shuffle_mapping = (int *)sysmem_resizeptrclear(x->shuffle_mapping, N2 * sizeof(int));
		x->last_shuffle_mapping = (int *)sysmem_resizeptrclear(x->last_shuffle_mapping, N2 * sizeof(int));
		x->shuffle_tmp = (int *)sysmem_resizeptrclear(x->shuffle_tmp, N2 * sizeof(int));
		x->list_data = (t_atom *)sysmem_resizeptrclear(x->list_data, (N+2) * sizeof(t_atom));
		x->last_channel = (double *)sysmem_resizeptrclear(x->last_channel,(N+2) * sizeof(double));
		x->composite_channel = (double *)sysmem_resizeptrclear(x->composite_channel, (N+2) * sizeof(double));

	}
	
	if( x->top_frequency < c_fundamental || x->top_frequency > 20000) {
		x->top_frequency = 20000.0 ;
	}
	x->max_bin = 1;  
	curfreq = 0;
	while( curfreq < x->top_frequency ) {
		++(x->max_bin);
		curfreq += c_fundamental ;
	}
	for( i = 0; i < N2; i++ ) {
		x->shuffle_mapping[i] = x->last_shuffle_mapping[i] = i*2;
	}
	reset_shuffle(x); // set shuffle lookup
	copy_shuffle_array(x);// copy it to the last lookup (for interpolation)
	x->frame_duration = (float) D / (float) R;
	x->interpolation_frames = x->interpolation_duration / x->frame_duration;
	x->frame_countdown = 0;
	x->shuffle_count = 0;
	x->last_shuffle_count = 0;
	x->x_obj.z_disabled = 0;
}


void disarrain_force_switch(t_disarrain *x, t_floatarg f)
{
	x->force_switch = (short)f;
}

void disarrain_fadetime (t_disarrain *x, t_floatarg f)
{
	int frames;
	float duration;
	
	if(x->frame_duration <= 0.0){
		error("%s: frame duration %f is too low", OBJECT_NAME, x->frame_duration);
		return;
	}
	duration = f * .001;
	frames = duration / x->frame_duration;
	if( frames <= 1){
		error("%s: fadetime too short",OBJECT_NAME);
		return;
	}
	x->interpolation_duration = f * .001;
	x->interpolation_frames = frames;
	
}

void disarrain_killfade(t_disarrain *x)
{
	x->frame_countdown = 0;
	
}



void disarrain_assist (t_disarrain *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input"); break;
		}
	} else if (msg==2) {
		switch (arg) {
			case 0:	sprintf(dst,"(signal) Output"); break;
			case 1: sprintf(dst,"(signal) Interpolation Sync"); break;
			case 2: sprintf(dst,"(list) Current State"); break;
		}
	}
}

void *disarrain_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_fftease *fft;
	t_disarrain *x = (t_disarrain *)object_alloc(disarrain_class);
	x->list_outlet = listout((t_pxobject *)x);
	dsp_setup((t_pxobject *)x,1);
	outlet_new((t_pxobject *)x, "signal");
	outlet_new((t_pxobject *)x, "signal");
	srand(time(0));
	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();	
	fft->initialized = 0;
	x->top_frequency = 3000;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	attr_args_process(x, argc, argv);
	disarrain_init(x);
	return x;
}

void disarrain_forcefade(t_disarrain *x, t_floatarg toggle)
{
	x->force_fade = (short)toggle;	
}

void disarrain_mute(t_disarrain *x, t_floatarg toggle)
{
	x->mute = (short)toggle;	
}

void disarrain_bypass(t_disarrain *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;	
}

void disarrain_fftsize(t_disarrain *x, t_floatarg f)
{	
	t_fftease *fft = x->fft;
	fft->N = (int) f;
	disarrain_init(x);
}

void disarrain_overlap(t_disarrain *x, t_floatarg f)
{
	t_fftease *fft = x->fft;
	fft->overlap = (int) f;
	disarrain_init(x);
}

void disarrain_winfac(t_disarrain *x, t_floatarg f)
{
	t_fftease *fft = x->fft;
	fft->winfac = (int) f;
	disarrain_init(x); /* calling lighter reinit routine */
}

void disarrain_fftinfo( t_disarrain *x )
{
	fftease_fftinfo( x->fft, OBJECT_NAME );
}

void do_disarrain(t_disarrain *x)
{
	t_fftease *fft = x->fft;
	double *channel = fft->channel;
	int N = fft->N;
	int  N2 = fft->N2;
	double *last_channel = x->last_channel;
	int		i,j;
	int max = x->max_bin;
	int temp, p1, p2;
	float tmp;
	int *shuffle_mapping = x->shuffle_mapping;
	int shuffle_count = x->shuffle_count;
	int *last_shuffle_mapping = x->last_shuffle_mapping;
	int *shuffle_tmp = x->shuffle_tmp;
	int last_shuffle_count = x->last_shuffle_count;	
	int frame_countdown = x->frame_countdown; // will read from variable
	int interpolation_frames = x->interpolation_frames;
	float ival = x->ival;
	int new_shuffle_count = x->new_shuffle_count;
	
// test
	/*
	float *input = fft->input;
	float *output = fft->output;
	for(i = 0; i < fft->D; i++){
		output[i] = input[i];
	}
	return;*/
	
	if(x->switchcount_flag){
		
		if( new_shuffle_count < 0 ){
			new_shuffle_count = 0;
		}
		if( new_shuffle_count > N2 ) {
			new_shuffle_count = N2;
		}
		if( new_shuffle_count > x->max_bin){
			new_shuffle_count = x->max_bin;
			post("disarrain~: switch constrained to %d", x->max_bin);
		}		

		memcpy(last_shuffle_mapping, shuffle_mapping,  N2 * sizeof(int));
		
		x->last_shuffle_count = x->shuffle_count;
		x->shuffle_count = new_shuffle_count;
		x->frame_countdown = x->interpolation_frames; // force interpolation

		shuffle_count = x->shuffle_count;
		frame_countdown = x->frame_countdown;
		
		x->switchcount_flag = 0;
		x->reset_flag = 0;
	}
	else if(x->reset_flag){
		/*
		for(i = 0; i<N2; i++){
			last_shuffle_mapping[i] = shuffle_mapping[i];
		}*/
		memcpy(last_shuffle_mapping, shuffle_mapping,  N2 * sizeof(int));
		last_shuffle_count = shuffle_count;
		shuffle_count = new_shuffle_count;
		// post("%d %d %d", last_shuffle_count, shuffle_count, new_shuffle_count);
		
		for( i = 0; i < N2; i++ ) {
			shuffle_tmp[i] = i;
		}
		
		// crashed before here
		for( i = 0; i < max; i++ ) {
			p1 = rand() % max;
			p2 = rand() % max;
			if(p1 < 0 || p1 > max || p2 < 0 || p2 > max){
				error("bad remaps: %d %d against %d", p1, p2, max);
			} else {
				temp = shuffle_tmp[p1];
				shuffle_tmp[ p1 ] = shuffle_tmp[ p2 ];
				shuffle_tmp[ p2 ] = temp;
			}
		}
		for( i = 0; i < N2; i++ ) {
			shuffle_tmp[i] *= 2;
		}
		frame_countdown = interpolation_frames;	
		// post("in: countdown: %d, frames: %d", frame_countdown, interpolation_frames);
		memcpy(shuffle_mapping, shuffle_tmp,  N2 * sizeof(int));
		
		x->reset_flag = 0;
	}
	
	fold(fft);	
	rdft(fft,1);
	leanconvert(fft);
	
	// first time for interpolation, just do last frame 
	
	if(frame_countdown == interpolation_frames){
		
		for( i = 0, j = 0; i < last_shuffle_count ; i++, j+=2){
			tmp = channel[j];
			channel[j] = channel[last_shuffle_mapping[i]];
			channel[last_shuffle_mapping[i]] = tmp;
		}
		--frame_countdown;
	} 
	// test only
	else if( frame_countdown > 0 ){
		ival = (float)frame_countdown/(float)interpolation_frames;
		// copy current frame to lastframe
		for(j = 0; j < N; j+=2){
			last_channel[j] = channel[j];
		}	
		// make last frame swap
		for(i = 0, j = 0; i < last_shuffle_count ; i++, j+=2){
			tmp = last_channel[j];
			last_channel[j] = last_channel[last_shuffle_mapping[i]];
			last_channel[last_shuffle_mapping[i]] = tmp;
			
		}	
		// make current frame swap
		for( i = 0, j = 0; i < shuffle_count ; i++, j+=2){
			tmp = channel[j];
			channel[j]  = channel[shuffle_mapping[i]];
			channel[shuffle_mapping[i]]  = tmp;
			
		}
		// now interpolate between the two
		
		for(j = 0; j < N; j+=2){
			channel[j] = channel[j] + ival * (last_channel[j] - channel[j]);
		}
		
		--frame_countdown;
		if(frame_countdown <= 0){
			for(i = 0; i<N2; i++){
				last_shuffle_mapping[i] = shuffle_mapping[i];
			}
			last_shuffle_count = shuffle_count;
		}
	} else {
		// otherwise straight swapping
		for( i = 0, j = 0; i < shuffle_count ; i++, j+=2){
			tmp = channel[j];
			channel[j]  = channel[ shuffle_mapping[i]];
			channel[shuffle_mapping[i]] = tmp;     
		}
		ival = 0.0;
	}
	
	leanunconvert(fft);
	rdft(fft,-1);
	overlapadd(fft);


	
	x->frame_countdown = frame_countdown;
	x->last_shuffle_count = last_shuffle_count;
	x->shuffle_count = shuffle_count;
	x->ival = ival;
}

// lean convert perform method
void disarrain_perform64(t_disarrain *x, t_object *dsp64, double **ins,
                           long numins, double **outs,long numouts, long vectorsize,
                           long flags, void *userparam)
{
	int i,j;
	t_fftease *fft = x->fft;
	double *MSPInputVector = ins[0];
	double *MSPOutputVector = outs[0];
	double *sync_vec = outs[1];
	double *input = fft->input;
	int D = fft->D;
	int Nw = fft->Nw;
	double *output = fft->output;
	double mult = fft->mult;
	int MSPVectorSize = fft->MSPVectorSize;
	double *internalInputVector = fft->internalInputVector;
	double *internalOutputVector = fft->internalOutputVector;		
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;	
	
	if(x->mute || x->x_obj.z_disabled){
		for(i=0; i < vectorsize; i++){ MSPOutputVector[i] = 0.0; }
		for(i=0; i < vectorsize; i++){ sync_vec[i] = 0.0; }
		return;
	}	

	if (x->bypass) {
		for( j = 0; j < MSPVectorSize; j++) {
			*MSPOutputVector++ = *MSPInputVector++ * FFTEASE_BYPASS_GAIN;
		}
		return;
	}
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_disarrain(x);
        
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
            
			do_disarrain(x);
			
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
            
			do_disarrain(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
	/* send out sync signal */
	for(j = 0; j < MSPVectorSize; j++){
		sync_vec[j] = x->ival;
	}
}		


void interpolate_frames_to_channel(t_disarrain *x)
{
	float ival;
	float tmp;
	int i,j;
	int frame_countdown = x->frame_countdown;
	int interpolation_frames = x->interpolation_frames;
	double *channel = x->fft->channel;
	double *last_channel = x->last_channel;
	int *shuffle_mapping = x->shuffle_mapping;
	int shuffle_count = x->shuffle_count;
	int *last_shuffle_mapping = x->last_shuffle_mapping;
	int last_shuffle_count = x->last_shuffle_count;	
	int local_max_bins;
	int N = x->fft->N;
	
	ival = (double)frame_countdown/(double)interpolation_frames;
	
	local_max_bins = (shuffle_count > last_shuffle_count)? shuffle_count : last_shuffle_count;
	for(j = 0; j < N; j+=2){
		last_channel[j] = channel[j];
	}
	// make last frame
	for( i = 0, j = 0; i < last_shuffle_count ; i++, j+=2){
		tmp = last_channel[j];
		last_channel[j] = last_channel[last_shuffle_mapping[i]];
		last_channel[last_shuffle_mapping[i]] = tmp;
	}
	// make current frame
	for( i = 0, j = 0; i < shuffle_count ; i++, j+=2){
		tmp = channel[j];
		channel[j]  = channel[shuffle_mapping[i]];
		channel[shuffle_mapping[i]]  = tmp;
	}
	// now interpolate between the two
	
	for(j = 0; j < N; j+=2){
		channel[j] += ival * (last_channel[j] - channel[j]);
	}
}


void disarrain_switch_count (t_disarrain *x, t_floatarg f)
{
	int i = f;
	int j;
	if( i < 0 ){
		i = 0;
	}
	if( i > x->max_bin ) {
		i = x->max_bin;
	}
	for(j = 0; j < i; j++){
		if(x->shuffle_mapping[j] < 0 || x->shuffle_mapping[j] > x->max_bin * 2 || 
		   x->last_shuffle_mapping[j] < 0 || x->last_shuffle_mapping[j] > x->shuffle_mapping[j] > x->max_bin * 2
			){
			post("%d: %d %d",j, x->shuffle_mapping[j], x->last_shuffle_mapping[j] );
		}
	}
	
	x->reset_flag = 1;
	x->new_shuffle_count = i;


}


void reset_shuffle (t_disarrain *x)
{	
	x->reset_flag = 1;
}

void copy_shuffle_array(t_disarrain *x)
{
//	int i;
	int N2 = x->fft->N2;
	int *shuffle_mapping = x->shuffle_mapping;
	int *last_shuffle_mapping = x->last_shuffle_mapping;	
	
	/*
	for(i = 0; i<N2; i++){
		last_shuffle_mapping[i] = shuffle_mapping[i];
	}*/
	memcpy(shuffle_mapping, last_shuffle_mapping,  N2 * sizeof(int));
	x->last_shuffle_count = x->shuffle_count;
	
}


int rand_index(int max) {
	
	return (rand() % max);
}

/*
 void disarrain_topfreq (t_disarrain *x, t_floatarg freq)
 {
 double funda = (double) x->fft->R / (double) x->fft->N;
 double curfreq;
 
 if( freq < funda || freq > 20000) {
 freq = 1000.0 ;
 }
 x->max_bin = 1;
 curfreq = 0;
 while( curfreq < freq ) {
 ++(x->max_bin);
 curfreq += funda ;
 }
 }
 */

t_max_err get_maxfreq(t_disarrain *x, void *attr, long *ac, t_atom **av)
{
	if (ac && av) {
		char alloc;
		
		if (atom_alloc(ac, av, &alloc)) {
			return MAX_ERR_GENERIC;
		}
		atom_setlong(*av, x->top_frequency);
	}
	return MAX_ERR_NONE;
	
}

t_max_err set_maxfreq(t_disarrain *x, void *attr, long ac, t_atom *av)
{
    double funda = (double) x->fft->R / (double) x->fft->N;
    double curfreq;
	if (ac && av) {
		double val = atom_getfloat(av);
		x->top_frequency = val;
	}
    if( x->top_frequency < funda || x->top_frequency > 20000) {
        x->top_frequency = 1000.0 ;
    }
    x->max_bin = 1;
    curfreq = 0;
    while( curfreq < x->top_frequency ) {
        ++(x->max_bin);
        curfreq += funda ;
    }
	return MAX_ERR_NONE;
}


t_max_err get_fftsize(t_disarrain *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_disarrain *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		disarrain_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_disarrain *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_disarrain *x, void *attr, long ac, t_atom *av)
{	
    int test_overlap;
    if (ac && av) {
        long val = atom_getlong(av);
        test_overlap = fftease_overlap(val);
        if(test_overlap > 0){
            x->fft->overlap = (int) val;
            disarrain_init(x);
        }
    }
    return MAX_ERR_NONE;
}

// ENTER STORED SHUFFLE
void disarrain_list (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv) {
	short i;
	int ival;
	//  post("list message called");
	x->shuffle_count = argc;
	//  post("list: count now %d",x->shuffle_count );
	for (i=0; i < argc; i++) {

		ival = argv[i].a_w.w_long;
		if (ival < x->fft->N2) {
			x->shuffle_mapping[i] = ival;
			//      post("set %d to %d",i, x->shuffle_mapping[ i ]);
		} else {
			//      post ("%d out of range",ival);
		}
		
	}
	//  post("last val is %d", x->shuffle_mapping[argc - 1]);
	return;
}


void disarrain_isetstate (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv) {
	short i;
	int ival;
	int N2 = x->fft->N2;
	
	copy_shuffle_array(x);
	x->shuffle_count = argc;
	
	for (i=0; i < argc; i++) {
		ival = 2 * atom_getfloatarg(i,argc,argv);
		
		if ( ival < N2 && ival >= 0) {
			x->shuffle_mapping[ i ] = ival;
		}else {
			error("%s: %d is out of range",OBJECT_NAME, ival);
		}
	}
	x->frame_countdown = x->interpolation_frames;
	
	return;
}

void disarrain_setstate (t_disarrain *x, t_symbol *msg, short argc, t_atom *argv) {
	short i;
	int ival;
	int N2 = x->fft->N2;
	x->shuffle_count = argc;
	for (i=0; i < argc; i++) {
		ival = 2 *atom_getfloatarg(i,argc,argv);
		
		if ( ival < N2 && ival >= 0) {
			x->shuffle_mapping[ i ] = ival;
		} else {
			error("%s: %d is out of range",OBJECT_NAME, ival);
		}
	}
	return;
}

// REPORT CURRENT SHUFFLE STATUS
void disarrain_showstate (t_disarrain *x ) {
	
	t_atom *list_data = x->list_data;
	short i;
	for( i = 0; i < x->shuffle_count; i++ ) {
		atom_setlong(list_data+i,x->shuffle_mapping[i]/2);
	}
	outlet_list(x->list_outlet,0,x->shuffle_count,list_data);
	return;
}

void disarrain_dsp64(t_disarrain *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
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
        disarrain_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,disarrain_perform64,0,NULL);
}
