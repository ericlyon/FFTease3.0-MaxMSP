#include "fftease.h"

static t_class *disarray_class;

#define OBJECT_NAME "disarray~"


typedef struct _disarray
{
	t_pxobject x_obj;
	t_fftease *fft;
    double top_frequency;
	int *shuffle_in;
    int *shuffle_out;
    int shuffle_count;
    int max_bin;
	void *list_outlet;
	t_atom *list_data;
	short mute;
	short bypass;
	long fftsize_attr;
	long overlap_attr;
} t_disarray;

void *disarray_new(t_symbol *msg, short argc, t_atom *argv);
void disarray_assist(t_disarray *x, void *b, long m, long a, char *s);
void disarray_switch_count (t_disarray *x, t_floatarg i);
void disarray_topfreq (t_disarray *x, t_floatarg freq);
void disarray_fadetime (t_disarray *x, t_floatarg f);
void reset_shuffle( t_disarray *x );
void disarray_showstate( t_disarray *x );
void disarray_list (t_disarray *x, t_symbol *msg, short argc, t_atom *argv);
void disarray_setstate (t_disarray *x, t_symbol *msg, short argc, t_atom *argv);
void disarray_isetstate (t_disarray *x, t_symbol *msg, short argc, t_atom *argv);
int rand_index(int max);
void disarray_mute(t_disarray *x, t_floatarg toggle);
void disarray_bypass(t_disarray *x, t_floatarg toggle);
void copy_shuffle_array(t_disarray *x);
void interpolate_frames_to_channel(t_disarray *x);
void disarray_killfade(t_disarray *x);
void disarray_forcefade(t_disarray *x, t_floatarg toggle);
void disarray_init(t_disarray *x);
void disarray_free(t_disarray *x);
//void disarray_overlap(t_disarray *x, t_floatarg o);
void disarray_winfac(t_disarray *x, t_floatarg o);
//void disarray_fftsize(t_disarray *x, t_floatarg o);
void disarray_fftinfo(t_disarray *x);
void disarray_force_switch(t_disarray *x, t_floatarg toggle);
void iswitch_count(t_disarray *x, t_int i);
void switch_count (t_disarray *x, t_floatarg i);
t_max_err set_fftsize(t_disarray *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_disarray *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_disarray *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_disarray *x, void *attr, long *ac, t_atom **av);
void disarray_dsp64(t_disarray *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void disarray_perform64(t_disarray *x, t_object *dsp64, double **ins,
                        long numins, double **outs,long numouts, long vectorsize,
                        long flags, void *userparam);

int C74_EXPORT main(void)
{
    
	t_class *c;
	c = class_new("fftz.disarray~", (method)disarray_new, (method)disarray_free, sizeof(t_disarray),0,A_GIMME,0);
	
	class_addmethod(c,(method)disarray_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)iswitch_count, "int", A_FLOAT, 0);
	class_addmethod(c,(method)reset_shuffle, "bang", 0);
	
    class_addmethod(c,(method)disarray_showstate,"showstate",0);
    class_addmethod(c,(method)disarray_list, "list", A_GIMME, 0);
    class_addmethod(c,(method)disarray_setstate, "setstate", A_GIMME, 0);
    class_addmethod(c,(method)disarray_assist,"assist",A_CANT,0);
    class_addmethod(c,(method)disarray_mute, "mute", A_FLOAT, 0);
    class_addmethod(c,(method)disarray_topfreq, "topfreq", A_FLOAT, 0);
    class_addmethod(c,(method)disarray_bypass, "bypass", A_LONG, 0);
    class_addmethod(c,(method)switch_count, "switch_count",  A_DEFFLOAT, 0);
	class_addmethod(c,(method)disarray_winfac, "winfac",  A_DEFFLOAT, 0);
	class_addmethod(c,(method)disarray_fftinfo, "fftinfo", 0);
	class_addmethod(c,(method)reset_shuffle, "reset_shuffle", 0);
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_disarray, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_disarray, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	disarray_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

void iswitch_count(t_disarray *x, t_int i)
{
	switch_count(x,(t_floatarg)i);
}

void switch_count (t_disarray *x, t_floatarg i)
{
	if( i < 0 ){
        i = 0;
	}
	if( i > x->fft->N2 ) {
		i = x->fft->N2;
	}
	x->shuffle_count = i;
}

void disarray_free(t_disarray *x)
{
    dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
	sysmem_freeptr(x->list_data);
	sysmem_freeptr(x->shuffle_in);
	sysmem_freeptr(x->shuffle_out);
}

void disarray_init(t_disarray *x )
{
    
	double curfreq;
	
	t_fftease *fft = x->fft;
	
	double c_fundamental;
	x->x_obj.z_disabled = 1;
 	fftease_init(fft);
	
	int N2 = fft->N2;
	int N = fft->N;
	short initialized = fft->initialized;
	
	c_fundamental = fft->c_fundamental;
	if(initialized == 0){
		x->mute = 0;
		x->bypass = 0;
		x->list_data = (t_atom *) sysmem_newptrclear((N+2) * sizeof(t_atom)) ;
		x->shuffle_in = (int *) sysmem_newptrclear(N2 * sizeof(int));
		x->shuffle_out = (int *) sysmem_newptrclear(N2 * sizeof(int));
	} else if (initialized == 1) {
		x->list_data = (t_atom *)sysmem_resizeptrclear(x->list_data, (N+2) * sizeof(t_atom));
		x->shuffle_in = (int *) sysmem_resizeptrclear(x->shuffle_in, N2 * sizeof(int));
		x->shuffle_out = (int *) sysmem_resizeptrclear(x->shuffle_out, N2 * sizeof(int));
	}
	
	if(initialized != 2){
		if( x->top_frequency < c_fundamental || x->top_frequency > 20000) {
			x->top_frequency = 20000.0 ;
		}
		x->max_bin = 1;
		curfreq = 0;
		while( curfreq < x->top_frequency ) {
			++(x->max_bin);
			curfreq += c_fundamental ;
		}
		reset_shuffle(x); // set shuffle lookup
		x->shuffle_count = 0;
	}
	x->x_obj.z_disabled = 0;
}

void disarray_topfreq (t_disarray *x, t_floatarg freq)
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

void disarray_assist (t_disarray *x, void *b, long msg, long arg, char *dst)
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

void *disarray_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_fftease *fft;
    
	t_disarray *x = (t_disarray *)object_alloc(disarray_class);
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
	disarray_init(x);
	return x;
}


