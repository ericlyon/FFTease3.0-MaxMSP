#include "fftease.h"

static t_class *pvwarpb_class;

#define OBJECT_NAME "pvwarpb~"

typedef struct _pvwarpb
{

	t_pxobject x_obj;
	t_symbol *buffername;
	t_buffer_ref *tablebuf_ref;
	double lofreq;/* user speficied lowest synthfreq */
	double hifreq;/* user specified highest synthfreq */
	double topfreq;
	t_fftease *fft;
	short *connections;
	short mute;
	short bypass;
	int pitch_connected;
	int synt_connected;
	short please_update;
	short always_update;
	double cf1;
	double bw1;
	double warpfac1;
	double cf2;
	double bw2;
	double warpfac2;
	int funcoff;
	short verbose;
	short automate;
	long fftsize_attr;
	long overlap_attr;
    t_symbol *buffername_attr;
    t_double *warpfunc; // workspace to create a new function
	short initialized; // state for object
} t_pvwarpb;

void *pvwarpb_new(t_symbol *s, int argc, t_atom *argv);
void pvwarpb_assist(t_pvwarpb *x, void *b, long m, long a, char *s);
void pvwarpb_bypass(t_pvwarpb *x, t_floatarg state);
void pvwarpb_mute(t_pvwarpb *x, t_floatarg state);
void pvwarpb_verbose(t_pvwarpb *x, t_floatarg state);
void pvwarpb_automate(t_pvwarpb *x, t_floatarg state);
void pvwarpb_autofunc(t_pvwarpb *x, t_floatarg minval, t_floatarg maxval);
void pvwarpb_float(t_pvwarpb *x, double f); // Look at floats at inlets
void pvwarpb_free( t_pvwarpb *x );
void pvwarpb_fftinfo( t_pvwarpb *x );
double fftease_randf( double min, double max );
double closestf(double test, double *arr) ;
int freq_to_bin(double target, double fundamental);
void update_warp_function( t_pvwarpb *x ) ;
void pvwarpb_init(t_pvwarpb *x);
void pvwarpb_bottomfreq(t_pvwarpb *x, t_floatarg f);
void pvwarpb_topfreq(t_pvwarpb *x, t_floatarg f);
void pvwarpb_fftinfo(t_pvwarpb *x);
void pvwarpb_winfac(t_pvwarpb *x, t_floatarg f);
void pvwarpb_dblclick(t_pvwarpb *x);
void pvwarpb_setbuf(t_pvwarpb *x, t_symbol *wavename);
t_max_err set_fftsize(t_pvwarpb *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_pvwarpb *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_pvwarpb *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_pvwarpb *x, void *attr, long *ac, t_atom **av);
t_max_err set_buffername(t_pvwarpb *x, void *attr, long ac, t_atom *av);
t_max_err get_buffername(t_pvwarpb *x, void *attr, long *ac, t_atom **av);

void pvwarpb_dsp64(t_pvwarpb *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void pvwarpb_perform64(t_pvwarpb *x, t_object *dsp64, double **ins,
                    long numins, double **outs,long numouts, long vectorsize,
                    long flags, void *userparam);


int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.pvwarpb~", (method)pvwarpb_new, (method)pvwarpb_free, sizeof(t_pvwarpb),0,A_GIMME,0);	
	class_addmethod(c,(method)pvwarpb_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)pvwarpb_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)pvwarpb_bypass,"bypass",A_DEFFLOAT,0);
	class_addmethod(c,(method)pvwarpb_mute,"mute",A_DEFFLOAT,0);
	class_addmethod(c,(method)pvwarpb_verbose,"verbose",A_DEFFLOAT,0);
	class_addmethod(c,(method)pvwarpb_bottomfreq,"bottomfreq",A_FLOAT,0);
	class_addmethod(c,(method)pvwarpb_topfreq,"topfreq",A_FLOAT,0);
	class_addmethod(c,(method)pvwarpb_fftinfo,"fftinfo",0);
	class_addmethod(c,(method)pvwarpb_autofunc,"autofunc",A_DEFFLOAT, A_DEFFLOAT,0);
