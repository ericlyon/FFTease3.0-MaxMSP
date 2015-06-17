#include "fftease.h"

/* Not ported to Pd due to array/buffer difference */
// #define FLEN 1024

static t_class *resident_class;

#define OBJECT_NAME "residency_buffer~"

typedef struct _resident
{
	t_pxobject x_obj;
	t_fftease *fft;
	
	t_buffer_ref *resbuf_ref;
    long b_frames;
    long b_valid;
	double current_frame;
	int framecount;
	//
	double frame_increment ;
	double fpos;
	double last_fpos;
	double tadv;
	long read_me;
	long frames_read;
	long MAXFRAMES;
	short mute;
	short connected[2];
	short in2_connected;
	short in3_connected;
	long buffer_frame_count;
	short initialized;
	short playthrough;
	double sync;
	short buffer_is_hosed;
	long fftsize_attr;
	long overlap_attr;
    long interpolation_attr;
    t_symbol *buffername_attr;
	void *size_outlet; // will send desired size in samples
    
} t_resident;

void *resident_new(t_symbol *msg, short argc, t_atom *argv);
void resident_assist(t_resident *x, void *b, long m, long a, char *s);
void resident_acquire_sample ( t_resident *x ) ;
void resident_meminfo( t_resident *x ) ;
void resident_float(t_resident *x, double f) ;
void resident_mute(t_resident *x, long toggle);
void resident_calcbuf(t_resident *x, t_floatarg desired_duration);
void resident_dsp_free( t_resident *x );
void resident_fftinfo(t_resident *x);
void resident_winfac(t_resident *x, t_floatarg f);
void resident_playthrough(t_resident *x, t_floatarg f);
//void resident_overlap(t_resident *x, t_floatarg f);
//void resident_fftsize(t_resident *x, t_floatarg f);
void resident_init(t_resident *x);
void resident_transpose(t_resident *x, t_floatarg tf);
void resident_synthresh(t_resident *x, t_floatarg thresh);
void resident_oscbank(t_resident *x, t_floatarg flag);
t_max_err set_fftsize(t_resident *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_resident *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_resident *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_resident *x, void *attr, long *ac, t_atom **av);
void resident_dsp64(t_resident *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void resident_perform64(t_resident *x, t_object *dsp64, double **ins,
                    long numins, double **outs,long numouts, long vectorsize,
                    long flags, void *userparam);
int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.residency_buffer~", (method)resident_new, (method)resident_dsp_free, 
				  sizeof(t_resident),0,A_GIMME,0);
	
	class_addmethod(c,(method)resident_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)resident_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)resident_mute, "mute", A_LONG, 0);
	class_addmethod(c,(method)resident_meminfo, "meminfo", 0);
	class_addmethod(c,(method)resident_calcbuf, "calcbuf", A_FLOAT, 0);
	class_addmethod(c,(method)resident_winfac, "winfac", A_FLOAT, 0);
//	class_addmethod(c,(method)resident_overlap, "overlap", A_FLOAT, 0);
//	class_addmethod(c,(method)resident_fftsize, "fftsize", A_FLOAT, 0);
	class_addmethod(c,(method)resident_playthrough, "playthrough", A_FLOAT, 0);
	class_addmethod(c,(method)resident_acquire_sample, "acquire_sample", 0);
	class_addmethod(c,(method)resident_fftinfo, "fftinfo", 0);
	class_addmethod(c,(method)resident_oscbank,"oscbank",A_FLOAT,0);
	class_addmethod(c,(method)resident_transpose,"transpose",A_FLOAT,0);
	class_addmethod(c,(method)resident_synthresh,"synthresh",A_FLOAT,0);
	class_addmethod(c,(method)resident_float,"float",A_FLOAT,0);
	class_addmethod(c,(method)resident_acquire_sample,"bang",0);
	
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_resident, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_resident, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	

    CLASS_ATTR_LONG(c, "interpolation", 0, t_resident, interpolation_attr);
    CLASS_ATTR_ENUMINDEX(c, "interpolation", 0, "OFF ON")
    CLASS_ATTR_LABEL(c, "interpolation", 0, "Interpolation");
    
    CLASS_ATTR_SYM(c,"buffername",0, t_resident, buffername_attr);
    
	CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
	CLASS_ATTR_ORDER(c, "overlap",    0, "2");
    CLASS_ATTR_ORDER(c, "interpolation",    0, "3");
    
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	resident_class = c;	
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;

}

