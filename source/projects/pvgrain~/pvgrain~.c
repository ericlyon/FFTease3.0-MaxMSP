#include "fftease.h"

static t_class *pvgrain_class;

#define OBJECT_NAME "pvgrain~"

typedef struct _pvgrain
{
	t_pxobject x_obj;
	t_fftease *fft;
	short *binsort;
	double topfreq;
	double bottomfreq;
	short bypass;
	int mute;
	double grain_probability;
	double sample_basefreq;
	int grains_per_frame ;
	void *list_outlet;
	double *listdata;
	short list_count;
	void *m_clock;
	t_atom myList[2];
	long fftsize_attr;
	long overlap_attr;
} t_pvgrain;

void *pvgrain_new(t_symbol *s, int argc, t_atom *argv);
void pvgrain_assist(t_pvgrain *x, void *b, long m, long a, char *s);
void pvgrain_mute(t_pvgrain *x, t_floatarg state);
void pvgrain_bypass(t_pvgrain *x, t_floatarg toggle);
void pvgrain_float(t_pvgrain *x, double f);
void pvgrain_tick(t_pvgrain *x);
void pvgrain_printchan(t_pvgrain *x);
void pvgrain_probability (t_pvgrain *x, t_floatarg prob);
void pvgrain_framegrains (t_pvgrain *x, t_floatarg grains);
void pvgrain_topfreq (t_pvgrain *x, t_floatarg top);
void pvgrain_bottomfreq (t_pvgrain *x, t_floatarg f);
void pvgrain_basefreq (t_pvgrain *x, t_floatarg base);
float pvgrain_randf(float min, float max);
void pvgrain_init(t_pvgrain *x);
void pvgrain_free(t_pvgrain *x);
void pvgrain_winfac(t_pvgrain *x, t_floatarg factor);
void pvgrain_fftinfo(t_pvgrain *x) ;
t_max_err set_fftsize(t_pvgrain *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_pvgrain *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_pvgrain *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_pvgrain *x, void *attr, long *ac, t_atom **av);
void pvgrain_perform64(t_pvgrain *x, t_object *dsp64, double **ins,
                       long numins, double **outs,long numouts, long vectorsize,
                       long flags, void *userparam);
void pvgrain_dsp64(t_pvgrain *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);

int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.pvgrain~", (method)pvgrain_new, (method)pvgrain_free, sizeof(t_pvgrain),0,A_GIMME,0);
	class_addmethod(c,(method)pvgrain_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)pvgrain_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)pvgrain_mute,"mute",A_DEFFLOAT,0);
	class_addmethod(c,(method)pvgrain_bypass,"bypass", A_FLOAT, 0);
	class_addmethod(c,(method)pvgrain_printchan,"printchan",A_DEFFLOAT,0);
	class_addmethod(c,(method)pvgrain_probability,"probability",A_DEFFLOAT,0);
	class_addmethod(c,(method)pvgrain_framegrains,"framegrains",A_DEFFLOAT,0);
	class_addmethod(c,(method)pvgrain_topfreq,"topfreq",A_FLOAT,0);
	class_addmethod(c,(method)pvgrain_basefreq,"basefreq",A_FLOAT,0);
//	class_addmethod(c,(method)pvgrain_winfac, "winfac",  A_FLOAT, 0);
	class_addmethod(c,(method)pvgrain_fftinfo, "fftinfo", 0);
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_pvgrain, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_pvgrain, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	pvgrain_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

void pvgrain_printchan(t_pvgrain *x)
{
	int i;
	double *channel = x->fft->channel;
	post("***");
	for( i = 0 ; i < 30; i+= 2 ){
		post("amp %f freq %f", channel[i*2], channel[i*2 + 1]);
	}
	post("***");
}

void pvgrain_basefreq (t_pvgrain *x, t_floatarg base)
{
	int R = x->fft->R;
	if( base < 0.0 )
		base = 0. ;
	if( base > R / 2 )
		base = R / 2 ;
	x->sample_basefreq = base;
}

