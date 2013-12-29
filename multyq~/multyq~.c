#include "fftease.h"

static t_class *multyq_class;

#define OBJECT_NAME "multyq~"

typedef struct _multyq
{
	t_pxobject x_obj;
	t_fftease *fft;
	double cf1;
	double gainfac1;
	double bw1;
	double cf2;
	double gainfac2;
	double bw2;
	double cf3;
	double gainfac3;
	double bw3;
	double cf4;
	double gainfac4;
	double bw4;
	double *rcos;
	double *filt;
	double *freqs;
	int rcoslen;
	short *connected;
	short please_update;
	short always_update;
	short mute;
	short bypass;
	long fftsize_attr;
	long overlap_attr;
} t_multyq;

void *multyq_new(t_symbol *s, int argc, t_atom *argv);
void multyq_assist(t_multyq *x, void *b, long m, long a, char *s);
void multyq_bypass(t_multyq *x, t_floatarg state);
void multyq_mute(t_multyq *x, t_floatarg state);
void update_filter_function(t_multyq *x);
void multyq_float(t_multyq *x, double f);
void filtyQ( float *S, float *C, float *filtfunc, int N2 );
void multyq_init(t_multyq *x);
void multyq_free(t_multyq *x);
void multyq_fftinfo(t_multyq *x);
//void multyq_overlap(t_multyq *x, t_floatarg f);
void multyq_winfac(t_multyq *x, t_floatarg f);
//void multyq_fftsize(t_multyq *x, t_floatarg f);
t_max_err set_fftsize(t_multyq *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_multyq *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_multyq *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_multyq *x, void *attr, long *ac, t_atom **av);
void multyq_perform64(t_multyq *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam);
void multyq_dsp64(t_multyq *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);

int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.multyq~", (method)multyq_new, (method)multyq_free, sizeof(t_multyq),0,A_GIMME,0);
	class_addmethod(c,(method)multyq_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)multyq_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)multyq_bypass,"bypass",A_DEFFLOAT,0);
	class_addmethod(c,(method)multyq_mute,"mute",A_DEFFLOAT,0);
//	class_addmethod(c,(method)multyq_overlap,"overlap",A_DEFFLOAT,0);
	class_addmethod(c,(method)multyq_winfac,"winfac",A_DEFFLOAT,0);
//	class_addmethod(c,(method)multyq_fftsize,"fftsize",A_FLOAT,0);
	class_addmethod(c,(method)multyq_fftinfo,"fftinfo",0);
	class_addmethod(c,(method)multyq_float,"float",A_FLOAT,0);
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_multyq, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_multyq, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	multyq_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

