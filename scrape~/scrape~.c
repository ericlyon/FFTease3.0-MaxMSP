#include "fftease.h"

static t_class *scrape_class;


#define OBJECT_NAME "scrape~"

typedef struct _scrape
{
	t_pxobject x_obj;
	t_fftease *fft;
	double knee;
	double cutoff;
	double scrape_mult;
	double thresh1;
	double thresh2;
	double *threshfunc;
	short connected[8];
	short mute;
	short bypass;
	long fftsize_attr;
	long overlap_attr;
} t_scrape;

void *scrape_new(t_symbol *msg, short argc, t_atom *argv);
void scrape_assist(t_scrape *x, void *b, long m, long a, char *s);
void scrape_float(t_scrape *x, double f) ;
void update_thresh_function( t_scrape *x );
void scrape_frowned( float *S, float *C, float *threshfunc, float fmult, int N2 );
void scrape_mute(t_scrape *x, t_floatarg toggle);
void scrape_bypass(t_scrape *x, t_floatarg toggle);
void scrape_free( t_scrape *x );
void update_thresh_function( t_scrape *x );
void scrape_init(t_scrape *x);
void scrape_fftinfo(t_scrape *x);
// void scrape_overlap(t_scrape *x, t_floatarg f);
void scrape_winfac(t_scrape *x, t_floatarg f);
// void scrape_fftsize(t_scrape *x, t_floatarg f);
//void scrape_version(void);
t_max_err set_fftsize(t_scrape *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_scrape *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_scrape *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_scrape *x, void *attr, long *ac, t_atom **av);
void scrape_dsp64(t_scrape *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void scrape_perform64(t_scrape *x, t_object *dsp64, double **ins,
                    long numins, double **outs,long numouts, long vectorsize,
                    long flags, void *userparam);


int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.scrape~", (method)scrape_new, (method)scrape_free, sizeof(t_scrape),0,A_GIMME,0);
	class_addmethod(c,(method)scrape_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)scrape_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)scrape_mute, "mute", A_FLOAT, 0);
	class_addmethod(c,(method)scrape_bypass, "bypass", A_FLOAT, 0);
//	class_addmethod(c,(method)scrape_overlap,"overlap",A_DEFFLOAT,0);
	class_addmethod(c,(method)scrape_winfac,"winfac",A_DEFFLOAT,0);
//	class_addmethod(c,(method)scrape_fftsize,"fftsize",A_FLOAT,0);
	class_addmethod(c,(method)scrape_fftinfo,"fftinfo",0);  
	class_addmethod(c,(method)scrape_float,"float",A_FLOAT,0);
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_scrape, fftsize_attr);
	CLASS_ATTR_DEFAULT_SAVE(c, "fftsize", 0, "1024");
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_scrape, overlap_attr);
	CLASS_ATTR_DEFAULT_SAVE(c, "overlap", 0, "8");
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	scrape_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;

}

