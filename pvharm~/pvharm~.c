#include "fftease.h"

static t_class *pvharm_class;

#define OBJECT_NAME "pvharm~"

typedef struct _pvharm
{
	t_pxobject x_obj;
	t_fftease *fft;
	t_fftease *fft2;
	long fftsize_attr;
	long overlap_attr;
	short connected[3];
	short mute;
	short bypass;
	short peakflag;
	double hifreq;/* highest frequency to synthesize */
	double lofreq;/* lowest frequency to synthesize */
	double framethresh; /* set lower limit for synth cutoff, to avoid noise */
	int osclimit;
	int oscnt;
	double local_thresh;
	double framepeak;
	double *ampsort; /* sort amplitudes from highest to lowest */
	double threshfloor;
	short compressor; /* flag to compress */
	double framestop; /* amplitude below which compressor is turned off */
} t_pvharm;


void *pvharm_new(t_symbol *s, int argc, t_atom *argv);
void pvharm_assist(t_pvharm *x, void *b, long m, long a, char *s);
void pvharm_float(t_pvharm *x, double f);
void pvharm_mute(t_pvharm *x, t_floatarg f);
void pvharm_init(t_pvharm *x);
void pvharm_rel2peak(t_pvharm *x, t_floatarg toggle);
void pvharm_free(t_pvharm *x);
void pvharm_oscnt(t_pvharm *x);
void pvharm_fftinfo(t_pvharm *x);
void pvharm_winfac(t_pvharm *x, t_floatarg f);
void pvharm_osclimit(t_pvharm *x, t_floatarg f);
void pvharm_threshfloor(t_pvharm *x, t_floatarg f);
void pvharm_bypass(t_pvharm *x, t_floatarg toggle);
void pvharm_compressor(t_pvharm *x, t_floatarg state);
void pvharm_framestop(t_pvharm *x, t_floatarg state);
t_max_err set_fftsize(t_pvharm *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_pvharm *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_pvharm *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_pvharm *x, void *attr, long *ac, t_atom **av);
t_max_err set_lowfreq(t_pvharm *x, void *attr, long ac, t_atom *av);
t_max_err get_lowfreq(t_pvharm *x, void *attr, long *ac, t_atom **av);
t_max_err set_highfreq(t_pvharm *x, void *attr, long ac, t_atom *av);
t_max_err get_highfreq(t_pvharm *x, void *attr, long *ac, t_atom **av);

