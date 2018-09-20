#include "fftease.h"

#define THRESHOLD_MIN (.000001)

static t_class *reanimator_class;

#define OBJECT_NAME "reanimator~"

typedef struct _reanimator
{
	t_pxobject x_obj;
	t_fftease *fft;
	double **framebank;
	double *normalized_frame;
	double current_frame;
	int framecount;
	double frame_increment ;
	double last_frame ;
	double fpos;
	double last_fpos;
	double tadv;
	int readme;
	int total_frames;
	short mute;
	short bypass;
	short initialized;
	double threshold;
	short inverse;
	int top_comparator_bin;
	short reanimator_mode;
	int matchframe; // current found frame
	double sample_len; /*duration of texture sample */
	double sync;
	int megs;
	long fftsize_attr;
	long overlap_attr;
} t_reanimator;

void *reanimator_new(t_symbol *msg, short argc, t_atom *argv);
void reanimator_assist(t_reanimator *x, void *b, long m, long a, char *s);
void reanimator_analyze (t_reanimator *x);
void reanimator_float(t_reanimator *x, double f) ;
void reanimator_mute(t_reanimator *x, t_floatarg flag);
void reanimator_inverse(t_reanimator *x, t_floatarg toggle);
void reanimator_topbin(t_reanimator *x, t_floatarg bin);
void reanimator_startframe(t_reanimator *x, t_floatarg start);
void reanimator_endframe(t_reanimator *x, t_floatarg end);
void reanimator_framerange(t_reanimator *x, t_floatarg start, t_floatarg end);
void reanimator_inverse(t_reanimator *x, t_floatarg toggle);
void reanimator_size(t_reanimator *x, t_floatarg size_ms);
void reanimator_freeze_and_march(t_reanimator *x, t_floatarg f);
void reanimator_resume( t_reanimator *x );
void reanimator_threshold(t_reanimator *x, t_floatarg threshold);
void reanimator_free( t_reanimator *x );
void reanimator_framecount ( t_reanimator *x );
void reanimator_init(t_reanimator *x);
void reanimator_fftinfo(t_reanimator *x);
//void reanimator_overlap(t_reanimator *x, t_floatarg f);
void reanimator_winfac(t_reanimator *x, t_floatarg f);
void reanimator_meminfo(t_reanimator *x);
//void reanimator_fftsize(t_reanimator *x, t_floatarg f);
void reanimator_transpose(t_reanimator *x, t_floatarg tf);
void reanimator_synthresh(t_reanimator *x, t_floatarg thresh);
void reanimator_oscbank(t_reanimator *x, t_floatarg flag);
void reanimator_bypass(t_reanimator *x, t_floatarg state);
t_max_err set_fftsize(t_reanimator *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_reanimator *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_reanimator *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_reanimator *x, void *attr, long *ac, t_atom **av);
void reanimator_dsp64(t_reanimator *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void reanimator_perform64(t_reanimator *x, t_object *dsp64, double **ins,
                    long numins, double **outs,long numouts, long vectorsize,
                    long flags, void *userparam);


int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.reanimator~", (method)reanimator_new, (method)reanimator_free, sizeof(t_reanimator),0,A_GIMME,0);
	class_addmethod(c,(method)reanimator_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)reanimator_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)reanimator_float,"float",A_FLOAT,0);
	class_addmethod(c,(method)reanimator_mute, "mute", A_FLOAT, 0);
	class_addmethod(c,(method)reanimator_bypass, "bypass", A_FLOAT, 0);
	class_addmethod(c,(method)reanimator_inverse, "inverse", A_FLOAT, 0);
	class_addmethod(c,(method)reanimator_topbin, "topbin", A_FLOAT, 0);
	class_addmethod(c,(method)reanimator_threshold, "threshold", A_FLOAT, 0);
	class_addmethod(c,(method)reanimator_analyze, "analyze", 0);
	class_addmethod(c,(method)reanimator_framecount, "framecount", 0);
	class_addmethod(c,(method)reanimator_freeze_and_march, "freeze_and_march", A_FLOAT, 0);
	class_addmethod(c,(method)reanimator_resume, "resume", 0);
