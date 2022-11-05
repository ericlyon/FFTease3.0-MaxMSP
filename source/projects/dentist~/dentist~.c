#include "fftease.h"

static t_class *dentist_class;


#define OBJECT_NAME "dentist~"

typedef struct _dentist
{

	t_pxobject x_obj;
	t_fftease *fft;
	short *bin_selection;
	short *last_bin_selection;
	int *active_bins;
	int tooth_count;
	int ramp_frames;
	int frames_left;
	double frame_duration;
	int max_bin;
	double topfreq;
	double funda;
	void *list_outlet;
	short direct_update;
	short mute;
	short bypass;
	t_atom *list_data;
	short interpolate_singles;
	double sync;
	long fftsize_attr;
	long overlap_attr;

} t_dentist;

void *dentist_new(t_symbol *msg, short argc, t_atom *argv);
t_int *offset_perform(t_int *w);
void dentist_assist(t_dentist *x, void *b, long m, long a, char *s);
void set_switch_bins (t_dentist *x, int i);
void reset_shuffle(t_dentist *x);
void dentist_showstate(t_dentist *x);
void dentist_direct_update(t_dentist *x, t_floatarg toggle);
void dentist_mute(t_dentist *x, t_floatarg toggle);
void dentist_setstate(t_dentist *x, t_symbol *msg, short argc, t_atom *argv);
void dentist_ramptime(t_dentist *x, t_floatarg ramp_ms);
int rand_index(int max);
void dentist_init(t_dentist *x);
void dentist_bins_pd (t_dentist *x, t_floatarg i);
void dentist_topfreq(t_dentist *x, t_floatarg f);
void dentist_free(t_dentist *x);
void dentist_toothcount(t_dentist *x, t_floatarg newcount);
void dentist_scramble(t_dentist *x);
void dentist_activate_bins(t_dentist *x, t_floatarg f);
void dentist_interpolate_singles(t_dentist *x, t_floatarg f);
//void dentist_overlap(t_dentist *x, t_floatarg o);
void dentist_winfac(t_dentist *x, t_floatarg o);
void dentist_fftinfo(t_dentist *x);
void dentist_mute(t_dentist *x, t_floatarg toggle);
//void dentist_fftsize(t_dentist *x, t_floatarg f);
void dentist_bypass(t_dentist *x, t_floatarg toggle);
t_max_err set_fftsize(t_dentist *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_dentist *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_dentist *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_dentist *x, void *attr, long *ac, t_atom **av);
void dentist_perform64(t_dentist *x, t_object *dsp64, double **ins,
                       long numins, double **outs,long numouts, long vectorsize,
                       long flags, void *userparam);
void dentist_dsp64(t_dentist *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);

