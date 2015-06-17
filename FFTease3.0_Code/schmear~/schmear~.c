#include "fftease.h"

static t_class *schmear_class;

#define OBJECT_NAME "schmear~"
#define MAXSCHMEAR (129)

typedef struct _schmear
{
	t_pxobject x_obj;
	t_fftease *fft;
	double schmearmult;
	short mute;
	short bypass;
	double *spreader;
	double *newamps;
	int spreadlen;
	double threshold;
	short freakmode;
	int shift;
	long fftsize_attr;
	long overlap_attr;
} t_schmear;

void *schmear_new(t_symbol *s, int argc, t_atom *argv);
void schmear_mute(t_schmear *x, t_floatarg toggle);
void schmear_rel2peak(t_schmear *x, t_floatarg toggle);
void schmear_assist(t_schmear *x, void *b, long m, long a, char *s);
void schmear_float(t_schmear *x, double f);
// void schmear_overlap(t_schmear *x, t_floatarg o);
void schmear_free(t_schmear *x);
void schmear_init(t_schmear *x);
void schmear_overlap(t_schmear *x, t_floatarg f);
void schmear_winfac(t_schmear *x, t_floatarg f);
void schmear_threshold(t_schmear *x, t_floatarg f);
void schmear_fftinfo(t_schmear *x);
void schmear_bypass(t_schmear *x, t_floatarg toggle);
void schmear_schmimp(t_schmear *x, t_symbol *msg, short argc, t_atom *argv);
void schmear_shift(t_schmear *x, t_floatarg f);
void schmear_oscbank(t_schmear *x, t_floatarg flag);
t_max_err set_fftsize(t_schmear *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_schmear *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_schmear *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_schmear *x, void *attr, long *ac, t_atom **av);
void schmear_dsp64(t_schmear *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void schmear_perform64(t_schmear *x, t_object *dsp64, double **ins,
                    long numins, double **outs,long numouts, long vectorsize,
                    long flags, void *userparam);
int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.schmear~", (method)schmear_new, (method)schmear_free, sizeof(t_schmear),0,A_GIMME,0);
	
	class_addmethod(c,(method)schmear_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)schmear_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)schmear_mute,"mute",A_FLOAT,0);
	class_addmethod(c,(method)schmear_bypass,"bypass", A_FLOAT, 0);
	class_addmethod(c,(method)schmear_threshold,"threshold",A_FLOAT,0);
	class_addmethod(c,(method)schmear_shift,"shift",A_FLOAT,0);
	class_addmethod(c,(method)schmear_schmimp,"schmimp",A_GIMME,0);
	class_addmethod(c,(method)schmear_oscbank,"oscbank",A_FLOAT,0);
	class_addmethod(c,(method)schmear_winfac,"winfac",A_FLOAT,0);
	class_addmethod(c,(method)schmear_fftinfo,"fftinfo",0);
	class_addmethod(c,(method)schmear_float,"float",A_FLOAT,0);

	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_schmear, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_schmear, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");

	class_dspinit(c);
	class_register(CLASS_BOX, c);
	schmear_class = c;
	
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}



void schmear_float(t_schmear *x, double f) // Look at floats at inlets
{
	
	int inlet = x->x_obj.z_in;

	if (inlet == 1)
    {
		x->threshold = f;
    }
	if (inlet == 2)
    {
		x->schmearmult = f;
    }
}


void schmear_fftsize(t_schmear *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	schmear_init(x);
}

void schmear_oscbank(t_schmear *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}

void schmear_overlap(t_schmear *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	schmear_init(x);
}

void schmear_winfac(t_schmear *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	schmear_init(x);
}

void schmear_fftinfo(t_schmear *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}