void multyq_free(t_multyq *x)
{
	dsp_free((t_pxobject *)x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
	sysmem_freeptr(x->rcos);
	sysmem_freeptr(x->freqs);
	sysmem_freeptr(x->filt);
}

void multyq_fftsize(t_multyq *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	multyq_init(x);
}

void multyq_overlap(t_multyq *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	multyq_init(x);
}

void multyq_winfac(t_multyq *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	multyq_init(x);
}

void multyq_fftinfo(t_multyq *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void *multyq_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_multyq *x = (t_multyq *)object_alloc(multyq_class);
	dsp_setup((t_pxobject *)x,13);
	outlet_new((t_pxobject *)x, "signal");
	x->x_obj.z_misc |= Z_NO_INPLACE;
	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();	
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;		
	fft->initialized = 0;
    attr_args_process(x, argc, argv);
	multyq_init(x);
	return x;
}

void multyq_init(t_multyq *x)
{
	int i;
	double funda, base;
	
	x->x_obj.z_disabled = 1;
	t_fftease  *fft = x->fft;
	short initialized = fft->initialized;
	
	fftease_init(fft);

	if(!initialized){
		x->please_update = 0;
		x->always_update = 0;
		x->rcoslen = 8192 ;	
		x->connected = (short *) malloc(16 * sizeof(short));	
		x->rcos = (double *) sysmem_newptrclear(x->rcoslen * sizeof( double ));
		x->freqs = (double *) sysmem_newptrclear(fft->N2 * sizeof( double ));
		x->filt = (double *) sysmem_newptrclear((fft->N2 + 1) * sizeof( double ));
		
		x->cf1  = 200.;
		x->gainfac1  = 0.0;
		x->bw1 = .15;
		x->cf2  = 700.;
		x->gainfac2  = 0.0;
		x->bw2  = .1; 
		x->cf3  = 3000.;
		x->gainfac3  = 0.0;
		x->bw3  = .15;
		x->cf4  = 12000.;
		x->gainfac4 = 0.0;
		x->bw4 = .15;
		x->mute = 0;
		x->bypass = 0;
		for (i = 0; i < x->rcoslen; i++){
			x->rcos[i] =  .5 - .5 * cos(((float)i/(float)x->rcoslen) * TWOPI);
		}	
	} else {
		x->freqs = (double *) sysmem_resizeptrclear(x->freqs, fft->N2 * sizeof( double ));
		x->filt = (double *) sysmem_resizeptrclear(x->filt, (fft->N2 + 1) * sizeof( double ));
	}
	x->fft->input = (double *)sysmem_resizeptrclear(fft->input, fft->Nw * sizeof(double));
    x->fft->output = (double *)sysmem_resizeptrclear(fft->output, fft->Nw * sizeof(double));
	
	funda = base = (double)fft->R /(double)fft->N ;
	for(i = 0; i < fft->N2; i++){
		x->freqs[i] = base;
		base += funda;
	}
	update_filter_function(x);
	x->x_obj.z_disabled = 0;	
}

// filtyQ( float *S, float *C, float *filtfunc, int N2 )
void do_multyq(t_multyq *x)
{
	int real, imag, amp, phase;
	double a, b;
	int i;
	t_fftease *fft = x->fft;
	double *S = fft->buffer;
	double *C = fft->channel;
	double *filtfunc = x->filt;
	int N2 = fft->N2;
	fold(fft);   
	rdft(fft,1);
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		a = ( i == N2 ? S[1] : S[real] );
		b = ( i == 0 || i == N2 ? 0. : S[imag] );
		C[amp] = hypot( a, b );
		C[amp] *= filtfunc[ i ];
		C[phase] = -atan2( b, a );
	}
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		S[real] = *(C+amp) * cos( *(C+phase) );
		if ( i != N2 )
			S[imag] = -*(C+amp) * sin( *(C+phase) );
	}
	rdft(fft,-1);
	overlapadd(fft);
}