int C74_EXPORT main(void)
{

	t_class *c;
	c = class_new("fftz.dentist~", (method)dentist_new, (method)dsp_free, sizeof(t_dentist),0,A_GIMME,0);
	class_addmethod(c,(method)dentist_dsp64, "dsp64", A_CANT, 0);
    class_addmethod(c,(method)set_switch_bins,"int",0);
	class_addmethod(c,(method)dentist_showstate,"showstate",0);
	class_addmethod(c,(method)dentist_mute,"mute",A_FLOAT, 0);
	class_addmethod(c,(method)dentist_bypass,"bypass", A_FLOAT, 0);
	class_addmethod(c,(method)dentist_setstate, "setstate", A_GIMME, 0);
	class_addmethod(c,(method)dentist_ramptime, "ramptime", A_FLOAT, 0);
	class_addmethod(c,(method)dentist_topfreq, "topfreq", A_FLOAT, 0);
	class_addmethod(c,(method)dentist_toothcount, "toothcount", A_FLOAT, 0);
	class_addmethod(c,(method)dentist_interpolate_singles, "interpolate_singles", A_FLOAT, 0);
	class_addmethod(c,(method)dentist_scramble, "scramble", 0);
	class_addmethod(c,(method)dentist_assist,"assist",A_CANT,0);
//	class_addmethod(c,(method)dentist_overlap, "overlap",  A_DEFFLOAT, 0);
	class_addmethod(c,(method)dentist_winfac, "winfac",  A_DEFFLOAT, 0);
//	class_addmethod(c,(method)dentist_fftsize, "fftsize",  A_FLOAT, 0);
	class_addmethod(c,(method)dentist_fftinfo, "fftinfo", 0);
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_dentist, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_dentist, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	dentist_class = c;
	
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

void dentist_interpolate_singles(t_dentist *x, t_floatarg f)
{
    x->interpolate_singles = (short)f;
	//    post("singles interp: %d",x->interpolate_singles);
}

void dentist_free(t_dentist *x)
{
	/* Pd might be having difficulty freeing its memory */

    dsp_free((t_pxobject *)x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
    sysmem_freeptr(x->bin_selection);
    sysmem_freeptr(x->active_bins);
    sysmem_freeptr(x->last_bin_selection);
    sysmem_freeptr(x->list_data);
	
}

void dentist_fftsize(t_dentist *x, t_floatarg f)
{	
	t_fftease *fft = x->fft;
	fft->N = (int) f;
	dentist_init(x);
}

void dentist_overlap(t_dentist *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	dentist_init(x);
}

void dentist_winfac(t_dentist *x, t_floatarg f)
{
	t_fftease *fft = x->fft;
	fft->winfac = (int) f;
	dentist_init(x);
}

void dentist_fftinfo( t_dentist *x )
{
	fftease_fftinfo( x->fft, OBJECT_NAME );
}


void dentist_direct_update( t_dentist *x, t_floatarg toggle)
{
	x->direct_update = (short)toggle;
}

void dentist_mute( t_dentist *x, t_floatarg toggle )
{
	x->mute = (short)toggle;
}


void dentist_bypass(t_dentist *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}


void dentist_assist (t_dentist *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal) Input ");
				break;
		}
	} else if (msg==2) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Output"); break;
			case 1: sprintf(dst,"(signal) Interpolation Sync"); break;
			case 2: sprintf(dst,"(list) Current Selected Bins"); break;			
		}
	}
}

void *dentist_new(t_symbol *msg, short argc, t_atom *argv)
{
    t_fftease *fft;
	t_dentist *x = (t_dentist *)object_alloc(dentist_class);
	x->list_outlet = listout((t_pxobject *)x);
	dsp_setup((t_pxobject *)x,1);
	outlet_new((t_pxobject *)x, "signal");
	outlet_new((t_pxobject *)x, "signal");

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();	
	
	x->topfreq = 3000; // an attribute candidate
	
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	
	attr_args_process(x, argc, argv);
	dentist_init(x);
	return x;
}

void dentist_topfreq(t_dentist *x, t_floatarg f)
{
	float funda = x->funda;
	float curfreq;
	t_fftease *fft = x->fft;
    if(f < 50 || f > fft->R/2.0)
        return;
	
    x->topfreq = f;
    x->max_bin = 1;  
    curfreq = 0;
    while(curfreq < x->topfreq) {
        ++(x->max_bin);
        curfreq += funda ;
    }
}

void dentist_init(t_dentist *x)
{
	int i;
	t_fftease *fft = x->fft;
	x->x_obj.z_disabled = 1;
	short initialized = fft->initialized;
 	fftease_init(fft);
	   
	if(!initialized){
		x->sync = 0;
		x->mute = 0;
		x->direct_update = 0;
		if(x->topfreq < 100)
			x->topfreq = 100.0;
		x->bin_selection = (short *) sysmem_newptrclear(fft->N * sizeof(short));
		x->active_bins = (int *) sysmem_newptrclear(fft->N2 * sizeof(int));
		x->last_bin_selection = (short *) sysmem_newptrclear(fft->N2 * sizeof(short)) ;
		x->list_data = (t_atom *) sysmem_newptrclear((fft->N + 2) * sizeof(t_atom));
		x->tooth_count = 0;
		x->interpolate_singles = 1;
		x->ramp_frames = 0;
		
	} else {
		x->bin_selection = (short *) sysmem_resizeptrclear((void *)x->bin_selection, fft->N * sizeof(short));
		x->active_bins = (int *) sysmem_resizeptrclear((void *)x->active_bins, fft->N2 * sizeof(int));
		x->last_bin_selection = (short *) sysmem_resizeptrclear((void *)x->last_bin_selection, fft->N2 * sizeof(short)) ;
		x->list_data = (t_atom *) sysmem_resizeptrclear((void *)x->list_data, (fft->N + 2) * sizeof(t_atom));
	}

	dentist_scramble(x);
	
    fft->mult = 1. / (double) fft->N;
    x->frame_duration = (double) fft->D / (double) fft->R;
    x->frames_left = 0;
    x->funda = (double) fft->R / (double) fft->N;
    x->max_bin = 1;  

    if(!x->funda){
    	error("%s: zero sampling rate!",OBJECT_NAME);
    	return;
    }
    x->max_bin = (int) (x->topfreq / x->funda);
    if(x->max_bin < 1)
    	x->max_bin = 1;
     
    for( i = 0; i < fft->N2; i++) {
        x->last_bin_selection[i] = x->bin_selection[i];
    }
	x->x_obj.z_disabled = 0;
}