//	class_addmethod(c,(method)pvwarpb_winfac,"winfac",A_DEFFLOAT,0);
	class_addmethod(c,(method)pvwarpb_fftinfo,"fftinfo",0);
    class_addmethod(c,(method)pvwarpb_dblclick,"dblclick", A_CANT, 0);
	class_addmethod(c,(method)pvwarpb_float,"float",A_FLOAT,0);
    
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_pvwarpb, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_pvwarpb, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");
    
    CLASS_ATTR_SYM(c, "buffername", 0, t_pvwarpb, buffername_attr);
    CLASS_ATTR_ACCESSORS(c, "buffername", (method)get_buffername, (method)set_buffername);
    CLASS_ATTR_LABEL(c, "buffername", 0, "Buffer Name");
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	pvwarpb_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

void pvwarpb_fftsize(t_pvwarpb *x, t_floatarg f)
{
	x->fft->N = (int) f;
	pvwarpb_init(x);
}

void pvwarpb_automate(t_pvwarpb *x, t_floatarg state)
{
	x->automate = state;
}
void pvwarpb_overlap(t_pvwarpb *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	pvwarpb_init(x);
}

void pvwarpb_winfac(t_pvwarpb *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	pvwarpb_init(x);
}

void pvwarpb_fftinfo(t_pvwarpb *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME );	
}

void update_warp_function( t_pvwarpb *x ) 
{
	int i,j;
	int N2 = x->fft->N2;
	double warpfac1 = x->warpfac1;
	double warpfac2 = x->warpfac2;
	t_buffer_obj *dbuf;
    long b_nchans;
    long b_frames;
    //double *warpfunc = (double*) sysmem_newptrclear(N2);
    t_double *warpfunc = x->warpfunc;
    float *b_samples;
	double cf1 = x->cf1;
	double cf2 = x->cf2;
	double bw1 = x->bw1;
	double bw2 = x->bw2;
	double c_fundamental = x->fft->c_fundamental;
	double deviation;
	double diff;
	int midbin, lobin, hibin ;
	double hif, lof;
	int bin_extent;
	short verbose = x->verbose;
    
    if(! x->tablebuf_ref){
        x->tablebuf_ref = buffer_ref_new((t_object*)x, x->buffername);
    } else {
        buffer_ref_set(x->tablebuf_ref, x->buffername);
    }
    dbuf = buffer_ref_getobject(x->tablebuf_ref);
    if(dbuf == NULL){
        object_post((t_object*)x,"%s: nonexistent buffer 2 ( %s )",OBJECT_NAME, x->buffername->s_name);
        return;
    }
    b_nchans = buffer_getchannelcount(dbuf);
    b_frames = buffer_getframecount(dbuf);
    if(b_frames < N2 || b_nchans != 1){
        post("%s: table too small or not mono",OBJECT_NAME);
        return;
    }
    
	for( i = 0; i < N2; i++ ){
		warpfunc[i] = 1.0;
	}
	hif = cf1 * (1. + bw1);
	lof = cf1 * (1. - bw1);
	midbin = freq_to_bin( cf1, c_fundamental );
	hibin = freq_to_bin( hif, c_fundamental );
	lobin = freq_to_bin( lof, c_fundamental );
	if( hibin >= N2 - 1 ){
		hibin = N2 - 1;
	}
	if( lobin < 0 ){
		lobin = 0;
	} 
    if( verbose ){
        post("bump1: hi %d mid %d lo %d",hibin,midbin,lobin);
    }
	warpfunc[midbin] = warpfac1;
	diff = warpfac1 - 1.0 ;
	bin_extent = hibin - midbin ;
	for( i = midbin, j = 0; i < hibin; i++, j++ ){
		deviation = diff * ((float)(bin_extent - j) / (float) bin_extent );
		warpfunc[ i ] += deviation ;
	}
	bin_extent = midbin - lobin ;
	for( i = midbin, j = 0; i > lobin; i--, j++ ){
		deviation = diff * ((float)(bin_extent - j) / (float) bin_extent );
		warpfunc[ i ] += deviation ;
	}
	
	// NOW DO SECOND BUMP
	hif = cf2 * (1. + bw2);
	lof = cf2 * (1. - bw2);
	midbin = freq_to_bin( cf2, c_fundamental );
	hibin = freq_to_bin( hif, c_fundamental );
	lobin = freq_to_bin( lof, c_fundamental );
	if( hibin >= N2 - 1 ){
		hibin = N2 - 1;
	}
	if( lobin < 0 ){
		lobin = 0;
	} 
    if( verbose ){
        post("bump2: hi %d mid %d lo %d",hibin,midbin,lobin);
    }
	warpfunc[midbin] = warpfac2;
	diff = warpfac2 - 1.0 ;
	bin_extent = hibin - midbin ;
	for( i = midbin, j = 0; i < hibin; i++, j++ ){
		deviation = diff * ((float)(bin_extent - j) / (float) bin_extent );
		warpfunc[ i ] += deviation ;
	}
	bin_extent = midbin - lobin ;
	for( i = midbin, j = 0; i > lobin; i--, j++ ){
		deviation = diff * ((float)(bin_extent - j) / (float) bin_extent );
		warpfunc[ i ] += deviation ;
	}
    // buffer stuffer
	b_samples = buffer_locksamples(dbuf);
    for(i = 0; i < N2; i++){
        b_samples[i] = warpfunc[i];
    }
    buffer_unlock(dbuf);
    object_method(dbuf, gensym("dirty"));
	x->please_update = 0;
}
void pvwarpb_verbose(t_pvwarpb *x, t_floatarg state)
{
	x->verbose = state;	
}