//	class_addmethod(c,(method)reanimator_fftsize,"fftsize",A_FLOAT,0);
//	class_addmethod(c,(method)reanimator_overlap,"overlap",A_DEFFLOAT,0);
	class_addmethod(c,(method)reanimator_winfac,"winfac",A_DEFFLOAT,0);
	class_addmethod(c,(method)reanimator_fftinfo,"fftinfo",0);
	class_addmethod(c,(method)reanimator_meminfo,"meminfo",0);
	class_addmethod(c,(method)reanimator_oscbank,"oscbank",A_FLOAT,0);
	class_addmethod(c,(method)reanimator_transpose,"transpose",A_FLOAT,0);
	class_addmethod(c,(method)reanimator_synthresh,"synthresh",A_FLOAT,0);
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_reanimator, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_reanimator, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
	
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	reanimator_class = c;
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}

void reanimator_fftsize(t_reanimator *x, t_floatarg f)
{
	x->fft->N = (int) f;
	reanimator_init(x);
}

void reanimator_overlap(t_reanimator *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	reanimator_init(x);
}

void reanimator_winfac(t_reanimator *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	reanimator_init(x);
}

void reanimator_transpose(t_reanimator *x, t_floatarg tf)
{
	x->fft->P = (float) tf;
}

void reanimator_synthresh(t_reanimator *x, t_floatarg thresh)
{
	x->fft->synt = (float) thresh;
}

void reanimator_oscbank(t_reanimator *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}

void reanimator_meminfo(t_reanimator *x)
{
	post("%s currently stores %d FFT Frames, duration: %f sec., size: %.2f MB",
		 OBJECT_NAME, x->framecount, x->tadv * x->framecount, (float)x->megs/1000000.0);
}

void reanimator_fftinfo(t_reanimator *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);	
}

void reanimator_framecount ( t_reanimator *x )
{	
	post("%d frames stored", x->total_frames);
}

void reanimator_freeze_and_march(t_reanimator *x, t_floatarg f)
{	
	x->frame_increment = f;
	x->reanimator_mode = 1;
}

void reanimator_resume( t_reanimator *x )
{
	x->reanimator_mode = 0;
}

void reanimator_free( t_reanimator *x ){
	int i ;

	dsp_free( (t_pxobject *) x);
    if(x->fft->initialized){
        fftease_free(x->fft);
        for(i = 0; i < x->framecount; i++){
            sysmem_freeptr(x->framebank[i]) ;
        }
        sysmem_freeptr((char**)x->framebank);
        sysmem_freeptr(x->normalized_frame);
    }
}


void reanimator_assist (t_reanimator *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal) Driver Sound ");
				break;
			case 1:
				sprintf(dst,"(signal) Texture Sound");
				break;
				
		}
	} else if (msg==2) {
		switch (arg) {
			case 0:
				sprintf(dst,"(signal) Output");
				break;
			case 1:
				sprintf(dst,"(signal) Matched Frame");
				break;
			case 2:
				sprintf(dst,"(signal) Sync");
				break;
				
		}
	}
}


void *reanimator_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_fftease *fft;
	t_reanimator *x = (t_reanimator *)object_alloc(reanimator_class);
	dsp_setup((t_pxobject *)x,2);
	outlet_new((t_pxobject *)x, "signal");
	outlet_new((t_pxobject *)x, "signal");
	outlet_new((t_pxobject *)x, "signal");

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
    fft->initialized = 0;
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();
	
	
	x->sample_len = 1000.0;
	atom_arg_getdouble(&x->sample_len , 0, argc, argv);
	x->sample_len *= .001; /* convert to seconds */
	
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	attr_args_process(x, argc, argv);
	reanimator_init(x);	
	return x;
}