void do_dentist(t_dentist *x)
{
	int	i;
	double oldfrac,newfrac;
	t_fftease *fft = x->fft;
	double *channel = fft->channel;
	int frames_left = x->frames_left;
	int ramp_frames = x->ramp_frames;
	short *bin_selection = x->bin_selection;
	short *last_bin_selection = x->last_bin_selection;
	int N2 = fft->N2;
	float sync = x->sync;

	fold(fft);	
	rdft(fft,1);
	leanconvert(fft);
	
	if(frames_left > 0 && ramp_frames > 0) {
		// INTERPOLATE ACCORDING TO POSITION IN RAMP
		oldfrac = (float) frames_left / (float) ramp_frames ;
		sync = newfrac = 1.0 - oldfrac;
		for( i = 0; i < N2 ; i++){
			if( (! bin_selection[i]) && (! last_bin_selection[i]) ){
				channel[i * 2]  = 0;
			} 
			else if (bin_selection[i]) {
				channel[i * 2]  *= newfrac;
			} 
			else if (last_bin_selection[i]) {
				channel[i * 2]  *= oldfrac;
			}
		}
		--frames_left;
		if( ! frames_left ){
			// Copy current to last
			for( i = 0; i < N2; i++) {
				last_bin_selection[i] = bin_selection[i];
			}
		}
	} else {
		for( i = 0; i < N2 ; i++){
			if( ! bin_selection[ i ] ){
				channel[ i * 2 ]  = 0;
			}
		}
		oldfrac = 0.0;
		sync = 1.0;
	}
	
	leanunconvert(fft);
	rdft(fft,-1);
	overlapadd(fft);
	x->frames_left = frames_left;
	x->sync = sync;
}