void pvwarpb_autofunc(t_pvwarpb *x, t_floatarg minval, t_floatarg maxval)
{
	int minpoints, maxpoints, segpoints, i;
	int pointcount = 0;
	double target, lastval;
	double m1, m2;
	int N2 = x->fft->N2;
	t_buffer_obj *dbuf;
    long b_nchans;
    long b_frames;
    t_double *warpfunc = x->warpfunc;
    float *b_samples;
    
    if(x->buffername == NULL){
        object_error((t_object*)x,"pvwarpb: no warp function defined");
        return;
    }
    if(! x->tablebuf_ref){
        x->tablebuf_ref = buffer_ref_new((t_object*)x, x->buffername);
    } else {
        buffer_ref_set(x->tablebuf_ref, x->buffername);
    }
    dbuf = buffer_ref_getobject(x->tablebuf_ref);
    if(dbuf == NULL){
        object_error((t_object*)x,"%s: nonexistent buffer 3 ( %s )",
            OBJECT_NAME, x->buffername->s_name);
        return;
    }
    b_nchans = buffer_getchannelcount(dbuf);
    b_frames = buffer_getframecount(dbuf);
    if(b_frames < N2 || b_nchans != 1){
        post("%s: table too small or not mono",OBJECT_NAME);
        return;
    }

	minpoints = 0.05 * (float) N2;
	maxpoints = 0.25 * (float) N2;
	if( minval > 1000.0 || minval < .001 ){
		minval = 0.5;
	}
	if( maxval < 0.01 || maxval > 1000.0 ){
		minval = 2.0;
	}
	
	lastval = fftease_randf(minval, maxval);
	// post("automate: min %d max %d",minpoints, maxpoints);
	while( pointcount < N2 ){
		target = fftease_randf(minval, maxval);
		segpoints = minpoints + (rand() % (maxpoints-minpoints));
		if( pointcount + segpoints > N2 ){
			segpoints = N2 - pointcount;
		}
		for( i = 0; i < segpoints; i++ ){
			m2 = (float)i / (float) segpoints ;
			m1 = 1.0 - m2;
			warpfunc[ pointcount + i ] = m1 * lastval + m2 * target;
		}
		lastval = target;
		pointcount += segpoints;
	}
    // buffer stuffer
	b_samples = buffer_locksamples(dbuf);
    for(i = 0; i < N2; i++){
        b_samples[i] = warpfunc[i];
    }
    buffer_unlocksamples(dbuf);
    object_method(dbuf, gensym("dirty"));
}