void resident_meminfo( t_resident *x )
{	
	long mem;
	// resident_attach_buffer(x);
	mem = x->buffer_frame_count * (x->fft->N + 2) * sizeof(double);
    post("%d frames in buffer", x->buffer_frame_count);
    post("frame_duration: %f, actual time in buffer: %f", x->tadv, (double)(x->buffer_frame_count) * x->tadv);
    post("actual time in buffer: %f", (double)(x->buffer_frame_count) * x->tadv);   
	post("buffer memory: %.2f MB", (double)mem / 1000000.0);

	t_buffer_obj *b;
    

    if(!x->resbuf_ref){
        x->resbuf_ref = buffer_ref_new((t_object*)x,x->buffername_attr);
    }
    buffer_ref_set(x->resbuf_ref, x->buffername_attr);
    b = buffer_ref_getobject(x->resbuf_ref);
    if(b == NULL){
        error("%s: no such buffer~ %s",OBJECT_NAME, x->buffername_attr->s_name);
    }
    x->b_frames = buffer_getframecount(b);
    post("%d frames found in buffer %s",x->b_frames, x->buffername_attr->s_name);
}

void resident_fftsize(t_resident *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	resident_init(x);
}

void resident_fftinfo(t_resident *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}


void resident_dsp_free( t_resident *x ){
	dsp_free( (t_pxobject *) x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
	
}

void resident_calcbuf(t_resident *x, t_floatarg desired_duration)
{
	double ms_calc;
	double seconds;
	double frames;
	double samples;
	double tadv = x->tadv;
	t_fftease *fft = x->fft;
	
	if(tadv == 0){
		post("zero tadv!");
		return;
	}
	seconds = desired_duration / 1000.0;
	frames = seconds / tadv;
	samples = frames * (double) (fft->N + 2);
	ms_calc = (samples / fft->R) * 1000.0;
	post("desired duration in ms: %f",desired_duration);
	post("you need %.0f samples in buffer to get %.0f frames or %f secs", 
		 samples, frames, seconds);
	outlet_int(x->size_outlet, (int) samples);
	
}

void resident_assist (t_resident *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0:sprintf(dst,"(signal/bang) Input, Sampling Trigger");break;
			case 1:sprintf(dst,"(signal/float) Frame Increment");break;
			case 2:sprintf(dst,"(signal/float) Frame Position [0-1]");break;
				
		}
	} else if (msg==2) {
		switch(arg) {
			case 0: sprintf(dst,"(signal) Output"); break;
			case 1: sprintf(dst,"(signal) Record Sync"); break;
			case 2: sprintf(dst,"(signal) Desired Buffer Size in Samples"); break;
		}
	}
}

void *resident_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_resident *x = (t_resident *)object_alloc(resident_class);
	t_fftease *fft;
	
	if(argc < 1){
		error("%s: you must provide the name of a valid buffer.",OBJECT_NAME);
		x->x_obj.z_disabled = 1;
		return 0; // kills object for good
	} 	
	
	dsp_setup((t_pxobject *)x,3);
	x->size_outlet = intout((t_object *)x);
	outlet_new((t_pxobject *)x, "signal");
	outlet_new((t_pxobject *)x, "signal");
	
    
	// THIS WAS A BUG - needed to malloc while connected to x
	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;
	fft->initialized = 0;	
	fft->R = sys_getsr();
	fft->MSPVectorSize = sys_getblksize();
	
//	x->resbuf_ref = buffer_ref_new((t_object*)x, x->buffername_attr);
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	
	attr_args_process(x, argc, argv);
	resident_init(x);  
	return x;
}

void resident_init(t_resident *x)
{
// crashes if bad values
	x->x_obj.z_disabled = 1;
	t_fftease *fft = x->fft;
	short initialized = x->fft->initialized;
	if( fft->R <= 0 ){
		post("bad SR");
		return;
	}
	if( fft->MSPVectorSize <= 0 ){
		post("bad vectorsize");
		return;
	}	

//	post("runinng init");
	
	fftease_init(fft);	
//	return;
	x->tadv = (double)fft->D / (double)fft->R;
	if(!initialized){
		x->mute = 0;
		x->sync = 0;
		x->initialized = 1;
		x->current_frame = x->framecount = 0;
		x->frame_increment = 1.0 ;
		x->fpos = x->last_fpos = 0;
		// safety
	}
	x->x_obj.z_disabled = 0;

}

