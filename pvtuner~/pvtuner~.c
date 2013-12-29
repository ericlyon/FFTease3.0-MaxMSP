#include "fftease.h"

static t_class *pvtuner_class;

#define OBJECT_NAME "pvtuner~"

#define MAXTONES (8192)
#define BASE_FREQ (27.5)	/* low A  */
#define DIATONIC 0
#define EASTERN 1
#define MINOR 2
#define EQ12 3
#define PENTATONIC 4
#define MAJOR_ADDED_SIXTH 5
#define MINOR_ADDED_SIXTH 6
#define ADDED_SIXTH_MAJOR 5
#define ADDED_SIXTH_MINOR 6
#define MAJOR_SEVENTH_CHORD 7
#define MINOR_SEVENTH_CHORD 8
#define DOMINANT_SEVENTH_CHORD 9
#define EQ8 10
#define PENTACLUST 11
#define QUARTERCLUST 12
#define EQ5 13
#define SLENDRO 14
#define PELOG 15
#define IMPORTED_SCALE 16
#define EQN 17

typedef struct {
	double *pitchgrid;
	int scale_steps; // total number of members
	short current_scale;
} t_pvtuner_scale;

typedef struct _pvtuner
{
	t_pxobject x_obj;
	t_fftease *fft;
	int lo_bin;
	int hi_bin;
	int hi_tune_bin;
	double topfreq;
	double curfreq;

	//
	int bypass;
	int pitch_connected;
	int synt_connected;
	short connections[4]; // 4 inlets, only look at last 3 though
	// TUNING
	float *pitchgrid;
	float pbase;
	int scale_steps;
	short current_scale;
	short mute;
	//
	double lofreq;
	double hifreq;
	double tabscale;
	int scale_len;
	short verbose;
	long fftsize_attr;
	long overlap_attr;
	// interpolation flag
	long scale_interpolation; // set to zero by default
	double interpolation_dur; // set to 1 second
	long interpolation_frames; // == duration / (D/R)
	long interpolation_countdown; // count frames for interpolation
	double ip; // interpolation point
	t_pvtuner_scale *this_scale;
	t_pvtuner_scale *last_scale;
} t_pvtuner;