void pvgrain_topfreq (t_pvgrain *x, t_floatarg top)
{
	int R = x->fft->R;
	t_fftease *fft = x->fft;

//	float curfreq ;
	if( top < 50. )
		top = 50.;
	if( top > R / 2 )
		top = R / 2;
	x->topfreq = top ;
	fftease_oscbank_setbins(fft,x->bottomfreq, x->topfreq);


}

void pvgrain_bottomfreq (t_pvgrain *x, t_floatarg f)
{
	int R = x->fft->R;
	t_fftease *fft = x->fft;
	if( f >= x->topfreq || f >= R/2){
		post("%f is too high a bottom freq",f);
		return;
	}
	
	x->bottomfreq = f;
	fftease_oscbank_setbins(fft,x->bottomfreq, x->topfreq);


}

void pvgrain_probability (t_pvgrain *x, t_floatarg prob)
{
	
	if( prob < 0. )
		prob = 0.;
	if( prob > 1. )
		prob = 1.;
	x->grain_probability = prob ;
}

void pvgrain_framegrains (t_pvgrain *x, t_floatarg grains)
{
	int N2 = x->fft->N2;
	if( grains < 1 )
		grains = 1;
	if( grains > N2 - 1 )
		grains = N2 - 1;
	x->grains_per_frame = grains ;
	
}

void pvgrain_tick(t_pvgrain *x)
{
	t_atom *myList = x->myList;
	double *listdata = x->listdata;
	int i;
	
	for (i=0; i < 2; i++) {
		atom_setfloat(myList+i,listdata[i]); /* macro for setting a t_atom */
	}
	outlet_list(x->list_outlet,0,2,myList);
}

void pvgrain_assist (t_pvgrain *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal) Input");
				break;
				
		}
	} else if (msg==2) {
		sprintf(dst,"(list) Amplitude/Frequency Pairs");
	}
}

void *pvgrain_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_pvgrain *x = (t_pvgrain *)object_alloc(pvgrain_class);

	dsp_setup((t_pxobject *)x,1);
	x->list_outlet = listout(x);

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();	
	fft->initialized = 0;
	
	x->grain_probability = 0.0001;
	x->topfreq = 1000.0;

	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;

	attr_args_process(x, argc, argv);
	pvgrain_init(x);
	return x;
}

void pvgrain_init(t_pvgrain *x)
{
	x->x_obj.z_disabled = 1;
	t_fftease  *fft = x->fft;
	short initialized = fft->initialized;
	fftease_init(fft);
	
	if(!initialized){
        x->m_clock = clock_new(x,(method)pvgrain_tick);
		x->sample_basefreq = 261.0;
		x->bottomfreq = 0.0;
		x->mute = 0;
		x->binsort = (short *) sysmem_newptrclear((fft->N2+1) * sizeof(short) );
		x->listdata = (double *) sysmem_newptrclear(40 * sizeof(double));
		
	} else if(initialized == 1) {
		x->binsort = (short *) sysmem_resizeptrclear(x->binsort,(fft->N2+1) * sizeof(short));
	}
	fftease_oscbank_setbins(fft,x->bottomfreq, x->topfreq);
	x->x_obj.z_disabled = 0;
}

void pvgrain_fftsize(t_pvgrain *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	pvgrain_init(x);
}

void pvgrain_overlap(t_pvgrain *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	pvgrain_init(x);
}

void pvgrain_winfac(t_pvgrain *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	pvgrain_init(x);
}

void pvgrain_fftinfo( t_pvgrain *x )
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}


void pvgrain_free(t_pvgrain *x)
{
	dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
	sysmem_freeptr(x->binsort);
	sysmem_freeptr(x->listdata);
}