void scrape_free( t_scrape *x )
{
	dsp_free( (t_pxobject *) x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
	sysmem_freeptr(x->threshfunc);
}

void scrape_assist (t_scrape *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input ");break;
			case 1: sprintf(dst,"(float/signal) Knee Frequency"); break;
			case 2: sprintf(dst,"(float/signal) Cutoff Frequency"); break;
			case 3: sprintf(dst,"(float/signal) Knee Threshold"); break;
			case 4: sprintf(dst,"(float/signal) Cutoff Threshold"); break;
			case 5: sprintf(dst,"(float/signal) Multiplier For Weak Bins"); break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}

void *scrape_new(t_symbol *msg, short argc, t_atom *argv)
{
t_fftease *fft;

	t_scrape *x = (t_scrape *)object_alloc(scrape_class);
	dsp_setup((t_pxobject *)x,6);
	outlet_new((t_pxobject *)x, "signal");

	x->fft = (t_fftease *) sysmem_newptrclear( sizeof(t_fftease) );
	fft = x->fft;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();
	fft->initialized = 0;
	x->knee = 1000.0;
	x->cutoff = 4000.0;
	x->thresh1 = 0.001;
	x->thresh2 = 0.09;
	x->scrape_mult = 0.1;

	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;

	attr_args_process(x, argc, argv);
	scrape_init(x);
	return x;
}

void scrape_init(t_scrape *x)
{
	t_fftease  *fft = x->fft;
	x->x_obj.z_disabled = 1;
	short initialized = fft->initialized;
	
	fftease_init(fft);
		
	if(!initialized){
		x->mute = 0;
		x->bypass = 0;
		x->threshfunc = (double *) sysmem_newptrclear(fft->N2 * sizeof(double));
		update_thresh_function(x);
	} else if(initialized == 1){
		x->threshfunc = (double *) sysmem_resizeptrclear(x->threshfunc, fft->N2 * sizeof(double));
		update_thresh_function(x);
	}
	x->x_obj.z_disabled = 0;
}

void scrape_fftsize(t_scrape *x, t_floatarg f)
{
	x->fft->N = (int) f;
	scrape_init(x);
}

void scrape_overlap(t_scrape *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	scrape_init(x);
}

void scrape_winfac(t_scrape *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	scrape_init(x);
}

void scrape_fftinfo(t_scrape *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void scrape_float(t_scrape *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	
	if (inlet == 1)
    {
		if( f> 50  && f < 18000) {
			x->knee = f ;
			update_thresh_function( x );	
		}	
		
    }
	else if (inlet == 2)
    {
		if( (f > x->knee)  && (f < 20000) ) {
			x->cutoff = f ;
			update_thresh_function( x );	
		}
    }
	else if (inlet == 3)
    {
		x->thresh1 = f;
		update_thresh_function( x );
    }
	else if (inlet == 4)
    {
		x->thresh2 = f;
		update_thresh_function( x );
    }
	else if (inlet == 5)
    {
		if( f > 0 ) {
			x->scrape_mult = f;
		}
    }
} 

void update_thresh_function( t_scrape *x ) 
{
	double funda, curfreq, m1, m2;
	int i;
	int R = x->fft->R;
	int N = x->fft->N;
	int N2 = x->fft->N2;
	
	funda = (double)  R / ((double)N);
	curfreq = funda ;
	for( i = 0; i < N2; i++ ) {
		if( curfreq  < x->knee ){
			x->threshfunc[i] = 0.0 ;
		} else if( curfreq >= x->knee && curfreq < x->cutoff ) {
			m2 = (x->knee - curfreq) / (x->cutoff - x->knee) ;
			m1 = 1.0 - m2 ;
			x->threshfunc[i] = m1 * x->thresh1 + m2 * x->thresh2 ;
		} else {
			x->threshfunc[i] = x->thresh2;
		}
		curfreq += funda ;
	}
}

void scrape_mute(t_scrape *x, t_floatarg toggle)
{
	x->mute = (short)toggle;	
}

void scrape_bypass(t_scrape *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;	
}

void do_scrape(t_scrape *x)
{
	int real, imag, amp, phase;
	double a, b;
	int i;
	double maxamp = 0.0;
	t_fftease *fft = x->fft;
	int N2 = fft->N2;
	double scrape_mult = x->scrape_mult;
	double *channel = fft->channel;
	double *buffer = fft->buffer;
	double *threshfunc = x->threshfunc;
	
	fold(fft);	
	rdft(fft,FFT_FORWARD);
	
	for( i = 0; i <= N2; i++ ){
		amp = i<<1;
		if( maxamp < channel[amp] ){
			maxamp = channel[amp];
		}
	}
	
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		a = ( i == N2 ? buffer[1] : buffer[real] );
		b = ( i == 0 || i == N2 ? 0. : buffer[imag] );
		channel[amp] = hypot( a, b );
		
		if ( (channel[amp]) < threshfunc[i] * maxamp ){
			channel[amp] *= scrape_mult;
		}
		channel[phase] = -atan2( b, a );
	}
	
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		buffer[real] = *(channel+amp) * cos( *(channel+phase) );
		if ( i != N2 )
			buffer[imag] = -*(channel+amp) * sin( *(channel+phase) );
	}

	rdft(fft, FFT_INVERSE);
	overlapadd(fft);
}

void scrape_perform64(t_scrape *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam)
{
	int	i,j;
	double tmp ;
	short update = 0;	
	double *MSPInputVector = ins[0];
	double *knee_freq = ins[1];
	double *cut_freq = ins[2];
	double *thresh1 = ins[3];
	double *thresh2 = ins[4];
	double *scrape_mult = ins[5];
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
	// float *threshfunc = x->threshfunc ;
	short *connected = x->connected;
	
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
	if( connected[1] ){
		tmp = *knee_freq++;
		if( tmp > 50 && tmp < 20000 ){
			x->knee = tmp;
			update = 1;
		}
	}
	if( connected[2] ){
		tmp = *cut_freq++;
		if( tmp > x->knee && tmp < 20000 ){
			x->cutoff = *cut_freq++;
			update = 1;
		}
	}
	if( connected[3] ){
		x->thresh1 = *thresh1;
		update = 1;
	}
	if( connected[4] ){
		x->thresh2 = *thresh2;
		update = 1;
	}
	if( connected[5] ){
		x->scrape_mult = *scrape_mult;
	}
	
	if( update ){
		update_thresh_function( x );	
	}
    // 
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_scrape(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(MSPInputVector + (D*i), input + (Nw-D), D * sizeof(t_double));
            
			do_scrape(x);
			
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
            
			do_scrape(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}		

t_max_err get_fftsize(t_scrape *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_scrape *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		scrape_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_scrape *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_scrape *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		scrape_init(x);
	}
	return MAX_ERR_NONE;
}


void scrape_dsp64(t_scrape *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    int i;
    t_fftease *fft = x->fft;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
	for( i = 0; i < 6; i++ ){
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
        scrape_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,scrape_perform64,0,NULL);
}