void pvharm_dsp64(t_pvharm *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void pvharm_perform64(t_pvharm *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam);


int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.pvharm~",(method)pvharm_new, (method)pvharm_free, sizeof(t_pvharm),0,A_GIMME,0);
	
	class_addmethod(c,(method)pvharm_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)pvharm_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)pvharm_mute,"mute",A_FLOAT,0);
	class_addmethod(c,(method)pvharm_bypass,"bypass", A_FLOAT, 0);
	class_addmethod(c,(method)pvharm_rel2peak,"rel2peak",A_FLOAT,0);
 	class_addmethod(c,(method)pvharm_winfac,"winfac",A_DEFFLOAT,0);
	class_addmethod(c,(method)pvharm_osclimit,"osclimit",A_DEFFLOAT,0);
	class_addmethod(c,(method)pvharm_threshfloor,"threshfloor",A_DEFFLOAT,0);
	class_addmethod(c,(method)pvharm_fftinfo,"fftinfo",0);  
	class_addmethod(c,(method)pvharm_oscnt,"oscnt",0);
	class_addmethod(c,(method)pvharm_compressor,"compressor",A_FLOAT,0);
	class_addmethod(c,(method)pvharm_framestop,"framestop",A_FLOAT,0);
	class_addmethod(c,(method)pvharm_float,"float",A_FLOAT,0);
	class_dspinit(c);
	
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_pvharm, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	

	CLASS_ATTR_FLOAT(c, "overlap", 0, t_pvharm, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	

	CLASS_ATTR_FLOAT(c, "lowfreq", 0, t_pvharm, lofreq);
	CLASS_ATTR_ACCESSORS(c, "lowfreq", (method)get_lowfreq, (method)set_lowfreq);
	CLASS_ATTR_LABEL(c, "lowfreq", 0, "Minimum Frequency");	

	CLASS_ATTR_FLOAT(c, "hifreq", 0, t_pvharm, hifreq);
	CLASS_ATTR_ACCESSORS(c, "hifreq",(method)get_highfreq, (method)set_highfreq);
	CLASS_ATTR_LABEL(c, "hifreq", 0, "Maximum Frequency");	
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	CLASS_ATTR_ORDER(c, "lowfreq",    0, "3");
	CLASS_ATTR_ORDER(c, "hifreq",    0, "4");
	
	class_register(CLASS_BOX, c);
	pvharm_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

t_max_err set_fftsize(t_pvharm *x, void *attr, long ac, t_atom *av)
{

	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		x->fft2->N = (int) val;
		pvharm_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_fftsize(t_pvharm *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_overlap(t_pvharm *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		x->fft2->overlap = (int) val;
		pvharm_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_pvharm *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_lowfreq(t_pvharm *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		t_fftease *fft = x->fft;
		t_fftease *fft2 = x->fft2;
		double f = atom_getfloat(av);
		if(f > x->hifreq){
			error("%s: minimum cannot exceed current maximum: %f",OBJECT_NAME,x->hifreq);
			return MAX_ERR_NONE;
		}
		if(f < 0 ){
			f = 0;
		}	
		x->lofreq = f;
		fftease_oscbank_setbins(fft,x->lofreq, x->hifreq);
		fftease_oscbank_setbins(fft2,x->lofreq, x->hifreq);
	}
	return MAX_ERR_NONE;
}


t_max_err get_lowfreq(t_pvharm *x, void *attr, long *ac, t_atom **av)
{
	if (ac && av) {
		char alloc;
		
		if (atom_alloc(ac, av, &alloc)) {
			return MAX_ERR_GENERIC;
		}
		atom_setfloat(*av, x->lofreq);
	}
	return MAX_ERR_NONE;
	
}

t_max_err set_highfreq(t_pvharm *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		t_fftease *fft = x->fft;
		t_fftease *fft2 = x->fft2;
		int R = x->fft->R;
		double f = atom_getfloat(av);
		if(f < x->lofreq){
			error("%s: maximum cannot go below current minimum: %f",OBJECT_NAME,x->lofreq);
			return MAX_ERR_NONE;
		}
		if(f > R/2 ){
			f = R/2;
		}	
		x->hifreq = f;
		fftease_oscbank_setbins(fft,x->lofreq, x->hifreq);
		fftease_oscbank_setbins(fft2,x->lofreq, x->hifreq);
	}
	return MAX_ERR_NONE;
}

t_max_err get_highfreq(t_pvharm *x, void *attr, long *ac, t_atom **av)
{
	if (ac && av) {
		char alloc;
		
		if (atom_alloc(ac, av, &alloc)) {
			return MAX_ERR_GENERIC;
		}
		atom_setfloat(*av, x->hifreq);
	}
	return MAX_ERR_NONE;
}


void pvharm_oscnt(t_pvharm *x)
{
	post("%s: osc count: %d, local thresh: %f, frame peak: %f",OBJECT_NAME, x->oscnt, x->local_thresh, x->framepeak);
}

void pvharm_free(t_pvharm *x)
{
	dsp_free((t_pxobject *) x);
	fftease_free(x->fft);
	fftease_free(x->fft2);
    sysmem_freeptr(x->fft);
    sysmem_freeptr(x->fft2);
	sysmem_freeptr(x->ampsort);
}

void pvharm_rel2peak(t_pvharm *x, t_floatarg toggle)
{
	x->peakflag = (short)toggle;
}

void pvharm_threshfloor(t_pvharm *x, t_floatarg f)
{
	x->threshfloor = (float)f;
}

void pvharm_assist (t_pvharm *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input"); break;
			case 1: sprintf(dst,"(signal/float) Pitch Multiplier 1"); break;
			case 2: sprintf(dst,"(signal/float) Pitch Multiplier 2"); break;
			case 3: sprintf(dst,"(signal/float) Synthesis Threshold"); break;
		}
	} else if (msg==2) {
		sprintf(dst,"(signal) Output");
	}
}

void pvharm_winfac(t_pvharm *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	x->fft2->winfac = (int) f;
	pvharm_init(x);
}

void pvharm_fftinfo(t_pvharm *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
	fftease_fftinfo(x->fft2, OBJECT_NAME);
}

void pvharm_version(void)
{
	fftease_version(OBJECT_NAME);
}




void *pvharm_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft, *fft2;
	t_pvharm *x = (t_pvharm *)object_alloc(pvharm_class);
	dsp_setup((t_pxobject *)x,4);
	outlet_new((t_pxobject *)x, "signal");

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	x->fft2 = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft2 = x->fft2;
	fft2->R = fft->R = sys_getsr();
	fft2->MSPVectorSize = fft->MSPVectorSize = sys_getblksize();	
	fft->initialized = 0;
	fft2->initialized = 0;
	x->lofreq = 0;
	x->hifreq = 4000;
	fft2->N = fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft2->overlap = fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft2->winfac = fft->winfac = FFTEASE_DEFAULT_WINFAC;

	attr_args_process(x, argc, argv);
	pvharm_init(x);
	return x;
}

void pvharm_init(t_pvharm *x)
{
	t_fftease  *fft = x->fft;
	t_fftease  *fft2 = x->fft2;
	short initialized = fft->initialized;
	x->x_obj.z_disabled = 1;
	fftease_init(fft);
	fftease_init(fft2);
    
	if(!initialized){
		x->framethresh = 0.005;
		x->osclimit = fft->N2;
		fft->P = .5 ; // for testing purposes
		fft2->P = .6666666666 ; // for testing purposes
		
		x->mute = 0;
		x->compressor = 0;
		x->framestop = .001;
		x->ampsort = (double *) sysmem_newptrclear((fft->N+1) * sizeof(double));
	} else if(initialized == 1){
		x->ampsort = (double *) sysmem_resizeptrclear(x->ampsort, (fft->N+1) * sizeof(double));
	}
	fftease_oscbank_setbins(fft, x->lofreq, x->hifreq);
	fftease_oscbank_setbins(fft2, x->lofreq, x->hifreq);	
	x->x_obj.z_disabled = 0;
}