void do_resident(t_resident *x)
{
	t_fftease *fft = x->fft;
	int N = x->fft->N;
	int i,j,k;
	double fframe = x->current_frame ;
	double fincr = x->frame_increment;
	double fpos = x->fpos;
	double last_fpos = x->last_fpos ;
	double *channel = fft->channel;
	float *b_samples;
	long b_frames = x->b_frames;
    long b_nchans;
	int frames_read = x->frames_read;
	long index_offset;
	long buffer_frame_count = x->buffer_frame_count;
    long index1, index2;
    double frak;
    
    t_buffer_obj *dbuf;
 //   post("read status: %d", x->read_me);
	if(!x->resbuf_ref){
        x->resbuf_ref = buffer_ref_new((t_object*)x, x->buffername_attr);
    } else {
        buffer_ref_set(x->resbuf_ref, x->buffername_attr);
    }
    dbuf = buffer_ref_getobject(x->resbuf_ref);
    // bad buffer
    if(dbuf == NULL){
        post("%s: nonexistent buffer - %s",OBJECT_NAME, x->buffername_attr->s_name);
        return;
    }
    b_nchans = buffer_getchannelcount(dbuf);
    b_frames = buffer_getframecount(dbuf);
    buffer_frame_count = (int)((double) b_frames / (double)(x->fft->N + 2));
    if(b_frames < 1 || b_nchans != 1){
        post("%s: table too small or not mono",OBJECT_NAME);
        return;
    }
    
    b_samples = buffer_locksamples(dbuf);
	if( x->read_me ) {
        // goto escape;
		fold(fft);	
		rdft(fft, FFT_FORWARD);
		convert(fft);
		
		index_offset = (N+2) * frames_read;
		
		for(i = index_offset, j = 0; i < index_offset + N + 2; i++, j++){
            if(i >= b_frames){
                post("hit end of buffer on frame %d", frames_read);
                goto escape;
            }
			b_samples[i] = channel[j];
		}
		
		++frames_read;
		// output empty buffers while reading
		x->sync = (double)frames_read/(double)(buffer_frame_count);
		
       //  post("read %d of %d, sync %f", frames_read, buffer_frame_count, x->sync);
		if( frames_read >= buffer_frame_count){
			x->read_me = 0;
			// post("%s: data acquisition completed",OBJECT_NAME);
            fpos = 0.0;
		} 
	} 
	else {
		// 
        
		if( fpos < 0 )
			fpos = 0;
		if( fpos > 1 )
			fpos = 1;
		if( fpos != last_fpos ){
			fframe =  fpos * (double) buffer_frame_count;
			last_fpos = fpos;
		}
		
		fframe += fincr;
        // post("fframe %f framecount %d", fframe, buffer_frame_count);
        
		while(fframe >= buffer_frame_count) {
			fframe -= buffer_frame_count;
		} 
		while( fframe < 0. ) {
			fframe += buffer_frame_count;
		}
		// goto escape;
		
		if(x->interpolation_attr == 1){
            long iframe = floor(fframe);
            index1 = (N+2) * iframe;
            index2 = (N+2) * ((iframe + 1) % buffer_frame_count);
            frak = fframe - iframe;
            for( i = index1, j = index2, k = 0; i < index1 + N + 2; i++, j++, k++){
                if(i >= b_frames || j >= b_frames){
                    post("hit end of buffer on frame %d, index %d %d", index1,i,j);
                    goto escape;
                }
                channel[k] = b_samples[i] + frak * (b_samples[j] - b_samples[i]);
            }
        }
        else {
            index_offset = (N+2) * (long) fframe;
            for( i = index_offset, j = 0; i < index_offset + N + 2; i++, j++ ){
                if(i >= b_frames){
                    post("hit end of buffer on frame %d, index %d", index_offset,i);
                    goto escape;
                }
                channel[j] = b_samples[i];
            }
        }
		x->sync = fframe / (double) buffer_frame_count;
		// REPLACE loveboat with buffer
		if(fft->obank_flag){
			oscbank(fft);
		}
        else {
			unconvert(fft);
			rdft(fft, FFT_INVERSE);
			overlapadd(fft);
		}
	}
escape:
    ;
	/* restore state variables */
	buffer_unlocksamples(dbuf);
	x->current_frame = fframe;
	x->frame_increment = fincr;
	x->fpos = fpos;
	x->last_fpos = last_fpos;
	x->frames_read = frames_read;
}