void dentist_perform64(t_dentist *x, t_object *dsp64, double **ins,
                         long numins, double **outs,long numouts, long vectorsize,
                         long flags, void *userparam)
{
	int	i,j;
	t_fftease *fft = x->fft;
	double *MSPInputVector = ins[0];
	double *MSPOutputVector = outs[0];
	double *sync_vec = outs[1];
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
        
		do_dentist(x);
        
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
            
			do_dentist(x);
			
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
            
			do_dentist(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
	for(i = 0; i < MSPVectorSize; i++){
		sync_vec[i] = x->sync;
	}
	fft->operationCount = operationCount;
}		


void set_switch_bins (t_dentist *x, int i)
{
	if( i < 0 ){
		i = 0;
	}
	if( i > x->fft->N2 ) {
		i = x->fft->N2;
	}
	x->tooth_count = i;
	if( x->direct_update ){
		reset_shuffle(x);
	}
	return;
}
//identical function for Pd
void dentist_bins_pd (t_dentist *x, t_floatarg i)
{
	if( i < 0 ){
		i = 0;
	}
	if( i > x->fft->N2 ) {
		i = x->fft->N2;
	}
	x->tooth_count = (int)i;
	if(x->direct_update){
		reset_shuffle(x);
	}
	return;
}

// experimental, not to be used
void dentist_activate_bins(t_dentist *x, t_floatarg f)
{
    if(f < 0 || f > x->max_bin){
#if PD
        post("* %d bin out of range",(int)f);
#endif        
        return;
    }
    x->tooth_count = (int)f;
}

void dentist_scramble(t_dentist *x)
{
	short *last_bin_selection = x->last_bin_selection;
	short *bin_selection = x->bin_selection;
	int *active_bins = x->active_bins;
	int N2 = x->fft->N2;
	int i,tmp,b1,b2;
	int maxswap = x->max_bin;

	for(i=0; i<N2; i++){
		bin_selection[i] = 0;
		active_bins[i] = i;
	}
	while(maxswap > 0){
		b1 = maxswap;
		b2 = rand_index(maxswap);
		tmp = active_bins[b1];
		active_bins[b1] = active_bins[b2];
		active_bins[b2] = tmp;
		--maxswap;
	}
	for( i = 0; i < x->tooth_count; i++ ) {
		x->bin_selection[active_bins[i]] = 1;
	}
	x->frames_left = x->ramp_frames;
	if(! x->ramp_frames) {
		for(i = 0; i < N2; i++){
			last_bin_selection[i] = bin_selection[i];
		}
	}    
}


void dentist_toothcount(t_dentist *x, t_floatarg newcount)
{
	int i;
	int nc = (int) newcount;
	int tooth_count = x->tooth_count;

	if(nc < 0 || nc > x->fft->N2){

		error("dentist~: %d out of range",nc);
		return;
	}

	if(nc < x->tooth_count){
		for(i = nc; i < tooth_count; i++){
			x->bin_selection[x->active_bins[i]] = 0;
		}
	} else {
		for(i = tooth_count; i < nc; i++){
			x->bin_selection[x->active_bins[i]] = 1;
		}
	}
	// if immediate reset
	if(x->interpolate_singles){
		//  post("setting frames left");
		x->frames_left = x->ramp_frames;
	}
	if(! x->ramp_frames) {
		for(i = 0; i < x->fft->N2; i++){
			x->last_bin_selection[i] = x->bin_selection[i];
		}
	}
	x->tooth_count = nc;
}


void reset_shuffle (t_dentist *x)
{
	int i;
	int max;
	
	max = x->max_bin;
	for(i = 0; i < x->fft->N2; i++){
		x->last_bin_selection[i] = x->bin_selection[i];
		x->bin_selection[i] = 0;
	}
	for(i = 0; i < x->max_bin; i++) {
		x->active_bins[i] = rand_index(max);
		x->bin_selection[x->active_bins[i]] = 1;
	}
	x->frames_left = x->ramp_frames;
	if(! x->ramp_frames) { // Ramp Off - Immediately set last to current
		for( i = 0; i < x->fft->N2; i++ ){
			x->last_bin_selection[ i ] = x->bin_selection[ i ];
		}
	}
}

int rand_index(int max) {
	return (rand() % max);
}

void dentist_setstate (t_dentist *x, t_symbol *msg, short argc, t_atom *argv) {
	short i;
	int selex;
	
	short *last_bin_selection = x->last_bin_selection;
	short *bin_selection = x->bin_selection;
	int *active_bins = x->active_bins;
	x->tooth_count = argc;
	int N2 = x->fft->N2;
	
	for(i = 0; i < N2; i++){
		last_bin_selection[i] = bin_selection[i]; // needed here
		bin_selection[i] = 0;
	}
	
	for (i=0; i < argc; i++) {
		selex = atom_getfloatarg(i,argc,argv);
		if (selex < N2 && selex >= 0 ) {
			active_bins[i] = selex;
			bin_selection[selex] = 1;
		} else {
			post ("%d out of range bin",selex);
		}		
	}
	
	
	x->frames_left = x->ramp_frames;
	if(! x->ramp_frames) { // Ramp Off - Immediately set last to current
		for(i = 0; i < N2; i++){
			last_bin_selection[i] = bin_selection[i];
		}
	}
	
	return;
}
void dentist_ramptime (t_dentist *x, t_floatarg ramp_ms) {
	
	if(ramp_ms <= 0){
		x->ramp_frames = 0;
		return;
	}
	
	x->frames_left = x->ramp_frames = (int)(ramp_ms * .001 / x->frame_duration);
	return;
}
// REPORT CURRENT SHUFFLE STATUS
void dentist_showstate (t_dentist *x) {
	
	t_atom *list_data = x->list_data;
	
	short i, count;
	float data;
	
	count = 0;
	for(i = 0; i < x->tooth_count; i++ ) {
		data = x->active_bins[i];
		atom_setlong(list_data+count,x->active_bins[i]);
		++count;
	}	
	outlet_list(x->list_outlet,0,x->tooth_count,list_data);
	return;
}

t_max_err get_fftsize(t_dentist *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_dentist *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		dentist_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_dentist *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_dentist *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		dentist_init(x);
	}
	return MAX_ERR_NONE;
}


void dentist_dsp64(t_dentist *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
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
        dentist_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,dentist_perform64,0,NULL);
}


