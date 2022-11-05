#include "fftease.h"

static t_class *pvcompand_class;

#define OBJECT_NAME "pvcompand~"

typedef struct _pvcompand
{
	t_pxobject x_obj;
	t_fftease *fft;
	double rescale;
	double *curthresh;
	double *atten;
	double *thresh;
	int count;
	double thresh_interval;
	double max_atten; 
	double atten_interval ; 
	double tstep;
	double gstep;
	double last_max_atten;
	short norml;
	short mute;
	short bypass;
	short connected[2];
	long fftsize_attr;
	long overlap_attr;
} t_pvcompand;

void *pvcompand_new(t_symbol *s, int argc, t_atom *argv);
//void pvcompand_dsp(t_pvcompand *x, t_signal **sp, short *count);
void pvcompand_assist(t_pvcompand *x, void *b, long m, long a, char *s);
void update_thresholds(t_pvcompand *x);
void pvcompand_normalize(t_pvcompand *x, t_floatarg val);
void pvcompand_float(t_pvcompand *x, double f);
void pvcompand_free(t_pvcompand *x);
float pvcompand_ampdb(float db);
void pvcompand_init(t_pvcompand *x);
void pvcompand_fftinfo(t_pvcompand *x);
//void pvcompand_overlap(t_pvcompand *x, t_floatarg f);
void pvcompand_winfac(t_pvcompand *x, t_floatarg f);
void pvcompand_bypass(t_pvcompand *x, t_floatarg f);
void pvcompand_mute(t_pvcompand *x, t_floatarg f);
//void pvcompand_fftsize(t_pvcompand *x, t_floatarg f);
t_max_err set_fftsize(t_pvcompand *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_pvcompand *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_pvcompand *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_pvcompand *x, void *attr, long *ac, t_atom **av);
void pvcompand_dsp64(t_pvcompand *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void pvcompand_perform64(t_pvcompand *x, t_object *dsp64, double **ins,
                         long numins, double **outs,long numouts, long vectorsize,
                         long flags, void *userparam);

int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.pvcompand~", (method)pvcompand_new, (method)pvcompand_free, sizeof(t_pvcompand),0,A_GIMME,0);
	class_addmethod(c,(method)pvcompand_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)pvcompand_normalize, "normalize", A_FLOAT, 0);
	class_addmethod(c,(method)pvcompand_winfac,"winfac", A_FLOAT, 0);
	class_addmethod(c,(method)pvcompand_fftinfo,"fftinfo", 0);
	class_addmethod(c,(method)pvcompand_bypass,"bypass", A_FLOAT, 0);
	class_addmethod(c,(method)pvcompand_mute,"mute", A_FLOAT, 0);
	class_addmethod(c,(method)pvcompand_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)pvcompand_float,"float",A_FLOAT,0);
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_pvcompand, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_pvcompand, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	pvcompand_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

void pvcompand_bypass(t_pvcompand *x, t_floatarg f)
{
	x->bypass = (short)f;
}

void pvcompand_mute(t_pvcompand *x, t_floatarg f)
{
	x->mute = (short)f;
}