void pvwarpb_bypass(t_pvwarpb *x, t_floatarg state)
{
	x->bypass = state;	
}
void pvwarpb_mute(t_pvwarpb *x, t_floatarg state)
{
	x->mute = state;	
}

void pvwarpb_free( t_pvwarpb *x ){
	dsp_free( (t_pxobject *) x);
	fftease_free(x->fft);
    sysmem_freeptr(x->warpfunc);
	sysmem_freeptr(x->connections);
}

void pvwarpb_assist (t_pvwarpb *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal) Input ");
				break;
			case 1:
				sprintf(dst,"(signal/float) Center Frequency 1");
				break;
			case 2:
				sprintf(dst,"(signal/float) Bandwidth Factor 1");
				break;
			case 3:
				sprintf(dst,"(signal/float) Warp Factor 1");
				break;
			case 4:
				sprintf(dst,"(signal/float) Center Frequency 2");
				break;
			case 5:
				sprintf(dst,"(signal/float) Bandwidth Factor 2");
				break;
			case 6:
				sprintf(dst,"(signal/float) Warp Factor 2");
				break;
			case 7:
				sprintf(dst,"(signal/float) Function Offset (0.0-1.0) ");
				break;
			case 8:
				sprintf(dst,"(signal/float) Pitch Factor");
				break;
			case 9:
				sprintf(dst,"(signal/float) Synthesis Gate Value");
				break;
				
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}




void *pvwarpb_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_pvwarpb *x = (t_pvwarpb *)object_alloc(pvwarpb_class);
	
	dsp_setup((t_pxobject *)x,10);
	outlet_new((t_pxobject *)x, "signal");
	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();
	fft->initialized = 0; // for FFTease package
	x->initialized = 0; // for object
	x->lofreq = 0.0;
	x->hifreq = 10000.0;
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
    x->tablebuf_ref = NULL;
    x->buffername = NULL;
    // atom_arg_getsym(&x->buffername,0,argc,argv);
    x->warpfunc = (t_double *) sysmem_newptrclear(8192);
	attr_args_process(x, argc, argv);
	pvwarpb_init(x);
	return x;
}


void pvwarpb_dblclick(t_pvwarpb *x)
{
    t_buffer_obj *dbuf;
    if(! x->tablebuf_ref){
        x->tablebuf_ref = buffer_ref_new((t_object*)x, x->buffername);
    } else {
        buffer_ref_set(x->tablebuf_ref, x->buffername);
    }
    dbuf = buffer_ref_getobject(x->tablebuf_ref);
    if(dbuf){
        buffer_view(dbuf);
    }
}


void pvwarpb_init(t_pvwarpb *x)
{
	t_fftease  *fft = x->fft;
	x->x_obj.z_disabled = 1;

	fftease_init(fft);
	
	if(!x->initialized){
		srand(clock());
		x->please_update = 0;
		x->verbose = 0;
		x->bypass = 0;
		x->mute = 0;
		x->topfreq = 3000. ;
		x->always_update = 0;
		x->automate = 0;
		x->warpfac1 = 1.0;
		x->warpfac2 = 1.0;
		x->funcoff = 0;
		x->cf1 = 500.;
		x->cf2 = 3000.;
		x->bw1 = 0.2;
		x->bw2 = 0.2;

		x->connections = (short *) sysmem_newptr(16 * sizeof(short));
		x->initialized = 1;
	}	
	fftease_oscbank_setbins(fft,x->lofreq, x->hifreq);
	x->x_obj.z_disabled = 0;
}



void pvwarpb_bottomfreq(t_pvwarpb *x, t_floatarg f)
{
	if( f < 0 || f > x->fft->R / 2.0 ){
		error("%s: frequency %f out of range", OBJECT_NAME, f);
		return;
	}
	x->lofreq = f;
	fftease_oscbank_setbins(x->fft, x->lofreq, x->hifreq);	
}