void multyq_perform64(t_multyq *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam)
{
	int i, j;
	double	*MSPInputVector = ins[0];
	double	*in2 = ins[1];
	double	*in3 = ins[2];
	double	*in4 = ins[3];
	double	*in5 = ins[4];
	double	*in6 = ins[5];
	double	*in7 = ins[6];
	double	*in8 = ins[7];
	double	*in9 = ins[8];
	double	*in10 = ins[9];
	double	*in11 = ins[10];
	double	*in12 = ins[11];
	double	*in13 = ins[12];
	double	*MSPOutputVector = outs[0];
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
	short *connected = x->connected;

	if(connected[1]){
		x->cf1 = *in2;
	}
	if(connected[2]){
		x->bw1 = *in3;
	}
	if(connected[3]){
		x->gainfac1 = *in4;
	}
	if(connected[4]){
		x->cf2 = *in5;
	}
	if(connected[5]){
		x->bw2 = *in6;
	}
	if(connected[6]){
		x->gainfac2 = *in7;
	}  
	if(connected[7]){
		x->cf3 = *in8;
	}
	if(connected[8]){
		x->bw3 = *in9;
	}
	if(connected[9]){
		x->gainfac3 = *in10;
	}  
	if(connected[10]){
		x->cf4 = *in11;
	}
	if(connected[11]){
		x->bw4 = *in12;
	}
	if(connected[12]){
		x->gainfac4 = *in13;
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
	if(x->always_update) {
		update_filter_function(x);
	}
	else if(x->please_update) {
		update_filter_function(x);	
		x->please_update = 0;
	}
	
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_multyq(x);
        
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
            
			do_multyq(x);
			
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
            
			do_multyq(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}		

void multyq_bypass(t_multyq *x, t_floatarg state)
{
	x->bypass = (short)state;	
}

void multyq_mute(t_multyq *x, t_floatarg state)
{
	x->mute = (short)state;	
}

void multyq_dsp64(t_multyq *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    int i;
    t_fftease *fft = x->fft;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
	for(i = 0; i < 13; i++){
		x->connected[i] = count[i];
	}
	x->always_update = 0;
	for(i = 1; i < 13; i++) {
		x->always_update += count[i];
	}
	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
	}
    if(reset_required){
        multyq_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,multyq_perform64,0,NULL);
}

void update_filter_function(t_multyq *x) 
{
//	float funda; //, curfreq, m1, m2;
	double lo, hi ;
	double ploc, gainer;
	int i;
	double nyquist = (float)x->fft->R / 2.0;
	double *filt = x->filt;
	double *rcos = x->rcos;
	double *freqs = x->freqs;
	int rcoslen = x->rcoslen;
	int N2 = x->fft->N2;
	
	// sanity
	if( x->cf1 < 0 ){
		x->cf1 = 0;
	} 
	else if( x->cf1 > nyquist){
		x->cf1 = nyquist ;
	} 
	if( x->bw1 <= .05 ){
		x->bw1 = .05;
	}
	else if( x->bw1 > 1. ){
		x->bw1 = 1.;
	}
	if( x->gainfac1 < -1. ){
		x->gainfac1 = -1;
	}
	if( x->cf2 < 0 ){
		x->cf2 = 0;
	} 
	else if( x->cf2> nyquist){
		x->cf2 = nyquist ;
	} 
	if( x->bw2 <= .05 ){
		x->bw2 = .05;
	}
	else if( x->bw2 > 1. ){
		x->bw2 = 1.;
	}
	if( x->gainfac2 < -1. ){
		x->gainfac2 = -1;
	}
	if( x->cf3 < 0 ){
		x->cf3 = 0;
	} 
	else if( x->cf3 > nyquist){
		x->cf3 = nyquist ;
	} 
	if( x->bw3 <= .05 ){
		x->bw3 = .05;
	}
	else if( x->bw3 > 1. ){
		x->bw3 = 1.;
	}
	if( x->gainfac3 < -1. ){
		x->gainfac3 = -1;
	}
	if( x->cf4 < 0 ){
		x->cf4 = 0;
	} 
	else if( x->cf4 > nyquist){
		x->cf4 = nyquist ;
	} 
	if( x->bw4 <= .05 ){
		x->bw4 = .05;
	}
	else if( x->bw4 > 1. ){
		x->bw4 = 1.;
	}
	if( x->gainfac4 < -1. ){
		x->gainfac4 = -1;
	}
	for( i = 0; i < N2; i++ ) {
		x->filt[i] = 1.0 ;
	}
	// filt 1
	lo = x->cf1 * (1.0 - x->bw1 );
	hi = x->cf1 * (1.0 + x->bw1 );
	for( i = 0; i < N2; i++ ) {
		if(freqs[i] >= lo && freqs[i] <= hi){
			ploc = (freqs[i] - lo) / (hi - lo);
			gainer = 1 + x->gainfac1 * rcos[ (int) (ploc * rcoslen) ] ;
			if( gainer < 0 ){
				gainer = 0;
			}
			filt[i] *= gainer ;
			
		}
	}
	// filt 2
	lo = x->cf2 * (1.0 - x->bw2 );
	hi = x->cf2 * (1.0 + x->bw2 );
	for( i = 0; i < N2; i++ ) {
		if( freqs[i] >= lo && freqs[i] <= hi){
			ploc = (freqs[i] - lo) / (hi - lo);
			gainer = 1 + x->gainfac2 * rcos[ (int) (ploc * rcoslen) ] ;
			if( gainer < 0 ){
				gainer = 0;
			}
			filt[i] *= gainer ;
			
		}
	}
	// filt 3
	lo = x->cf3 * (1.0 - x->bw3 );
	hi = x->cf3 * (1.0 + x->bw3 );
	for( i = 0; i < N2; i++ ) {
		if(freqs[i] >= lo && freqs[i] <= hi){
			ploc = (freqs[i] - lo) / (hi - lo);
			gainer = 1 + x->gainfac3 * rcos[ (int) (ploc * rcoslen) ] ;
			if( gainer < 0 ){
				gainer = 0;
			}
			filt[i] *= gainer ;
			
		}
	}
	// filt 4
	lo = x->cf4 * (1.0 - x->bw4 );
	hi = x->cf4 * (1.0 + x->bw4 );
	for( i = 0; i < N2; i++ ) {
		if(freqs[i] >= lo && freqs[i] <= hi){
			ploc = (freqs[i] - lo) / (hi - lo);
			gainer = 1 + x->gainfac4 * rcos[ (int) (ploc * rcoslen) ] ;
			if( gainer < 0 ){
				gainer = 0;
			}
			filt[i] *= gainer ;
		}
	}
}

void multyq_float(t_multyq *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	
	if (inlet == 1)
    {
		x->cf1 = f;
    }
	else if (inlet == 2)
    {
		x->bw1 = f;
    }
	else if (inlet == 3)
    {
		x->gainfac1 = f;
    }
	else if (inlet == 4)
    {
		x->cf2 = f;
    }
	else if (inlet == 5)
    {
		x->bw2 = f;
    }
	else if (inlet == 6)
    {
		x->gainfac2 = f;
    }
	else if (inlet == 7)
    {
		x->cf3 = f;
    }
	else if (inlet == 8)
    {
		x->bw3 = f;
    }
	else if (inlet == 9)
    {
		x->gainfac3 = f;
    }	
	else if (inlet == 10)
    {
		x->cf4 = f;
    }
	else if (inlet == 11)
    {
		x->bw4 = f;
    }
	else if (inlet == 12)
    {
		x->gainfac4 = f;
    }		
	x->please_update = 1;
}

void multyq_assist (t_multyq *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input"); break;
			case 1: sprintf(dst,"(signal/float) Cf1");break;
			case 2: sprintf(dst,"(signal/float) Bw1"); break;
			case 3: sprintf(dst,"(signal/float) Gain1"); break;
			case 4: sprintf(dst,"(signal/float) Cf2"); break;
			case 5: sprintf(dst,"(signal/float) Bw2"); break;
			case 6: sprintf(dst,"(signal/float) Gain2"); break;
			case 7: sprintf(dst,"(signal/float) Cf3"); break;
			case 8: sprintf(dst,"(signal/float) Bw3"); break;
			case 9: sprintf(dst,"(signal/float) Gain3"); break;
			case 10: sprintf(dst,"(signal/float) Cf4"); break;
			case 11: sprintf(dst,"(signal/float) Bw4"); break;
			case 12: sprintf(dst,"(signal/float) Gain4"); break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}


void filtyQ( float *S, float *C, float *filtfunc, int N2 )
{
	int real, imag, amp, phase;
	double a, b;
	int i;
//	float maxamp = 1.;
	
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		a = ( i == N2 ? S[1] : S[real] );
		b = ( i == 0 || i == N2 ? 0. : S[imag] );
		C[amp] = hypot( a, b );
		C[amp] *= filtfunc[ i ];
		C[phase] = -atan2( b, a );
	}
	
	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		S[real] = *(C+amp) * cos( *(C+phase) );
		if ( i != N2 )
			S[imag] = -*(C+amp) * sin( *(C+phase) );
	}
}

t_max_err get_fftsize(t_multyq *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_multyq *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		multyq_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_multyq *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_multyq *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		multyq_init(x);
	}
	return MAX_ERR_NONE;
}