double closestf(double test, double *arr) ;
void pvtuner_diatonic( t_pvtuner *x );
void pvtuner_eastern( t_pvtuner *x );
void pvtuner_minor( t_pvtuner *x );
void pvtuner_eq12( t_pvtuner *x );
void pvtuner_pentatonic( t_pvtuner *x );
void pvtuner_major_added_sixth( t_pvtuner *x );
void pvtuner_minor_added_sixth( t_pvtuner *x );
void pvtuner_major_seventh_chord( t_pvtuner *x );
void pvtuner_minor_seventh_chord( t_pvtuner *x );
void pvtuner_dominant_seventh_chord( t_pvtuner *x );
void pvtuner_eq8( t_pvtuner *x );
void pvtuner_pentaclust( t_pvtuner *x );
void pvtuner_quarterclust( t_pvtuner *x );
void pvtuner_eq5( t_pvtuner *x );
void pvtuner_slendro( t_pvtuner *x );
void pvtuner_pelog( t_pvtuner *x );
void pvtuner_update_imported( t_pvtuner *x );
void pvtuner_init(t_pvtuner *x);
void *pvtuner_new(t_symbol *s, int argc, t_atom *argv);
void pvtuner_import_scale(t_pvtuner *x, t_symbol *filename);
void pvtuner_list (t_pvtuner *x, t_symbol *msg, short argc, t_atom *argv);
void pvtuner_toptune( t_pvtuner *x, t_floatarg f );
void pvtuner_frequency_range(t_pvtuner *x, t_floatarg lo, t_floatarg hi);
void pvtuner_basefreq( t_pvtuner *x, t_floatarg bassfreq);
void pvtuner_free(t_pvtuner *x);
void pvtuner_assist (t_pvtuner *x, void *b, long msg, long arg, char *dst);
void pvtuner_bypass(t_pvtuner *x, t_floatarg state);
void pvtuner_mute(t_pvtuner *x, t_floatarg state);
void pvtuner_float(t_pvtuner *x, double f) ;
// void pvtuner_toptune(t_pvtuner *x, t_floatarg f);
void pvtuner_list (t_pvtuner *x, t_symbol *msg, short argc, t_atom *argv);
void pvtuner_fftinfo(t_pvtuner *x);
//void pvtuner_overlap(t_pvtuner *x, t_floatarg f);
void pvtuner_winfac(t_pvtuner *x, t_floatarg f);
void pvtuner_binfo(t_pvtuner *x);
//void pvtuner_fftsize(t_pvtuner *x, t_floatarg f);
void pvtuner_eqn(t_pvtuner *x, t_floatarg steps);
t_max_err set_fftsize(t_pvtuner *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_pvtuner *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_pvtuner *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_pvtuner *x, void *attr, long *ac, t_atom **av);
t_max_err set_lowfreq(t_pvtuner *x, void *attr, long ac, t_atom *av);
t_max_err set_highfreq(t_pvtuner *x, void *attr, long ac, t_atom *av);
t_max_err get_lowfreq(t_pvtuner *x, void *attr, long *ac, t_atom **av);
t_max_err get_highfreq(t_pvtuner *x, void *attr, long *ac, t_atom **av);
void pvtuner_dsp64(t_pvtuner *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void pvtuner_perform64(t_pvtuner *x, t_object *dsp64, double **ins,
                       long numins, double **outs,long numouts, long vectorsize,
                       long flags, void *userparam);
int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.pvtuner~", (method)pvtuner_new, (method)pvtuner_free, sizeof(t_pvtuner), 0,A_GIMME,0);
	
	class_addmethod(c,(method)pvtuner_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)pvtuner_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)pvtuner_bypass,"bypass",A_FLOAT,0);
	class_addmethod(c,(method)pvtuner_mute,"mute",A_FLOAT,0);
	class_addmethod(c,(method)pvtuner_basefreq,"basefreq",A_DEFFLOAT,0);
	class_addmethod(c,(method)pvtuner_diatonic,"diatonic",0);
	class_addmethod(c,(method)pvtuner_eastern,"eastern",0);
	class_addmethod(c,(method)pvtuner_minor,"minor",0);
	class_addmethod(c,(method)pvtuner_eq12,"eq12",0);
	class_addmethod(c,(method)pvtuner_pentatonic,"pentatonic",0);
	class_addmethod(c,(method)pvtuner_major_added_sixth,"added_sixth_major",0);
	class_addmethod(c,(method)pvtuner_minor_added_sixth,"added_sixth_minor",0);
	class_addmethod(c,(method)pvtuner_major_seventh_chord,"major_seventh_chord",0);
	class_addmethod(c,(method)pvtuner_minor_seventh_chord,"minor_seventh_chord",0);
	class_addmethod(c,(method)pvtuner_dominant_seventh_chord,"dominant_seventh_chord",0);
	class_addmethod(c,(method)pvtuner_eq8,"eq8",0);               
	class_addmethod(c,(method)pvtuner_pentaclust,"pentaclust",0);
	class_addmethod(c,(method)pvtuner_quarterclust,"quarterclust",0);   
	class_addmethod(c,(method)pvtuner_eq5,"eq5",0); 
	class_addmethod(c,(method)pvtuner_eqn,"eqn",A_FLOAT, 0);
	class_addmethod(c,(method)pvtuner_slendro,"slendro",0);
	class_addmethod(c,(method)pvtuner_pelog,"pelog",0); 
	class_addmethod(c,(method)pvtuner_list,"list",A_GIMME,0);    
//	class_addmethod(c,(method)pvtuner_toptune,"toptune",A_DEFFLOAT,0);
	class_addmethod(c,(method)pvtuner_frequency_range,"frequency_range",A_FLOAT,A_FLOAT, 0);    
	class_addmethod(c,(method)pvtuner_winfac,"winfac",A_DEFFLOAT,0);
	class_addmethod(c,(method)pvtuner_fftinfo,"fftinfo",0);
	class_addmethod(c,(method)pvtuner_binfo,"binfo",0);
	class_addmethod(c,(method)pvtuner_float,"float",A_FLOAT,0);

	CLASS_ATTR_LONG(c, "scale_interpolation", 0, t_pvtuner, scale_interpolation);
	CLASS_ATTR_ENUMINDEX(c, "scale_interpolation", 0, "Off On");
	CLASS_ATTR_LABEL(c, "scale_interpolation", 0, "Scale Interpolation");
	
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_pvtuner, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_pvtuner, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	
	CLASS_ATTR_FLOAT(c, "lowfreq", 0, t_pvtuner, lofreq);
	CLASS_ATTR_ACCESSORS(c, "lowfreq", (method)get_lowfreq, (method)set_lowfreq);
	CLASS_ATTR_LABEL(c, "lowfreq", 0, "Minimum Frequency");	
	
	CLASS_ATTR_FLOAT(c, "highfreq", 0, t_pvtuner, hifreq);
	CLASS_ATTR_ACCESSORS(c, "highfreq",(method)get_highfreq, (method)set_highfreq);
	CLASS_ATTR_LABEL(c, "highfreq", 0, "Maximum Frequency");	
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	CLASS_ATTR_ORDER(c, "lowfreq",    0, "3");
	CLASS_ATTR_ORDER(c, "highfreq",    0, "4");
	CLASS_ATTR_ORDER(c, "scale_interpolation",    0, "5");
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	pvtuner_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