void resident_perform64(t_resident *x, t_object *dsp64, double **ins,
                          long numins, double **outs,long numouts, long vectorsize,
                          long flags, void *userparam)
{
	int i, j;
	
	////////////////////////////////////////////// 
	double *MSPInputVector = ins[0];
	double *increment = ins[1];
	double *position = ins[2];
	double *MSPOutputVector = outs[0];
	double *vec_sync = outs[1];		
	t_fftease *fft = x->fft;
	int MSPVectorSize = fft->MSPVectorSize;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	double *internalInputVector = fft->internalInputVector;
	double *internalOutputVector = fft->internalOutputVector;
	
	int D = fft->D;
	int Nw = fft->Nw;
	double *input = fft->input;
	double *output = fft->output;
	float mult = fft->mult;
	short *connected = x->connected;
	
	if( fft->obank_flag )
		mult *= FFTEASE_OSCBANK_SCALAR;

	
	/* quit before doing anything unless we're good to go */

	if( x->mute ) {
		for(i=0; i < vectorsize; i++){ MSPOutputVector[i] = 0.0; }
        for(i=0; i < vectorsize; i++){ vec_sync[i] = 0.0; }
		return;
	}



	if (connected[1]) {
		x->frame_increment = *increment; 
	} 
	
	if (connected[2]) {
		x->fpos = *position; 
	}
	for ( i = 0; i < MSPVectorSize; i++ ){
		vec_sync[i] = x->sync;
	}
	if(x->framecount > 0 && x->read_me ){
		x->sync = (double)x->frames_read/(double)x->framecount;
	}
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_resident(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(MSPInputVector + (D*i), input + (Nw-D), D * sizeof(t_double));
            
			do_resident(x);
			
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
            
			do_resident(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}	
	
void resident_float(t_resident *x, double f) // Look at floats at inlets
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
void resident_acquire_sample ( t_resident *x )
{
    t_buffer_obj *dbuf;
    if(!x->resbuf_ref){
        x->resbuf_ref = buffer_ref_new((t_object *)x, x->buffername_attr);
    } else {
        buffer_ref_set(x->resbuf_ref, x->buffername_attr);
    }
    dbuf = buffer_ref_getobject(x->resbuf_ref);
    // bad buffer
    if(dbuf == NULL){
        post("%s: nonexistent buffer",OBJECT_NAME);
        return;
    }
    x->b_frames = buffer_getframecount(dbuf);
	x->read_me = 1;
	x->frames_read = 0;
    x->buffer_frame_count = (int)((double) (x->b_frames) / (double)(x->fft->N + 2));
	// post("storing %d FFT frames", x->buffer_frame_count);
	// post("%s: beginning spectral data acquisition",OBJECT_NAME);
	return;
	
}

void resident_mute(t_resident *x, long toggle)
{
	x->mute = (short)toggle;	
}

void resident_playthrough(t_resident *x, t_floatarg toggle)
{
	x->playthrough = (short)toggle;	
}


void resident_transpose(t_resident *x, t_floatarg tf)
{
	x->fft->P = (float) tf;
}

void resident_synthresh(t_resident *x, t_floatarg thresh)
{
	x->fft->synt = (float) thresh;
}

void resident_oscbank(t_resident *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}


void resident_winfac(t_resident *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	resident_init(x);
}

void resident_overlap(t_resident *x, t_floatarg f)
{
	x->fft->overlap = (int) f;
	resident_init(x);
}

t_max_err get_fftsize(t_resident *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_resident *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
		resident_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_resident *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_resident *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		resident_init(x);
	}
	return MAX_ERR_NONE;
}

void resident_dsp64(t_resident *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    int i;
    t_fftease *fft = x->fft;
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
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
	}
    if(reset_required){
        resident_init(x);
    }
//    resident_attach_buffer(x);
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,resident_perform64,0,NULL);
}