void reanimator_init(t_reanimator *x )
{
	t_fftease  *fft = x->fft;
	double **framebank = x->framebank;
	int framecount = x->framecount;
	x->x_obj.z_disabled = 1;
	short initialized = fft->initialized;
	
	fftease_init(fft);
	if(!fftease_msp_sanity_check(fft,OBJECT_NAME)){
		return;
	}
	// sanity check here	
	x->tadv = (float)fft->D/(float)fft->R;
	x->current_frame = framecount = 0;
	x->fpos = x->last_fpos = 0;
	x->total_frames =  x->sample_len / x->tadv;	

	
	if(!initialized){
		x->sync = 0.0;
		x->inverse = 0;
		x->initialized = 0; // for perform
		x->threshold = .0001;
		x->top_comparator_bin = 10;
		x->reanimator_mode = 0;
		x->frame_increment = 1.0;
		x->mute = 0;
		x->bypass = 0;
		x->readme = 0;		
		x->total_frames =  x->sample_len / x->tadv;
//		x->normalized_frame = (float *) calloc((fft->N + 2), sizeof(float));
		x->framebank = (double **) sysmem_newptrclear(x->total_frames * sizeof(double *));
		// post("tot frames: %d", x->total_frames);
		
		while(framecount < x->total_frames ){
			x->framebank[framecount] = (double *) sysmem_newptrclear((fft->N+2)*sizeof(double));
			++framecount;
		}
		
	}
    else if(initialized == 1){
			post("doing reinit! tot frames: %d", x->total_frames);

	// danger: could be more frames this time!!!
		while(framecount < x->total_frames ){
			x->framebank[framecount] = (double *) sysmem_resizeptrclear(framebank[framecount], (fft->N+2) * sizeof(double));
			++framecount;
		}
	}

	x->framecount = framecount;
	x->megs = sizeof(double) * x->framecount * (fft->N+2);
	x->x_obj.z_disabled = 0;
}