void *pvtuner_new(t_symbol *s, int argc, t_atom *argv)
{
    t_fftease *fft;
	t_pvtuner *x = (t_pvtuner *)object_alloc(pvtuner_class);
	dsp_setup((t_pxobject *)x,4);
	outlet_new((t_pxobject *)x, "signal");
	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0;
	x->fft->R = sys_getsr();
	x->fft->MSPVectorSize = sys_getblksize();
	x->verbose = 0;
	x->lofreq = 0;
	x->hifreq = 10000;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;	
	// check the called functions -> that may be where this whole thing is crashing
	attr_args_process(x, argc, argv);
	pvtuner_init(x);
	return x;
}
/* Copies current scale (this_scale) to next scale (next_scale)
 */
void pvtuner_copy_scale(t_pvtuner *x)
{
	int i;
	t_pvtuner_scale *this = x->this_scale;
	t_pvtuner_scale *last = x->last_scale;
	last->scale_steps = this->scale_steps;
	last->current_scale = this->current_scale;
	for(i = 0; i < last->scale_steps; i++){
		last->pitchgrid[i] = this->pitchgrid[i];
	}
}

void pvtuner_init(t_pvtuner *x)
{
	int mem;
	
	t_fftease *fft = x->fft;
	if( fft->R <= 0.0 ){
		post("%s: zero sample rate reported - initialization deferred", OBJECT_NAME);
		x->fft->initialized = 0; // failed initialization
		return;
	}
	x->x_obj.z_disabled = 1;
	/* 
	 this will require a major rebuild - all of the scales must be applied multiple
	 times, so their functions must be redesigned!!!
	 */

	if(!x->fft->initialized){
		x->bypass = 0;
		x->mute = 0;	
		mem = (MAXTONES+1)*sizeof(float);
		// next 2 lines will be commented out
		x->pitchgrid = (float *) calloc(mem,1);	
		x->pbase = BASE_FREQ;
		
		// here's the new stuff
		x->this_scale = (t_pvtuner_scale *) sysmem_newptrclear(sizeof(t_pvtuner_scale));
		x->last_scale = (t_pvtuner_scale *) sysmem_newptrclear(sizeof(t_pvtuner_scale));
		x->this_scale->pitchgrid = (double *) sysmem_newptrclear(mem);
		x->last_scale->pitchgrid = (double *) sysmem_newptrclear(mem);
		x->this_scale->scale_steps = 0;
		x->last_scale->scale_steps = 0;
		pvtuner_diatonic(x);// default scale (rebuilt for new scale structure)
		pvtuner_copy_scale(x); // now both this and next scale are diatonic
/*		
		x->interpolation_dur = 2.0; // set to 1 second
		x->interpolation_countdown = 0; // count frames for interpolation
*/
	} 

	fftease_init(fft);
	if( x->fft->D && x->fft->R ){
		/*
		x->interpolation_frames = x->interpolation_dur / ((float)x->fft->D / (float)x->fft->R);
		post("interpolation frames: %d", x->interpolation_frames);
		*/
	} else {
		post("pvtuner~: dire warning!");
		return;
		// dire warning
	}
	fftease_oscbank_setbins(fft,x->lofreq, x->hifreq);

	x->hi_tune_bin = fft->hi_bin;
	x->x_obj.z_disabled = 0;
}

void pvtuner_toptune(t_pvtuner *x, t_floatarg f)
{
	int tbin;
	double curfreq;
	double fundamental = x->fft->c_fundamental;
	t_fftease *fft = x->fft;
	tbin = 1;  
	curfreq = 0;
	
	if( f < 0 || f > x->fft->R / 2.0 ){
		error("frequency %f out of range", f);
		return;
	}
	while( curfreq < f ) {
		++tbin;
		curfreq += fundamental ;
	}
	if( tbin > fft->lo_bin && tbin <= fft->hi_bin ){
		x->hi_tune_bin = tbin;
	} else {
		error("pvtuner~: bin %d out of range", tbin);
	}
	
}