void do_pvgrain(t_pvgrain *x)
{
	int 	i,j;
	double tmp, dice;
	short print_grain;
	t_fftease *fft = x->fft;
	double *channel = fft->channel;
	short *binsort = x->binsort;
	int grains_per_frame = x->grains_per_frame ;
	double selection_probability = x->grain_probability;
	int hi_bin = fft->hi_bin;
	int lo_bin = fft->lo_bin;
	double *listdata = x->listdata;
		
	x->list_count = 0;
	
	fold(fft);   
	rdft(fft,FFT_FORWARD);
	convert(fft);
	if( grains_per_frame > hi_bin - lo_bin )
		grains_per_frame = hi_bin - lo_bin;
	//  binsort[0] = 0;
	for( i = 0; i < hi_bin; i++ ){// could be hi_bin - lo_bin
		binsort[i] = i + lo_bin;
	}
	for( i = lo_bin; i < hi_bin - 1; i++ ){
		for( j = i+1; j < hi_bin; j++ ){
			if(channel[binsort[j] * 2] > channel[binsort[i] * 2]) {
				tmp = binsort[j];
				binsort[j] = binsort[i];
				binsort[i] = tmp;
			}
		}
	}
	for( i = 0; i < grains_per_frame; i++ ){
		print_grain = 1;
		dice = fftease_randf(0.,1.);
		if( dice < 0.0 || dice > 1.0 ){
			error("dice %f out of range", dice);
		}
		if( selection_probability < 1.0 ){
			if( dice > selection_probability) {
				print_grain = 0;
			} 
		}
		if( print_grain ){
			listdata[ x->list_count * 2 ] = channel[ binsort[i]*2 ];
			listdata[ (x->list_count * 2) + 1 ] = channel[(binsort[i]*2) + 1] ;
			++(x->list_count);
			clock_delay(x->m_clock,0); 
		}
	}
}

void pvgrain_perform64(t_pvgrain *x, t_object *dsp64, double **ins,
                       long numins, double **outs,long numouts, long vectorsize,
                       long flags, void *userparam)
{
	int 	i;
	double *MSPInputVector = ins[0];
	t_fftease *fft = x->fft;
	int D = fft->D;
	int Nw = fft->Nw;
	double *input = fft->input;
	int MSPVectorSize = fft->MSPVectorSize;
	double *internalInputVector = fft->internalInputVector;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;

	if (x->bypass || x->mute) {
		return;
	}
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
		do_pvgrain(x);
	}	
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(MSPInputVector + (D*i), input + (Nw-D), D * sizeof(t_double));
			do_pvgrain(x);
		}
	}
	else if( fft->bufferStatus == BIGGER_THAN_MSP_VECTOR ) {
        sysmem_copyptr(MSPInputVector,internalInputVector + (operationCount * MSPVectorSize),MSPVectorSize * sizeof(t_double));
		operationCount = (operationCount + 1) % operationRepeat;
		if( operationCount == 0 ) {
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(internalInputVector, input + (Nw - D), D * sizeof(t_double));
			do_pvgrain( x );
		}
        fft->operationCount = operationCount;
	}	
}	

void pvgrain_bypass(t_pvgrain *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void pvgrain_mute(t_pvgrain *x, t_floatarg state)
{
	x->mute = (short)state;	
}

#if MSP
void pvgrain_float(t_pvgrain *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	
	if (inlet == 1)
    {
		x->fft->P = f;
		//post("P set to %f",f);
    }
	else if (inlet == 2)
    {
		x->fft->synt = f;
		//post("synt set to %f",f);
    }
}
#endif
t_max_err get_fftsize(t_pvgrain *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_pvgrain *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		pvgrain_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_pvgrain *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_pvgrain *x, void *attr, long ac, t_atom *av)
{	
    int test_overlap;
    if (ac && av) {
        long val = atom_getlong(av);
        test_overlap = fftease_overlap(val);
        if(test_overlap > 0){
            x->fft->overlap = (int) val;
            pvgrain_init(x);
        }
    }
    return MAX_ERR_NONE;
}

void pvgrain_dsp64(t_pvgrain *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
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
        pvgrain_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,pvgrain_perform64,0,NULL);
}