void do_reanimator(t_reanimator *x)
{
//	float sample, outsamp ;
	double ampsum, new_ampsum, rescale;
	double min_difsum, difsum;
	int	
	i,j;
	t_fftease *fft = x->fft;
	
	int framecount = x->framecount;
	int total_frames = x->total_frames;
	
	float threshold = x->threshold;
	int top_comparator_bin = x->top_comparator_bin ;
	
	double **framebank = x->framebank;
	// for reanimator mode
	double fframe = x->current_frame ;
	double last_fpos = x->last_fpos ;
	double fincr = x->frame_increment;
	double fpos = x->fpos ;
	double sync = x->sync;
	double *channel = fft->channel;
	double *output = fft->output;
	int matchframe = x->matchframe;
	int N = fft->N;
	int D = fft->D;
	float rescale_inv;
	/***********************************/

	if(total_frames <= 0)
		return;
	/* SAMPLE MODE */
	if( x->readme ) {
			
		
		if( framecount >= total_frames ){
			sync = 1.0;
			x->readme = 0;
			post("reanimator~: data acquisition completed");
			x->initialized = 1;
			// clear input buffer
			for( i = 0; i < fft->Nw; i++ ){
				fft->input[i] = 0.0;
			}
		} else {
			fold(fft);	
			rdft(fft,FFT_FORWARD);
			convert(fft);
			sync = (float) framecount / (float) total_frames;
			
			new_ampsum = ampsum = 0;
			for(i = 0; i < N; i += 2 ){
				ampsum += channel[i];
			}
			
			if( ampsum > .000001 ){
				rescale = 1.0 / ampsum ;
				
				// use more efficient memcpy
				for(i = 0; i < N; i++){
					framebank[framecount][i] = channel[i];
				}
				for( i = 0; i < N; i += 2 ){
					framebank[framecount][i] *= rescale;
				} 
				++framecount;

			} else {
				post("amplitude for frame %d is too low\n", framecount);
			}
		}		
		
	} 	/* reanimator RESYNTHESIS */
	else if(x->reanimator_mode) {   
		if( fpos < 0 )
			fpos = 0;
		if( fpos > 1 )
			fpos = 1;
		if( fpos != last_fpos ){
			fframe =  fpos * (float) framecount ;
			last_fpos = fpos;
		}
		
		
		fframe += fincr;
		while( fframe >= framecount ) {
			fframe -= framecount;
		} 
		while( fframe < 0. ) {
			fframe += framecount ;
		}
		matchframe = (int) fframe;
		
		// use memcopy
		for(i = 0; i < N; i++){
			channel[i] = framebank[matchframe][i];
		}
		if(fft->obank_flag){
			oscbank(fft);
		} else {
			unconvert(fft);
			rdft(fft,FFT_INVERSE);
			overlapadd(fft);
		}
		
		
	}
	/* REANIMATION HERE */
	else {
		fold(fft);	
		rdft(fft,FFT_FORWARD);
		convert(fft);
		ampsum = 0;
		// NORMALIZE INPUT FRAME
		for( i = 0; i < N; i += 2 ){
			ampsum += channel[i];
		}
		
		if( ampsum > threshold ){
			rescale = 1.0 / ampsum;
			for( i = 0; i < N; i += 2 ){
				channel[i] *= rescale;
			}
		} 
		else {
			// AMPLITUDE OF INPUT WAS TOO LOW - OUTPUT SILENCE AND RETURN
			for (i = 0; i < D; i++ ){
				output[i] = 0.0;
			}
			matchframe = 0;		
			x->current_frame = fframe;
			x->frame_increment = fincr;
			x->fpos = fpos;
			x->sync = sync;
			x->framecount = framecount;
			x->matchframe = matchframe;
			return;
			
		}
		// NOW COMPARE TO STORED FRAMES
		if( x->inverse ){ // INVERSE CASE
			min_difsum = 0.0 ;
			
			for( j = 0; j < framecount; j++ ){
				difsum = 0;
				for( i = 0; i < top_comparator_bin * 2; i += 2 ){
					difsum += fabs( channel[i] - framebank[j][i] ); 
				}
				//      fprintf(stderr,"bin 20: in %f compare %f\n", channel[40], frames[j][40]);
				if( difsum > min_difsum ){
					matchframe = j;
					min_difsum = difsum;
				}
			}
		} else { // NORMAL CASE
			min_difsum = 1000000.0 ;
			
			for( j = 0; j < framecount; j++ ){
				difsum = 0;
				for( i = 0; i < top_comparator_bin * 2; i += 2 ){
					difsum += fabs( channel[i] - framebank[j][i] ); 
				}
				//      fprintf(stderr,"bin 20: in %f compare %f\n", channel[40], frames[j][40]);
				if( difsum < min_difsum ){
					matchframe = j;
					min_difsum = difsum;
				}
			}
		}
		// use memcopy
		for(i = 0; i < N; i++){
			channel[i] = framebank[matchframe][i];
		}		
		if(fft->obank_flag){
			oscbank(fft);
		} else {
			unconvert(fft);
			rdft(fft,FFT_INVERSE);
			overlapadd(fft);
		}
		
		// scale back to match
		rescale_inv = 1.0 / rescale;
		for (i = 0; i < D; i++){
			output[i] *= rescale_inv;
		}
	}
	
	/* restore state variables */
	x->current_frame = fframe;
	x->frame_increment = fincr;
	x->fpos = fpos;
	x->sync = sync;
	x->framecount = framecount;
	x->matchframe = matchframe;
}