void pvtuner_list (t_pvtuner *x, t_symbol *msg, short argc, t_atom *argv)
{
	double *pitchgrid = x->this_scale->pitchgrid;
    double dval;
	t_pvtuner_scale *s = x->this_scale;
	int i = 0;
	
	if( ! atom_getfloatarg(i,argc,argv) ){
		error("either zero length scale or 0.0 (prohibited) is first value");
		return;
	}
	pvtuner_copy_scale(x);
	/* first set every value to maximum */
	for(i=0; i < MAXTONES; i++){
		pitchgrid[i] = (double)x->fft->R / 2.0;
	}
    // now read scale
	for( i = 0; i < argc; i++ ){
        atom_arg_getdouble(&dval, i, argc, argv);
		pitchgrid[ i ] = dval;
	}
	/*
	lastval = atom_getfloatarg(argc - 1,argc,argv);
	
	for(i = argc; i <  MAXTONES; i++){
		pitchgrid[i] = lastval;
	}
	*/
	s->scale_steps = argc;
	s->current_scale = IMPORTED_SCALE;
}

void pvtuner_binfo(t_pvtuner *x)
{
	t_fftease *fft = x->fft;
	post("%s: frequency targets: %f %f", OBJECT_NAME, x->lofreq, x->hifreq);
	post("synthesizing %d bins, from %d to %d",(fft->hi_bin - fft->lo_bin), fft->lo_bin, fft->hi_bin);
}

void pvtuner_frequency_range(t_pvtuner *x, t_floatarg lo, t_floatarg hi)
{
	t_fftease *fft = x->fft;
	x->lofreq = lo ;
	x->hifreq = hi;
	
	
	if( lo >= hi ){
		error("low frequency must be lower than high frequency");
		return;
	}
	x->curfreq = 0;
	fft->hi_bin = 0;
	
	while( x->curfreq < x->hifreq ) {
		++(fft->hi_bin);
		x->curfreq += x->fft->c_fundamental ;
	}
	
	
	x->curfreq = 0;
	fft->lo_bin = 0;  
	while( x->curfreq < x->lofreq ) {
		++(fft->lo_bin);
		x->curfreq += x->fft->c_fundamental ;
	}
	
}

void do_pvtuner(t_pvtuner *x)
{
	t_fftease *fft = x->fft;
	int freq,chan;
	double *channel = fft->channel;
	double *this_pitchgrid = x->this_scale->pitchgrid;
	double *last_pitchgrid = x->last_scale->pitchgrid;
	int hi_bin = fft->hi_bin;
	int lo_bin = fft->lo_bin;
	int hi_tune_bin = x->hi_tune_bin;
	long scale_interpolation = x->scale_interpolation;
	double ip = x->ip;
	double freq_this, freq_last;
	fold(fft);   
	rdft(fft, 1);
	convert(fft);
	// static case
	if( scale_interpolation == 0) {
		for ( chan = lo_bin; chan < hi_bin; chan++ ) {
			freq = (chan * 2) + 1;
			if(chan <= hi_tune_bin){
				channel[freq] = closestf(channel[freq], this_pitchgrid);
			}
		}
	}
	// interpolated case
	else if( scale_interpolation == 1) {
		// clip
		if( ip < 0 )
			ip = 0;
		if( ip > 1 )
			ip = 1;
		// degenerate cases first
		if( ip == 0 ){
			for ( chan = lo_bin; chan < hi_bin; chan++ ) {
				freq = (chan * 2) + 1;
				if(chan <= hi_tune_bin){
					channel[freq] = closestf(channel[freq], last_pitchgrid);
				}
			}
		} 
		else if ( ip == 1){
			for ( chan = lo_bin; chan < hi_bin; chan++ ) {
				freq = (chan * 2) + 1;
				if(chan <= hi_tune_bin){
					channel[freq] = closestf(channel[freq], this_pitchgrid);
				}
			}
		}
		else {
			for ( chan = lo_bin; chan < hi_bin; chan++ ) {
				freq = (chan * 2) + 1;
				if(chan <= hi_tune_bin){
					freq_this = closestf(channel[freq], this_pitchgrid);
					freq_last = closestf(channel[freq], last_pitchgrid);
					channel[freq] = freq_last + (freq_this - freq_last) * ip; // linear interpolation
				}
			}
		}
	}
	oscbank(fft);
}