void pvcompand_free( t_pvcompand *x ){
	dsp_free( (t_pxobject *) x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
	sysmem_freeptr(x->curthresh);
	sysmem_freeptr(x->atten);
	sysmem_freeptr(x->thresh);
}

void pvcompand_assist (t_pvcompand *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input"); break;
			case 1: sprintf(dst,"(float/signal) Threshold"); break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}


void pvcompand_float(t_pvcompand *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	
    if (inlet == 1)
	{
		x->last_max_atten = x->max_atten = f;
		update_thresholds(x);	
		
	}
}

void *pvcompand_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_pvcompand *x = (t_pvcompand *)object_alloc(pvcompand_class);
	dsp_setup((t_pxobject *)x,2);
	outlet_new((t_pxobject *)x, "signal");

	x->fft = (t_fftease *) malloc(sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();	
	
	// INITIALIZATIONS
	x->max_atten = -6.0;
	atom_arg_getdouble(&x->max_atten , 0, argc, argv);
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	attr_args_process(x, argc, argv);
	pvcompand_init(x);
	return x;
}

void pvcompand_init(t_pvcompand *x)
{
	t_fftease  *fft = x->fft;
	short initialized = x->fft->initialized;
	x->x_obj.z_disabled = 1;
	fftease_init(fft);
	
	if(!initialized){
		x->norml = 0;
		x->mute = 0;
		x->bypass = 0;
		x->thresh_interval = 1.0;
		x->last_max_atten =  x->max_atten; 
		x->atten_interval = 2.0 ; 
		x->tstep = 1.0 ;
		x->gstep = 2.0 ;
		//		x->rescale = 1.0;	
		x->thresh = (double *) sysmem_newptrclear((fft->N) * sizeof(double));
		x->atten = (double *) sysmem_newptrclear(fft->N * sizeof(double) );
		x->curthresh = (double *) sysmem_newptrclear(fft->N * sizeof(double) );
	} else if(initialized == 1) {
		x->thresh = (double *) sysmem_resizeptrclear(x->thresh, fft->N * sizeof(double));
		x->atten = (double *) sysmem_resizeptrclear(x->atten, fft->N * sizeof(double));
		x->curthresh = (double *) sysmem_resizeptrclear(x->curthresh, fft->N * sizeof(double));
	}
	update_thresholds(x); 
	x->x_obj.z_disabled = 0;
	
}

void update_thresholds( t_pvcompand *x ) {
	int i;
	double nowamp = x->max_atten ;
	double nowthresh = 0.0 ;
	int N = x->fft->N;
	x->count = 0;
	if( nowamp < 0.0 ){
		while( nowamp < 0.0 ){
			x->atten[x->count] = pvcompand_ampdb( nowamp );
			nowamp += x->gstep ;
			++(x->count);
			if(x->count >= N){
				error("count exceeds %d",N);
				x->count = N - 1;
				break;
			}
		}
    }
	else if( nowamp > 0.0 ){
		while( nowamp > 0.0 ){
			x->atten[x->count] = pvcompand_ampdb( nowamp );
			nowamp -= x->gstep ;
			++(x->count);
			if(x->count >= N){
				error("count exceeds %d",N);
				x->count = N - 1;
				break;
			}
		}
	}
	for( i = 0; i < x->count; i++){
		x->thresh[i] = pvcompand_ampdb( nowthresh );
		nowthresh -= x->tstep ;
	}
	/*
	 for( i = 0; i < x->count; i++)
	 post("thresh %f gain %f\n",x->thresh[i], x->atten[i]);
	 */
}

void pvcompand_normalize(t_pvcompand *x, t_floatarg val) 
{
	x->norml = (short)val;
	//  post("normalize is %d",x->norml);
}

void do_pvcompand(t_pvcompand *x)
{
	t_fftease *fft = x->fft;
	double *channel = fft->channel;
	int N = fft->N;
	double *curthresh = x->curthresh;
	double *thresh = x->thresh;
	double *atten = x->atten;
	int count = x->count;
	double max_atten = x->max_atten;
	int i,j;	
	double maxamp ;	
	double cutoff;
	double avr, new_avr, rescale;
	
	fold(fft);	
	rdft(fft,FFT_FORWARD);
	leanconvert(fft);

	maxamp = 0.;
	avr = 0;
	for( i = 0; i < N; i+= 2 ){
		avr += channel[i];
		if( maxamp < channel[i] ){
			maxamp = channel[i] ;
		}
	}
	if(count <= 1){
		//	post("count too low!"); 
		count = 1;
	}
	for( i = 0; i < count; i++ ){
		curthresh[i] = thresh[i]*maxamp ;
	}
	cutoff = curthresh[count-1];
	new_avr = 0;
	for( i = 0; i < N; i += 2){
		if( channel[i] > cutoff ){
			j = count-1;
			while( channel[i] > curthresh[j] ){
				j--;
				if( j < 0 ){
					j = 0;
					break;
				}
			}
			channel[i] *= atten[j];
		}
		new_avr += channel[i] ;
	}
	
	if( x->norml ) {
		if( new_avr <= 0 ){
			new_avr = .0001;
		}
		rescale =  avr / new_avr ;
		
	} else {
		rescale = pvcompand_ampdb( max_atten * -.5); 
	}
	for( i = 0; i < N; i += 2){
		channel[i] *= rescale;
	} 
	
	leanunconvert(fft);
	rdft(fft, FFT_INVERSE);
	overlapadd(fft);
}

void pvcompand_perform64(t_pvcompand *x, t_object *dsp64, double **ins,
                         long numins, double **outs,long numouts, long vectorsize,
                         long flags, void *userparam)
{
	int i,j;	
	double *MSPInputVector = ins[0];
	double *in2 = ins[1];
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
	//	short *connected = x->connected;	
	
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

	if( x->connected[1] ){
		x->max_atten = *in2;
		if(x->max_atten != x->last_max_atten) {
			x->last_max_atten = x->max_atten;
			update_thresholds(x);
		}
	} 
	
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_pvcompand(x);
        
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
            
			do_pvcompand(x);
			
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
            
			do_pvcompand(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}	

float pvcompand_ampdb(float db) 
{
	float amp;
	amp = pow((double)10.0, (double)(db/20.0)) ;
	return(amp);
}

void pvcompand_overlap(t_pvcompand *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	pvcompand_init(x);
}

void pvcompand_winfac(t_pvcompand *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	pvcompand_init(x);
}

void pvcompand_fftinfo(t_pvcompand *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void pvcompand_fftsize(t_pvcompand *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	pvcompand_init(x);
}

t_max_err get_fftsize(t_pvcompand *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_pvcompand *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		pvcompand_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_pvcompand *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_pvcompand *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		pvcompand_init(x);
	}
	return MAX_ERR_NONE;
}


void pvcompand_dsp64(t_pvcompand *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    t_fftease *fft = x->fft;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
    x->connected[1] = count[1];
	
	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
	}
    if(reset_required){
        pvcompand_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,pvcompand_perform64,0,NULL);
}