void pvwarpb_topfreq(t_pvwarpb *x, t_floatarg f)
{
	if( f < x->lofreq || f > x->fft->R / 2.0 ){
		error("%s: frequency %f out of range", OBJECT_NAME, f);
		return;
	}
	x->hifreq = f;
	fftease_oscbank_setbins(x->fft, x->lofreq, x->hifreq);	
}

void do_pvwarpb(t_pvwarpb *x)
{
	t_fftease *fft = x->fft;
	int lo_bin = fft->lo_bin;
	int hi_bin = fft->hi_bin;
	int chan, freq;
	int funcoff = x->funcoff;
	int N2 = fft->N2;
	double *channel = fft->channel;
	t_buffer_obj *dbuf;
    long b_nchans;
    long b_frames;
    float *b_samples;

	fold(fft);   
	rdft(fft,FFT_FORWARD);
	convert(fft);
    if(! x->tablebuf_ref){
        x->tablebuf_ref = buffer_ref_new((t_object*)x, x->buffername);
    } else {
        buffer_ref_set(x->tablebuf_ref, x->buffername);
    }
    dbuf = buffer_ref_getobject(x->tablebuf_ref);
    if(dbuf == NULL){
        post("%s: nonexistent buffer 1 ( %s )",OBJECT_NAME, x->buffername->s_name);
        return;
    }
    b_nchans = buffer_getchannelcount(dbuf);
    b_frames = buffer_getframecount(dbuf);
    if(b_frames < N2 || b_nchans != 1){
        post("%s: table too small or not mono",OBJECT_NAME);
        return;
    }
	b_samples = buffer_locksamples(dbuf);
    
    
	for ( chan = lo_bin; chan < hi_bin; chan++ ) {
		freq = (chan << 1) + 1;
		channel[freq] *= b_samples[(chan + funcoff) % N2];
	}
    buffer_unlock(dbuf);
	oscbank(fft);
}