void do_pvharm(t_pvharm *x)
{
	double framethresh = x->framethresh;
	int osclimit = x->osclimit;
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int i;
	int D = fft->D;
	int freq, amp,chan;
	double framesum, frame_rescale;
	double framestop = x->framestop;
	double *channel = fft->channel;
	double *channel2 = fft2->channel;
	double *output = fft->output;
	double *output2 = fft2->output;
	int lo_bin = fft->lo_bin;
	int hi_bin = fft->hi_bin;

	fold(fft);
	rdft(fft,FFT_FORWARD);
	convert(fft);
	
	if(x->compressor){
		framesum = 0.0;
		for(chan = fft->lo_bin; chan < fft->hi_bin; chan++){
			amp = chan << 1;
			framesum += channel[amp];
		}
		if(framesum > framestop && framesum >= 0.0){
			frame_rescale = 1.0 / framesum;
			for(chan = lo_bin; chan < hi_bin; chan++){
				amp = chan << 1;
				channel[amp] *= frame_rescale;		
			}
		}	
	}
	// copy spectrum to second channel (yes it is inefficient)

	for(chan = lo_bin; chan < hi_bin; chan++){
		amp = chan << 1;
		freq = amp + 1;
		channel2[amp] = channel[amp];
		channel2[freq] = channel[freq];
	} 
	limited_oscbank(fft, osclimit, framethresh);
	limited_oscbank(fft2, osclimit, framethresh);
	for(i = 0; i < D; i++){
		output[i] += output2[i];
	}
}

void pvharm_perform64(t_pvharm *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam)
{
	int i,j; 	
	double *MSPInputVector = ins[0];
	double *in2 = ins[1];
	double *in3 = ins[2];
	double *in4 = ins[3];
	double *MSPOutputVector = outs[0];
	
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int D = fft->D;
	int Nw = fft->Nw;
	double *input = fft->input;
	double *output = fft->output;
	double *output2 = fft2->output;
	double mult = fft->mult;
	int MSPVectorSize = fft->MSPVectorSize;
	double *internalInputVector = fft->internalInputVector;
	double *internalOutputVector = fft->internalOutputVector;		
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	short *connected = x->connected;		
	
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

	if(connected[1]) {
		fft->P = *in2;
	}
	if(connected[2]) {
		fft2->P = *in3;
	}
	if(connected[3]){
		fft->synt = fft2->synt =  *in4;
	}
	
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
		do_pvharm(x);
		for ( j = 0; j < D; j++ ){
			*MSPOutputVector++ = output[j] * mult;
        }
        sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
        sysmem_copyptr(output2 + D, output2, (Nw-D) * sizeof(t_double));
        for(j = (Nw-D); j < Nw; j++){ output2[j] = 0.0; }

	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(MSPInputVector + (D*i), input + (Nw-D), D * sizeof(t_double));
            
			do_pvharm(x);
            for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
            sysmem_copyptr(output2 + D, output2, (Nw-D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output2[j] = 0.0; }
		}
	}
	else if( fft->bufferStatus == BIGGER_THAN_MSP_VECTOR ) {
        sysmem_copyptr(MSPInputVector,internalInputVector + (operationCount * MSPVectorSize),MSPVectorSize * sizeof(t_double));
        sysmem_copyptr(internalOutputVector + (operationCount * MSPVectorSize),MSPOutputVector,MSPVectorSize * sizeof(t_double));
		operationCount = (operationCount + 1) % operationRepeat;
		
		if( operationCount == 0 ) {
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(internalInputVector, input + (Nw - D), D * sizeof(t_double));
			
			do_pvharm( x );
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
            sysmem_copyptr(output2 + D, output2, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output2[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}	
}		

void pvharm_float(t_pvharm *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	
	if (inlet == 1)
    {
		x->fft->P = f;
		
    }
	else if (inlet == 2)
    {
		x->fft2->P = f;
    }
	else if (inlet == 3)
    {
		x->fft->synt = x->fft2->synt = f;
    }
	
}

void pvharm_compressor(t_pvharm *x, t_floatarg state)
{
	x->compressor = (short)state;	
}

void pvharm_framestop(t_pvharm *x, t_floatarg state)
{
	x->framestop = state;	
}

void pvharm_bypass(t_pvharm *x, t_floatarg toggle)
{
	x->bypass = (short)toggle;
}

void pvharm_mute(t_pvharm *x, t_floatarg state)
{
	x->mute = (short)state;	
}

void pvharm_osclimit(t_pvharm *x, t_floatarg limit)
{
	x->osclimit = (int)limit;	
}


void pvharm_dsp64(t_pvharm *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    int i;
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
	for(i = 0; i < 3; i++){
		x->connected[i] = count[i];
	}
	
	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
        fftease_set_fft_buffers(fft2);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
	}
    if(reset_required){
        pvharm_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,pvharm_perform64,0,NULL);
}