void disarray_mute(t_disarray *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void disarray_bypass(t_disarray *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void disarray_fftsize(t_disarray *x, t_floatarg f)
{
	t_fftease *fft = x->fft;
	fft->N = (int) f;
	disarray_init(x);
}

void disarray_overlap(t_disarray *x, t_floatarg f)
{
	t_fftease *fft = x->fft;
	fft->overlap = (int) f;
	disarray_init(x);
}

void disarray_winfac(t_disarray *x, t_floatarg f)
{
	t_fftease *fft = x->fft;
	fft->winfac = (int) f;
	disarray_init(x); /* calling lighter reinit routine */
}

void disarray_fftinfo( t_disarray *x )
{
	fftease_fftinfo( x->fft, OBJECT_NAME );
}

void do_disarray(t_disarray *x)
{
	t_fftease *fft = x->fft;
	double *channel = fft->channel;
	int		i;
	double tmp;
	int shuffle_count = x->shuffle_count;
    int *shuffle_in = x->shuffle_in;
    int *shuffle_out = x->shuffle_out;
  	
	fold(fft);
	rdft(fft,1);
	leanconvert(fft);
	for( i = 0; i < shuffle_count ; i++){
		tmp = channel[ shuffle_in[ i ] * 2 ];
		channel[ shuffle_in[ i ] * 2]  = channel[ shuffle_out[ i ] * 2];
		channel[ shuffle_out[ i ] * 2]  = tmp;
	}
	leanunconvert(fft);
	rdft(fft,-1);
	overlapadd(fft);
}

// lean convert perform method
void disarray_perform64(t_disarray *x, t_object *dsp64, double **ins,
                        long numins, double **outs,long numouts, long vectorsize,
                        long flags, void *userparam)
{
	int i,j;
	t_fftease *fft = x->fft;
	double *MSPInputVector = ins[0];
	double *MSPOutputVector = outs[0];
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
        
		do_disarray(x);
        
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
            
			do_disarray(x);
			
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
            
			do_disarray(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}


void reset_shuffle (t_disarray *x)
{
    int i;
    int temp, p1, p2;
    int max;
    
    //post("max bin %d",x->max_bin);
    max = x->max_bin;
	for( i = 0; i < x->fft->N2; i++ ) {
		x->shuffle_out[i] = x->shuffle_in[i] = i ;
	}
	
	for( i = 0; i < 10000; i++ ) {
		p1 = x->shuffle_out[ rand_index( max ) ];
		p2 = x->shuffle_out[ rand_index( max ) ];
		temp = x->shuffle_out[ p1 ];
		x->shuffle_out[ p1 ] = x->shuffle_out[ p2 ];
		x->shuffle_out[ p2 ] = temp;
	}
	
}

int rand_index(int max) {
	
	return (rand() % max);
}



void disarray_list (t_disarray *x, t_symbol *msg, short argc, t_atom *argv) {
	short i;
	int ival;
	x->shuffle_count = argc;
	for (i=0; i < argc; i++) {
        
        ival = (int)atom_getfloatarg(i,argc,argv);
        
        
		if ( ival < x->fft->N2 ) {
			x->shuffle_out[ i ] = ival;
		} else {
			post ("%d out of range",ival);
		}
	}
}

void disarray_setstate (t_disarray *x, t_symbol *msg, short argc, t_atom *argv) {
    short i;
    int ival;
    
    x->shuffle_count = argc;
    for (i=0; i < argc; i++) {
        ival = atom_getfloatarg(i,argc,argv);
        
        if ( ival < x->fft->N2 && ival >= 0) {
            x->shuffle_out[ i ] = ival;
        } else {
            error("%s: %d is out of range",OBJECT_NAME, ival);
        }
    }
}

void disarray_showstate (t_disarray *x ) {
    
    t_atom *list_data = x->list_data;
    
    short i;

    for( i = 0; i < x->shuffle_count; i++ ) {
        atom_setlong(list_data+i,x->shuffle_out[i]);
    }
    outlet_list(x->list_outlet,0,x->shuffle_count,list_data);
}


t_max_err get_fftsize(t_disarray *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_disarray *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		disarray_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_disarray *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_disarray *x, void *attr, long ac, t_atom *av)
{
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		disarray_init(x);
	}
	return MAX_ERR_NONE;
}

/*
void disarray_dsp(t_disarray *x, t_signal **sp, short *count)
{
	t_fftease *fft = x->fft;
	if(fft->MSPVectorSize != sp[0]->s_n){
		fft->MSPVectorSize = sp[0]->s_n;
		fftease_set_fft_buffers(fft);
	}
	
	if(fft->R != sp[0]->s_sr){
		fft->R = sp[0]->s_sr;
		disarray_init(x);
	}
	if(fftease_msp_sanity_check(fft,OBJECT_NAME))	
		dsp_add(disarray_perform, 3, x, sp[0]->s_vec, sp[1]->s_vec);
	
}
*/

void disarray_dsp64(t_disarray *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
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
        disarray_init(x);
    }
    object_method(dsp64, gensym("dsp_add64"),x,disarray_perform64,0,NULL);
}