void pvtuner_perform64(t_pvtuner *x, t_object *dsp64, double **ins,
                       long numins, double **outs,long numouts, long vectorsize,
                       long flags, void *userparam)
{
	int 	i,j;
	double *MSPInputVector = ins[0];
	double *pitchfac = ins[1];
	double *synth_thresh = ins[2];
	double *ip = ins[3];
	double *MSPOutputVector = outs[0];
	t_fftease *fft = x->fft;
	
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
	short *connections = x->connections; 
	
	if (x->bypass) {
        for(i=0; i < vectorsize; i++){ MSPOutputVector[i] = 0.0; }
		return;
	}
	
	if (x->mute || x->x_obj.z_disabled) {
		memset(MSPOutputVector, 0.0, MSPVectorSize * sizeof(float));
		return;
	}
	if( connections[1] ) {
  		fft->P  = *pitchfac ; 
  	}
  	if ( connections[2] ) {
  		fft->synt = *synth_thresh ;
  	}
  	if ( connections[3] ) {
  		x->ip = *ip;
  	}
	// HERE IS THE GOOD STUFF
    //do_pvtuner( x );
//	post("perform: pitchfac %f, synt %f, interpolation point %f", fft->P, fft->synt, x->ip);
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_pvtuner(x);
        
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
            
			do_pvtuner(x);
			
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
            
			do_pvtuner(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}		


void pvtuner_basefreq( t_pvtuner *x, t_floatarg bassfreq)
{
	if( bassfreq < 1 ){
		bassfreq = 1;
		post("%s: base frequency may not go lower than 1 Hz", OBJECT_NAME);
	}
	if( bassfreq > 10000. ){
		bassfreq = 10000.;
		post("%s: base frequency may not go higher than 10000 Hz", OBJECT_NAME);
	}
	x->pbase = bassfreq;
	if( x->current_scale == IMPORTED_SCALE ){
		pvtuner_update_imported( x );
	}
	else if( x->current_scale == DIATONIC ){
		pvtuner_diatonic( x );
	} 
	else if( x->current_scale == EASTERN) {
		pvtuner_eastern( x );
	}
	else if( x->current_scale == MINOR) {
		pvtuner_minor( x );
	}
	else if( x->current_scale == EQ12) {
		pvtuner_eq12( x );
	}
	else if( x->current_scale == PENTATONIC) {
		pvtuner_pentatonic( x );
	}
	else if( x->current_scale == MAJOR_ADDED_SIXTH) {
		pvtuner_major_added_sixth( x );
	}
	else if( x->current_scale == MINOR_ADDED_SIXTH) {
		pvtuner_minor_added_sixth( x );
	}
	else if( x->current_scale == MAJOR_SEVENTH_CHORD) {
		pvtuner_major_seventh_chord( x );
	}
	else if( x->current_scale == MINOR_SEVENTH_CHORD) {
		pvtuner_minor_seventh_chord( x );
	}
	else if( x->current_scale == DOMINANT_SEVENTH_CHORD) {
		pvtuner_dominant_seventh_chord( x );
	}
	else if( x->current_scale == EQ8) {
		pvtuner_eq8( x );
	}
	else if( x->current_scale == PENTACLUST) {
		pvtuner_pentaclust( x );
	}
	else if( x->current_scale == QUARTERCLUST ) {
		pvtuner_quarterclust( x );
	}
	else if( x->current_scale == EQ5 ) {
		pvtuner_eq5( x );
	}
	else if( x->current_scale == SLENDRO ) {
		pvtuner_slendro( x );
	}
	else if( x->current_scale == PELOG ) {
		pvtuner_pelog( x );
	}	
	else {
		post("unknown scale");
	}
}

void pvtuner_free(t_pvtuner *x)
{
	dsp_free( (t_pxobject *) x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
	sysmem_freeptr(x->this_scale);
	sysmem_freeptr(x->last_scale);
}

void pvtuner_assist (t_pvtuner *x, void *b, long msg, long arg, char *dst)
{
	if (msg==ASSIST_INLET) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input"); break;
			case 1: sprintf(dst,"(signal/float) Pitch Modification Factor"); break;
			case 2: sprintf(dst,"(signal/float) Synthesis Threshold"); break;
			case 3: sprintf(dst,"(signal/float) Interpolation Point"); break;
		}
	} else if (msg==ASSIST_OUTLET) {
		sprintf(dst,"(signal) Output");
	}
}

void pvtuner_update_imported( t_pvtuner *x ){
	double *pitchgrid = x->this_scale->pitchgrid;
	double factor; 
	int i;

	if( pitchgrid[0] <= 0.0){
		error("%s: illegal first value of scale: %f",OBJECT_NAME,pitchgrid[0]);
		return;
	}
	
	factor = x->pbase / pitchgrid[0];
	pvtuner_copy_scale(x); // copies this scale to next
	for( i = 0; i < x->scale_len; i++ )
		pitchgrid[i] *= factor;
}