void reanimator_perform64(t_reanimator *x, t_object *dsp64, double **ins,
                            long numins, double **outs,long numouts, long vectorsize,
                            long flags, void *userparam)
{
	int		i,j;
	
	////////////////////////////////////////////// 
	double *driver = ins[0]; // was driver
	double *texture = ins[1];
	double *MSPOutputVector = outs[0]; // was soundout
	double *matchout = outs[1];
	double *sync_vec = outs[2];
	
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

	/***********************************/
	if(x->mute || x->x_obj.z_disabled || ! x->initialized){
        for(i=0; i < vectorsize; i++){ MSPOutputVector[i] = 0.0; }
		return;
	}	
	if (x->bypass) {
		for( j = 0; j < MSPVectorSize; j++) {
			*MSPOutputVector++ = *driver++ * FFTEASE_BYPASS_GAIN;
		}
		return;
	}
	if( fft->obank_flag )
		mult *= FFTEASE_OSCBANK_SCALAR;
		
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        
		if(x->readme){
            sysmem_copyptr(texture, input + (Nw - D), D * sizeof(t_double));
		} else {
            sysmem_copyptr(driver, input + (Nw - D), D * sizeof(t_double));
		}
		do_reanimator(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}	
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
			if(x->readme){
				//memcpy(input + (Nw - D), texture + (D * i), D * sizeof(float));
                sysmem_copyptr(texture + (D*i), input + (Nw-D), D * sizeof(t_double));
			} else {
                sysmem_copyptr(driver + (D*i), input + (Nw-D), D * sizeof(t_double));
				//memcpy(input + (Nw - D), driver + (D * i), D * sizeof(float));
			}
			do_reanimator(x);
			for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
	}
	else if( fft->bufferStatus == BIGGER_THAN_MSP_VECTOR ) {
		if(x->readme){
			//memcpy(internalInputVector + (operationCount * MSPVectorSize), texture, MSPVectorSize * sizeof(float));
            sysmem_copyptr(texture,internalInputVector + (operationCount * MSPVectorSize),MSPVectorSize * sizeof(t_double));
		} else {
            sysmem_copyptr(driver,internalInputVector + (operationCount * MSPVectorSize),MSPVectorSize * sizeof(t_double));
			//memcpy(internalInputVector + (operationCount * MSPVectorSize), driver, MSPVectorSize * sizeof(float));
		}
		//memcpy(MSPOutputVector, internalOutputVector + (operationCount * MSPVectorSize), MSPVectorSize * sizeof(float));
        sysmem_copyptr(internalOutputVector + (operationCount * MSPVectorSize),MSPOutputVector,MSPVectorSize * sizeof(t_double));
		operationCount = (operationCount + 1) % operationRepeat;

		if( operationCount == 0 ) {
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(internalInputVector, input + (Nw - D), D * sizeof(t_double));

			do_reanimator( x );

			for ( j = 0; j < D; j++ ){
				internalOutputVector[j] = output[j] * mult;
			}
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
        fft->operationCount = operationCount;
	}	
	// now load other output buffers
	for(i = 0; i < MSPVectorSize; i++){
		matchout[i] = x->matchframe;
		sync_vec[i] = x->sync;
	}
}

	
void reanimator_float(t_reanimator *x, double f) // Look at floats at inlets
{
	int inlet = x->x_obj.z_in;
	
	if (inlet == 1)
    {
		x->frame_increment = f;
    }
	else if (inlet == 2)
    {
		if (f < 0 ){
			f = 0;
		} else if(f > 1) {
			f = 1.;
		}
		x->fpos = f;
		
    }
}

void reanimator_analyze ( t_reanimator *x )
{
	// int i, j;
	
	x->readme = 1;
	x->initialized = 1;
	x->framecount = 0;
	post("reanimator: beginning spectral data acquisition");
	return;
	
}

void reanimator_mute(t_reanimator *x, t_floatarg flag)
{
	x->mute = (short)flag;	
	//  post ("mute set to %d", flag);
}

void reanimator_bypass(t_reanimator *x, t_floatarg state)
{
	x->bypass = (short)state;	
}


void reanimator_topbin(t_reanimator *x, t_floatarg bin)
{
	if( bin > 1 && bin < x->fft->N2 )
		x->top_comparator_bin = bin;
}


void reanimator_inverse(t_reanimator *x, t_floatarg toggle)
{
	x->inverse = (short)toggle;	
	//  post ("inverse set to %d", toggle);
}

void reanimator_threshold(t_reanimator *x, t_floatarg threshold)
{
	if( threshold > THRESHOLD_MIN )
		x->threshold = threshold;
	else
		x->threshold = THRESHOLD_MIN;	
}
t_max_err get_fftsize(t_reanimator *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_reanimator *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		reanimator_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_reanimator *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_reanimator *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		reanimator_init(x);
	}
	return MAX_ERR_NONE;
}


void reanimator_dsp64(t_reanimator *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
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
        reanimator_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,reanimator_perform64,0,NULL);
}