void pvwarpb_perform64(t_pvwarpb *x, t_object *dsp64, double **ins,
                       long numins, double **outs,long numouts, long vectorsize,
                       long flags, void *userparam)
{
	int 	i,j;
	
	double f;
	
	double *MSPInputVector = ins[0];
	double *in1 = ins[1];
	double *in2 = ins[2];
	double *in3 = ins[3];
	double *in4 = ins[4];
	double *in5 = ins[5];
	double *in6 = ins[6];
	double *in7 = ins[7];
	double *in8 = ins[8];
	double *in9 = ins[9];
	double *MSPOutputVector = outs[0];
	t_fftease *fft = x->fft;
	int D = fft->D;
	int Nw = fft->Nw;
	int N2 = fft->N2;
	double *input = fft->input;
	double *output = fft->output;
	double mult = fft->mult;
	int MSPVectorSize = fft->MSPVectorSize;
	double *internalInputVector = fft->internalInputVector;
	double *internalOutputVector = fft->internalOutputVector;		
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	short *connections = x->connections;	
    t_buffer_obj *dbuf;
    buffer_ref_set(x->tablebuf_ref, x->buffername);
    dbuf = buffer_ref_getobject(x->tablebuf_ref);

	if(! dbuf ){
        for(i=0; i < vectorsize; i++){ MSPOutputVector[i] = 0.0; }
		return;
	}
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

	if(!x->automate) {
		if( connections[0] ) {
			x->cf1  = *in1 ; 
		}
		if ( connections[1] ) {
			x->bw1 = *in2 ;
		}
		if ( connections[2] ) {
			x->warpfac1 = *in3 ;
		}
		if( connections[3] ) {
			x->cf2  = *in4 ; 
		}
		if ( connections[4] ) {
			x->bw2 = *in5 ;
		}
		if ( connections[5] ) {
			x->warpfac2 = *in6 ;
		}
	}  
	
	if( connections[6] ) {
		f  = *in7 ; 
		if( f < 0 ) {
			f = 0.0;
		} else if (f > 1.0 ){
			f = 1.0;
		}
		x->funcoff = (int) (f * (double) (N2 - 1));
	}
	if ( connections[7] ) {
		fft->P = *in8 ;
	}
	if ( connections[8] ) {
		fft->synt = *in9 ;
	}
	
	if( (x->please_update || x->always_update)  && ! x->automate){
		update_warp_function(x);
	}
    //do_pvwarpb(x);
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_pvwarpb(x);
        
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
            
			do_pvwarpb(x);			
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
            
			do_pvwarpb(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}		


int freq_to_bin( double target, double fundamental ){
	float lastf = 0.0;
	float testf = 0.0;
	int thebin = 0;
	while( testf < target ){
		++thebin;
		lastf = testf;
		testf += fundamental;
	}
	
	if(fabs(target - testf) < fabs(target - lastf) ){
		return thebin;
	} else {
		return (thebin - 1);
	}
}

void pvwarpb_float(t_pvwarpb *x, double f) // Look at floats at inlets
{
	int inlet = ((t_pxobject*)x)->z_in;
	t_fftease *fft = x->fft;
	if (inlet == 1){
		x->cf1 = f;
		x->please_update = 1;
	} else if (inlet == 2) {
		x->bw1 = f;
		x->please_update = 1;
	} else if (inlet == 3) {
		x->warpfac1 = f;
		x->please_update = 1;
	} else if (inlet == 4) {
		x->cf2 = f;
		x->please_update = 1;
		
	} else if (inlet == 5) {
		x->bw2 = f;
		x->please_update = 1;
		
	} else if (inlet == 6) {
		x->warpfac2 = f;
		x->please_update = 1;
	} else if (inlet == 7) {
		if( f < 0 ) {
			f = 0.0;
		} else if (f > 1.0 ){
			f = 1.0;
		}
		x->funcoff = f * (float) (fft->N2 - 1);
	} else if (inlet == 8) {
		
		if(f < -500 || f > 500){
			post("%s: uncouth pitch factor rejected: %f",OBJECT_NAME, f );
			return;
		}
		fft->P = f;
	} else if (inlet == 9)
    {
		fft->synt = f;
    }
}


void pvwarpb_setbuf(t_pvwarpb *x, t_symbol *wavename)
{
	t_buffer_obj *b;
    t_buffer_ref *bref = buffer_ref_new((t_object*)x, wavename);
    b = buffer_ref_getobject(bref);
    
	if (!b) {
        error("%s: bad buffer",OBJECT_NAME);
        return;
    }
    // could do more error checking here
    object_free(bref);
    x->buffername = wavename;
}
t_max_err get_fftsize(t_pvwarpb *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_pvwarpb *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		pvwarpb_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_pvwarpb *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_pvwarpb *x, void *attr, long ac, t_atom *av)
{	
    int test_overlap;
    if (ac && av) {
        long val = atom_getlong(av);
        test_overlap = fftease_overlap(val);
        if(test_overlap > 0){
            x->fft->overlap = (int) val;
            pvwarpb_init(x);
        }
    }
    return MAX_ERR_NONE;
}

t_max_err get_buffername(t_pvwarpb *x, void *attr, long *ac, t_atom **av)
{
    if (ac && av) {
        char alloc;
        
        if (atom_alloc(ac, av, &alloc)) {
            return MAX_ERR_GENERIC;
        }
        x->buffername_attr = x->buffername;
        atom_setsym(*av, x->buffername_attr);
    }
    return MAX_ERR_NONE;
}


t_max_err set_buffername(t_pvwarpb *x, void *attr, long ac, t_atom *av)
{
    if (ac && av) {
        t_symbol *val = atom_getsym(av);
        x->buffername = val;
        // pvwarpb_init(x);
    }
    return MAX_ERR_NONE;
}


void pvwarpb_dsp64(t_pvwarpb *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    t_fftease *fft = x->fft;
	long i;

    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
	x->always_update = 0;
	// update_warp_function(x);
	for( i = 1; i < 10; i++ ){
		// only the first 6 inlets alter warp function
		if( i < 6 ){
			x->always_update += count[i];
		}
		x->connections[i-1] = count[i];
	}
	
	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
	}
    if(reset_required){
        pvwarpb_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,pvwarpb_perform64,0,NULL);
}