void pvtuner_diatonic( t_pvtuner *x ){
	int i, j;
	int octsteps = 7;
	t_pvtuner_scale *s = x->this_scale;
	
	pvtuner_copy_scale(x); // copies this scale to next

	s->pitchgrid[0] = x->pbase;
	s->pitchgrid[1] = x->pbase * (9./8.);
	s->pitchgrid[2] = x->pbase * (5./4.);
	s->pitchgrid[3] = x->pbase * (4./3.);
	s->pitchgrid[4] = x->pbase * (3./2.);
	s->pitchgrid[5] = x->pbase * (27./16.);
	s->pitchgrid[6] = x->pbase * (15./8.);
	s->scale_steps = 7;
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < octsteps; j++ ){
			s->pitchgrid[ i * octsteps + j] = s->pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = DIATONIC ;
	s->scale_steps = 70; // 10 * 7
}

void pvtuner_minor( t_pvtuner *x ){
	int i, j;
	int octsteps = 7;
	t_pvtuner_scale *s = x->this_scale;
	double *pitchgrid = x->this_scale->pitchgrid;
	
	pvtuner_copy_scale(x); // copies this scale to next
	
	pitchgrid[0] = x->pbase;
	pitchgrid[1] = x->pbase * (9./8.);
	pitchgrid[2] = x->pbase * (6./5.);
	pitchgrid[3] = x->pbase * (4./3.);
	pitchgrid[4] = x->pbase * (3./2.);
	pitchgrid[5] = x->pbase * (8./5.);
	pitchgrid[6] = x->pbase * (9./5.);
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < octsteps; j++ ){
			pitchgrid[ i * octsteps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = MINOR;
	s->scale_steps = 70;
	
}

void pvtuner_pentatonic( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	double *pitchgrid = x->this_scale->pitchgrid;
	int octsteps = 5;
	pvtuner_copy_scale(x); 
	pitchgrid[0] = x->pbase;
	pitchgrid[1] = x->pbase * (9./8.);
	pitchgrid[2] = x->pbase * (81./64.);
	pitchgrid[3] = x->pbase * (3./2.);
	pitchgrid[4] = x->pbase * (27./16.);
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < octsteps; j++ ){
			pitchgrid[ i * octsteps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = PENTATONIC;
	s->scale_steps = 50;
}

void pvtuner_eq12( t_pvtuner *x ){
	int i, j;
	double expon;
	int octsteps = 12;
	double pbase = x->pbase;
	t_pvtuner_scale *s = x->this_scale;
	double *pitchgrid = x->this_scale->pitchgrid;
	
	pvtuner_copy_scale(x); // copies this scale to next
	
	// now refill this scale
	pitchgrid[0] = pbase;
	for( i = 0; i < octsteps; i++ ){
		expon = (float) i / (float) octsteps;
		pitchgrid[i] = pbase * pow(2.0,expon);
	}
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < octsteps; j++ ){
			pitchgrid[ i * octsteps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = EQ12;
	s->scale_steps = 120;
}

void pvtuner_major_added_sixth( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	double *pitchgrid = x->this_scale->pitchgrid;
	double pbase = x->pbase;
	int octsteps = 4;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.259921;
	pitchgrid[2] = pbase * 1.498307;
	pitchgrid[3] = pbase * 1.681793;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < octsteps; j++ ){
			pitchgrid[ i * octsteps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = MAJOR_ADDED_SIXTH;
	s->scale_steps = 40;
}

void pvtuner_minor_added_sixth( t_pvtuner *x ){
	int i, j;
	// float *pitchgrid = x->pitchgrid;
	double pbase = x->pbase;
	t_pvtuner_scale *s = x->this_scale;
	double *pitchgrid = x->this_scale->pitchgrid;
	int octsteps = 4;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.189207;
	pitchgrid[2] = pbase * 1.498307;
	pitchgrid[3] = pbase * 1.587401;
	// scale_steps = 4 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < octsteps; j++ ){
			pitchgrid[ i * octsteps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = MINOR_ADDED_SIXTH;
	s->scale_steps = 40;
}

void pvtuner_major_seventh_chord( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	double *pitchgrid = s->pitchgrid;
	double pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.25;
	pitchgrid[2] = pbase * 1.5;
	pitchgrid[3] = pbase * 1.875;
	scale_steps = 4 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = MAJOR_SEVENTH_CHORD;
	s->scale_steps = 40;
}

void pvtuner_minor_seventh_chord( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	double *pitchgrid = s->pitchgrid;
	double pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.2;
	pitchgrid[2] = pbase * 1.5;
	pitchgrid[3] = pbase * 1.781797;
	scale_steps = 4 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = MINOR_SEVENTH_CHORD;
	s->scale_steps = 40;
}

void pvtuner_dominant_seventh_chord( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	double *pitchgrid = s->pitchgrid;
	double pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.25;
	pitchgrid[2] = pbase * 1.5;
	pitchgrid[3] = pbase * 1.781797;
	scale_steps = 4 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = DOMINANT_SEVENTH_CHORD;
	s->scale_steps = 40;
}
void pvtuner_eqn( t_pvtuner *x, t_floatarg steps )
{
int dexter = 0;
	if(steps <= 0.0){
		return;
	}
	t_pvtuner_scale *s = x->this_scale;
	double *pitchgrid = x->this_scale->pitchgrid;
	
	float pbase = x->pbase;
	float factor = pow(2.0, (1.0/steps) );
	
	pvtuner_copy_scale(x);
	
	while(pbase < (x->fft->R / 2.0) && dexter < MAXTONES ){
		pitchgrid[dexter] = pbase;
		pbase = pbase * factor;
		dexter = dexter + 1;
	}
	s->scale_steps = dexter;
	s->current_scale = EQN;
}

void pvtuner_eq8( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	double *pitchgrid = s->pitchgrid;
	double pbase = x->pbase;
	int octsteps = 8;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.090508;
	pitchgrid[2] = pbase * 1.189207;
	pitchgrid[3] = pbase * 1.296840;
	pitchgrid[4] = pbase * 1.414214;
	pitchgrid[5] = pbase * 1.542211;
	pitchgrid[6] = pbase * 1.681793;
	pitchgrid[7] = pbase * 1.834008;
	
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < octsteps; j++ ){
			pitchgrid[ i * octsteps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = EQ8;
	s->scale_steps = 80;
}

void pvtuner_pentaclust( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	double *pitchgrid = s->pitchgrid;
	double pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.059463;
	pitchgrid[2] = pbase * 1.122462;
	pitchgrid[3] = pbase * 1.189207;
	pitchgrid[4] = pbase * 1.259921;
	
	scale_steps = 5 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = PENTACLUST;
	s->scale_steps = 50;
}

void pvtuner_quarterclust( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	double *pitchgrid = s->pitchgrid;
	double pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.029302;
	pitchgrid[2] = pbase * 1.059463;
	pitchgrid[3] = pbase * 1.090508;
	pitchgrid[4] = pbase * 1.122462;
	pitchgrid[5] = pbase * 1.155353;
	pitchgrid[6] = pbase * 1.189207;
	pitchgrid[7] = pbase * 1.224054;
	
	scale_steps = 8 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = QUARTERCLUST;
	s->scale_steps = 80;
}

void pvtuner_eq5( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	double *pitchgrid = s->pitchgrid;
	double pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.148698;
	pitchgrid[2] = pbase * 1.319508;
	pitchgrid[3] = pbase * 1.515717;
	pitchgrid[4] = pbase * 1.741101;
	
	scale_steps = 5 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = EQ5;
	s->scale_steps = 50;
}

void pvtuner_pelog( t_pvtuner *x ){
	int i, j;
	double *pitchgrid = x->this_scale->pitchgrid;
	t_pvtuner_scale *s = x->this_scale;
	double pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.152;
	pitchgrid[2] = pbase * 1.340;
	pitchgrid[3] = pbase * 1.532;
	pitchgrid[4] = pbase * 1.756;
	scale_steps = 5 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = PELOG;
	s->scale_steps = 50;
}

void pvtuner_slendro( t_pvtuner *x ){
	int i, j;
	double *pitchgrid = x->this_scale->pitchgrid;
	t_pvtuner_scale *s = x->this_scale;
	double pbase = x->pbase;
	int scale_steps;
	
	pvtuner_copy_scale(x);
	
	pitchgrid[0] = pbase;
	pitchgrid[1] = pbase * 1.104;
	pitchgrid[2] = pbase * 1.199;
	pitchgrid[3] = pbase * 1.404;
	pitchgrid[4] = pbase * 1.514;
	pitchgrid[5] = pbase * 1.615;    
	pitchgrid[6] = pbase * 1.787;    
	scale_steps = 7 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < scale_steps; j++ ){
			pitchgrid[ i * scale_steps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = SLENDRO;
	s->scale_steps = 70;
}
void pvtuner_eastern( t_pvtuner *x ){
	int i, j;
	t_pvtuner_scale *s = x->this_scale;
	double *pitchgrid = s->pitchgrid;
	pvtuner_copy_scale(x);
	int octsteps = 7;
	
	pitchgrid[0] = x->pbase;
	pitchgrid[1] = x->pbase * 1.059463;
	pitchgrid[2] = x->pbase * 1.259921;
	pitchgrid[3] = x->pbase * 1.334840;
	pitchgrid[4] = x->pbase * 1.498307;
	pitchgrid[5] = x->pbase * 1.587401;
	pitchgrid[6] = x->pbase * 1.887749;
	// scale_steps = 7 ;
    
	for( i = 1; i < 10; i++ ){
		for( j = 0; j < x->scale_steps; j++ ){
			pitchgrid[ i * octsteps + j] = pitchgrid[j] * pow(2.0,(float)i);
		}
	}
	s->current_scale = EASTERN ;
	s->scale_steps = 70;
	//   post("eastern scale");
}

void pvtuner_float(t_pvtuner *x, double f) 
{
	//	int inlet = ((t_pxobject*)x)->z_in;
	int inlet = x->x_obj.z_in;
	t_fftease *fft = x->fft;
	
	if (inlet == 1)
	{
		fft->P = f;
		fft->pitch_increment = fft->P*fft->L/fft->R;
		//		post("P set to %f",f);
	}
	else if (inlet == 2)
	{
		fft->synt = f;
		//		post("synt set to %f",f);
	}
	else if (inlet == 3)
	{
		x->ip = f;
		//		post("synt set to %f",f);
	}
}


double closestf(double test, double *arr) 
{
	int i;
	i = 0;
	if( test <= arr[0] ){
		return arr[0];
	}
	while( i < MAXTONES ){
		if( arr[i] > test ){
			break;
		}
		++i;
	}
	if( i >= MAXTONES - 1) {
		return arr[MAXTONES - 1];
	}
	if( (test - arr[i-1]) > ( arr[i] - test) ) {
		return arr[i];
	} else {
		return arr[i-1];
	}
}


void pvtuner_bypass(t_pvtuner *x, t_floatarg state)
{
	x->bypass = (short)state;	
}


void pvtuner_mute(t_pvtuner *x, t_floatarg state)
{
	x->mute = (short)state;	
}

void pvtuner_fftsize(t_pvtuner *x, t_floatarg f)
{
	x->fft->N = (int) f;
	pvtuner_init(x);
}

void pvtuner_overlap(t_pvtuner *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	pvtuner_init(x);
}

void pvtuner_winfac(t_pvtuner *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	pvtuner_init(x);
}

void pvtuner_fftinfo(t_pvtuner *x)
{
	t_fftease *fft = x->fft;
	fftease_fftinfo( fft, OBJECT_NAME );	
}

t_max_err get_fftsize(t_pvtuner *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_pvtuner *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		pvtuner_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_pvtuner *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_pvtuner *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		pvtuner_init(x);
	}
	return MAX_ERR_NONE;
}



t_max_err get_lowfreq(t_pvtuner *x, void *attr, long *ac, t_atom **av)
{
	if (ac && av) {
		char alloc;
		
		if (atom_alloc(ac, av, &alloc)) {
			return MAX_ERR_GENERIC;
		}
		atom_setlong(*av, x->lofreq);
	}
	return MAX_ERR_NONE;
}

t_max_err set_lowfreq(t_pvtuner *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		t_fftease *fft = x->fft;
		float f = atom_getfloat(av);
		if(x->fft->R <= 0){
			error("%s: zero sampling rate in set_lowfreq", OBJECT_NAME);
			return MAX_ERR_NONE;
		}
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

t_max_err get_highfreq(t_pvtuner *x, void *attr, long *ac, t_atom **av)
{
	if (ac && av) {
		char alloc;
		
		if (atom_alloc(ac, av, &alloc)) {
			return MAX_ERR_GENERIC;
		}
		atom_setlong(*av, x->hifreq);
	}
	return MAX_ERR_NONE;
}

t_max_err set_highfreq(t_pvtuner *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		t_fftease *fft = x->fft;
		int R = x->fft->R;
		float f = atom_getfloat(av);
		if(R <= 0){
			error("%s: zero sampling rate in set_hifreq", OBJECT_NAME);
			return MAX_ERR_NONE;
		}
		if(f < x->lofreq){
			error("%s: maximum cannot go below current minimum: %f",OBJECT_NAME,x->lofreq);
			return MAX_ERR_NONE;
		}
		if(f > R/2 ){
			f = R/2;
		}	
		x->hifreq = f;
		fftease_oscbank_setbins(fft,x->lofreq, x->hifreq);
	}
	return MAX_ERR_NONE;
}

void pvtuner_dsp64(t_pvtuner *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    int i;
    t_fftease *fft = x->fft;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
	for(i = 0; i < 4; i++){
		x->connections[i] = count[i];
	}
	
	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
	}
    if(reset_required){
        pvtuner_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,pvtuner_perform64,0,NULL);
}