void schmear_mute(t_schmear *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void schmear_shift(t_schmear *x, t_floatarg f)
{
	x->shift = (int)f;
}

void schmear_schmimp(t_schmear *x, t_symbol *msg, short argc, t_atom *argv)
{
	int i;
	if(argc > MAXSCHMEAR){
		post("%d is too long for schmear", argc);
		return;
	}
	if(! (argc % 2) ){
		post("%s: impulse length %d must be odd",OBJECT_NAME, argc);
		return;
	}
	for( i = 0; i < argc; i++ ){
		atom_arg_getdouble(&x->spreader[i],i,argc,argv);
	}
	x->spreadlen = argc;
}

void schmear_threshold(t_schmear *x, t_floatarg t)
{
	x->threshold = (float)t;
}

void schmear_bypass(t_schmear *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void schmear_assist (t_schmear *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input"); break;
			case 1: sprintf(dst,"(signal/float) Threshold Generator"); break;
			case 2: sprintf(dst,"(signal/float) Multiplier for Weak Bins"); break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}

void *schmear_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft;
	t_schmear *x = (t_schmear *)object_alloc(schmear_class);
	dsp_setup((t_pxobject *)x,1);
	outlet_new((t_pxobject *)x, "signal");

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();
	fft->initialized = 0;
	x->threshold = 0.1;
	x->schmearmult = 0.1;
	x->mute = 0;
	atom_arg_getdouble(&x->threshold, 0, argc, argv);
	atom_arg_getdouble(&x->schmearmult, 1, argc, argv);
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	attr_args_process(x, argc, argv);
	schmear_init(x);
	return x;
}



void schmear_init(t_schmear *x)
{
	x->x_obj.z_disabled = 1;
	t_fftease  *fft = x->fft;
	fftease_init(fft);
	x->newamps = (double *) sysmem_newptrclear((fft->N2+1) * sizeof(double));
	x->spreadlen = 7;
	x->spreader = (double *) sysmem_newptrclear(MAXSCHMEAR * sizeof(double));
	x->spreader[0] = 0.6;
	x->spreader[1] = 0.3;
	x->spreader[2] = 0.15;
	x->spreader[3] = 0.0;
	x->spreader[4] = 0.15;
	x->spreader[5] = 0.3;
	x->spreader[6] = 0.6;
//	x->freakmode = 0;
	x->shift = 0;
	x->x_obj.z_disabled = 0;
}

void schmear_free(t_schmear *x)
{
	dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
	sysmem_freeptr(x->newamps);
	sysmem_freeptr(x->spreader);
}

void do_schmear(t_schmear *x)
{
	int i, j;
	t_fftease *fft = x->fft;
	double *channel = fft->channel;
	double frame_peak = 0.0, local_thresh;
	double threshold = x->threshold;
//	int shift = x->shift;
	int N = fft->N;
	int N2 = fft->N2;
	double *newamps = x->newamps;
	double *spreader = x->spreader;
	double curamp;
	int spreadlen = x->spreadlen;
	int spread_center = (spreadlen - 1) / 2;
	int thisbin;
//	short freakmode = x->freakmode;
	
	fold(fft);	
	rdft(fft,1);
	convert(fft);

	for(i = 0; i < N; i += 2){	
		if(frame_peak < channel[i])
			frame_peak = channel[i];
	}
	local_thresh = frame_peak * threshold;
	// clean newamps
	// memset(newamps, 0, N2 * sizeof(float));
	for(i = 0; i < N2; i++){
		newamps[i] = 0.0;
	}
	/*
	if( freakmode ){ // weird mistake version
		for(i = 0; i < N2; i++){	
			if(channel[i * 2] > local_thresh){
				curamp = channel[i * 2];
				for(j = i - spread_center; j <= i + spread_center; j++){
					if(j >= 0 && j < N2){
						newamps[j] += curamp * spreader[j + spread_center];
					}
				}
			}
		}  
	} 
	*/
	// no spread for now
	
		for(i = 0; i < N2; i++){	
			curamp = channel[i * 2];
			if(curamp > local_thresh){

				for(j = 0; j < spreadlen; j++){
					thisbin = i + j - spread_center;
					if(thisbin >= 0 && thisbin < N2){
						newamps[thisbin] += curamp * spreader[j];
					}
				}
			
			} else {
				newamps[i] = curamp;
			}
		}
	
	
	// shift works ok
	/*
	for( i = 0; i < N2; i++){
		if( i + shift < N2 && i + shift >= 0 ){
			newamps[i + shift] = channel[i * 2];
		}
	}*/

	// move amps back where they belong
	for(i = 0; i < N2; i++){
		channel[i * 2] = newamps[i];
	}
	if(fft->obank_flag){
		oscbank(fft);
	} else {
	unconvert(fft);
	rdft(fft,-1);
	overlapadd(fft);
	}
}

void schmear_perform64(t_schmear *x, t_object *dsp64, double **ins,
                       long numins, double **outs,long numouts, long vectorsize,
                       long flags, void *userparam)
{
	int	i,j;
	double *MSPInputVector = ins[0];
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
	
	if(x->mute || x->x_obj.z_disabled){
		for( j = 0; j < MSPVectorSize; j++) {
			*MSPOutputVector++ = *MSPInputVector++ * FFTEASE_BYPASS_GAIN;
		}
		return;
	}	
	if (x->bypass) {
		for( j = 0; j < MSPVectorSize; j++) {
			*MSPOutputVector++ = *MSPInputVector++ * FFTEASE_BYPASS_GAIN;
		}
		return;
	}
	if(x->fft->obank_flag)
		mult *= FFTEASE_OSCBANK_SCALAR;
//do_schmear(x);
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_schmear(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(MSPInputVector + (D*i), input + (Nw-D), D * sizeof(t_double));
            
			do_schmear(x);
			
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
            
			do_schmear(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}		

t_max_err get_fftsize(t_schmear *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_schmear *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		schmear_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_schmear *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_schmear *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		schmear_init(x);
	}
	return MAX_ERR_NONE;
}


void schmear_dsp64(t_schmear *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
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
        schmear_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,schmear_perform64,0,NULL);
}
