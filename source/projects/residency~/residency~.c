#include "fftease.h"

static t_class *residency_class;


#define OBJECT_NAME "residency~"

typedef struct _residency
{
	t_pxobject x_obj;
	t_fftease *fft;
	double **loveboat;
	double current_frame;
	long framecount;
    long last_framecount;
	//
	double frame_increment;
	double fpos;
	double last_fpos;
	double tadv;
	short acquire_stop; // flag to stop recording immediately
	float force_pos; // force to this position on receiving message
	int read_me;
	int frames_read;
	short mute;
    //	short virgin;
	short playthrough;
	short in2_connected;
	short in3_connected;
	double duration;
	short lock;
	short verbose;
	short override;
	double *input_vec;
	double sync;
	short failed_init; // flag to check if init failed due to bad data from Max
	long fftsize_attr;
	long overlap_attr;
    double size_attr;
    long interpolation_attr;
} t_residency;

void *residency_new(t_symbol *s, int argc, t_atom *argv);
void residency_assist(t_residency *x, void *b, long m, long a, char *s);
void residency_bangname(t_residency *x) ;
void residency_fftinfo(t_residency *x) ;
void residency_playthrough( t_residency *x, t_floatarg tog) ;
void residency_float(t_residency *x, double f) ;
void residency_mute(t_residency *x, t_floatarg tog);
void residency_free(t_residency *x);
void residency_init(t_residency *x);
void residency_size(t_residency *x, t_floatarg newsize);
void residency_winfac(t_residency *x, t_floatarg factor);
//void residency_overlap(t_residency *x, t_floatarg o);
void residency_verbose(t_residency *x, t_floatarg t);
void residency_force_position(t_residency *x, t_floatarg position);
void residency_acquire_sample(t_residency *x);
void residency_meminfo( t_residency *x );
void residency_acquire_stop(t_residency *x);
//void residency_fftsize(t_residency *x, t_floatarg f);
void residency_transpose(t_residency *x, t_floatarg tf);
void residency_synthresh(t_residency *x, t_floatarg thresh);
void residency_oscbank(t_residency *x, t_floatarg flag);
void do_residency(t_residency *x);
t_max_err set_fftsize(t_residency *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_residency *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_residency *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_residency *x, void *attr, long *ac, t_atom **av);
t_max_err set_size(t_residency *x, void *attr, long ac, t_atom *av);
t_max_err get_size(t_residency *x, void *attr, long *ac, t_atom **av);
void residency_dsp64(t_residency *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void residency_perform64(t_residency *x, t_object *dsp64, double **ins,
                         long numins, double **outs,long numouts, long vectorsize,
                         long flags, void *userparam);
int C74_EXPORT main(void)
{
	t_class *c;
	c = class_new("fftz.residency~", (method)residency_new, (method)residency_free, sizeof(t_residency),0,A_GIMME,0);
	
	class_addmethod(c,(method)residency_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c,(method)residency_assist,"assist",A_CANT,0);
	class_addmethod(c,(method)residency_acquire_sample, "acquire_sample",  0);
	class_addmethod(c,(method)residency_acquire_stop, "acquire_stop",  0);
	class_addmethod(c,(method)residency_mute, "mute", A_FLOAT, 0);
	class_addmethod(c,(method)residency_fftinfo, "fftinfo",  0);
	class_addmethod(c,(method)residency_meminfo, "meminfo",  0);
	class_addmethod(c,(method)residency_playthrough, "playthrough", A_DEFFLOAT, 0);
	class_addmethod(c,(method)residency_force_position, "force_position", A_FLOAT, 0);
	class_addmethod(c,(method)residency_winfac, "winfac", A_DEFFLOAT, 0);
	class_addmethod(c,(method)residency_verbose, "verbose", A_DEFFLOAT, 0);
	class_addmethod(c,(method)residency_oscbank,"oscbank",A_FLOAT,0);
	class_addmethod(c,(method)residency_transpose,"transpose",A_FLOAT,0);
	class_addmethod(c,(method)residency_synthresh,"synthresh",A_FLOAT,0);
	class_addmethod(c,(method)residency_float, "float",  A_FLOAT, 0);
	class_addmethod(c,(method)residency_acquire_sample, "bang", 0);

	CLASS_ATTR_DOUBLE(c, "size", 0, t_residency, size_attr);
	CLASS_ATTR_ACCESSORS(c, "size", (method)get_size, (method)set_size);
	CLASS_ATTR_LABEL(c, "size", 0, "Sample Size");
    
	CLASS_ATTR_FLOAT(c, "fftsize", 0, t_residency, fftsize_attr);
	CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
	CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");
	
	CLASS_ATTR_FLOAT(c, "overlap", 0, t_residency, overlap_attr);
	CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
	CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");

    CLASS_ATTR_LONG(c, "interpolation", 0, t_residency, interpolation_attr);
    CLASS_ATTR_ENUMINDEX(c, "interpolation", 0, "OFF ON")
    CLASS_ATTR_LABEL(c, "interpolation", 0, "Interpolation");
    
	CLASS_ATTR_ORDER(c, "size",    0, "1");
	CLASS_ATTR_ORDER(c, "fftsize",    0, "2");
	CLASS_ATTR_ORDER(c, "overlap",    0, "3");
    CLASS_ATTR_ORDER(c, "interpolation",    0, "4");
    
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	residency_class = c;
	
	post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
	return 0;
}
void residency_force_position(t_residency *x, t_floatarg position)
{
	if( position >= 0.0 && position < 1.0 ){
		x->force_pos = position;
	}
}

void residency_meminfo( t_residency *x )
{
    t_fftease *fft = x->fft;
    post("%d frames in buffer", x->framecount);
    post("frame_duration: %f, actual time in buffer: %f", x->tadv, (float)(x->framecount) * x->tadv);
	post("main storage chunk: %.2f MB", (x->framecount * (fft->N + 2) * sizeof(double)) / 1000000.0 );
}
/*
 void residency_overlap(t_residency *x, t_floatarg f)
 {
 //	x->fft->overlap = (int) f;
 //	residency_init(x);
 }
 */

void residency_winfac(t_residency *x, t_floatarg f)
{
	x->fft->winfac = (int) f;
	residency_init(x);
}
/*
 void residency_fftsize(t_residency *x, t_floatarg f)
 {
 x->fft->N = (int) f;
 residency_init(x);
 }
 */
void residency_transpose(t_residency *x, t_floatarg tf)
{
	x->fft->P = tf;
}

void residency_synthresh(t_residency *x, t_floatarg thresh)
{
	x->fft->synt = thresh;
}

void residency_oscbank(t_residency *x, t_floatarg flag)
{
	x->fft->obank_flag = (short) flag;
}

void residency_fftinfo(t_residency *x)
{
	fftease_fftinfo(x->fft, OBJECT_NAME);
}

void residency_verbose(t_residency *x, t_floatarg t)
{
	x->verbose = t;
}

void residency_size(t_residency *x, t_floatarg newsize)
{
	//protect with DACs off?
	
	if(newsize > 0.0){//could be horrendous size, but that's the user's problem
		x->duration = newsize/1000.0;
		residency_init(x);
	}
}

void residency_playthrough (t_residency *x, t_floatarg tog)
{
	x->playthrough = tog;
}

void residency_acquire_stop(t_residency *x)
{
	x->acquire_stop = 1;
	x->read_me = 0;
}

void residency_free(t_residency *x){
	int i ;
	dsp_free((t_pxobject *)x);
	fftease_free(x->fft);
    sysmem_freeptr(x->fft);
	for(i = 0; i < x->framecount; i++){
		sysmem_freeptr(x->loveboat[i]) ;
	}
	sysmem_freeptr(x->loveboat);
}

void *residency_new(t_symbol *s, int argc, t_atom *argv)
{
    
    t_fftease *fft;
	t_residency *x = (t_residency *)object_alloc(residency_class);
	dsp_setup((t_pxobject *)x,3);
	outlet_new((t_pxobject *)x, "signal");
	outlet_new((t_pxobject *)x, "signal");
	x->x_obj.z_misc |= Z_NO_INPLACE;

	x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
	fft = x->fft;

	x->fft->N = FFTEASE_DEFAULT_FFTSIZE;
	x->fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	x->fft->winfac = FFTEASE_DEFAULT_WINFAC;
    x->last_framecount = x->framecount = 0;
	
	attr_args_process(x, argc, argv);
    x->fft->initialized = 0;
	residency_init(x);
	return x;
}

void residency_init(t_residency *x)
{
	int i;
	t_fftease *fft = x->fft;
    if(!fft->R){
       // post("zero sample rate in init routine");
        return;
    }
   //  residency_fftinfo(x);
   // post("initialization status: %d", fft->initialized);
	if(fft->initialized == -1){
       // post("initialization blocked");
		return;
	}
    
    fftease_init(x->fft);
	x->x_obj.z_disabled = 1;
	
	// int last_framecount = x->framecount;
	x->tadv = (double)fft->D/(double)fft->R;
	if( x->duration <= 0 ){
		x->duration = 1.0;
	}
    if(!x->tadv){
      //  post("zero tadv in init routine");
        return;
    }
	x->framecount =  x->duration / x->tadv;
   // post("framecount %d last framecount %d",x->framecount, x->last_framecount);
	x->read_me = 0;
	x->acquire_stop = 0;
	if(x->verbose){
		post("%s: will allocate %d frames",OBJECT_NAME, x->framecount);
	}
	// could probably improve memory management here
    if(x->framecount <= 0){
      //  post("bad framecount:%s",x->framecount);
        return;
    }
	if(fft->initialized == 0){
        // x->virgin = 1;
        x->force_pos = -1.0;
        x->current_frame = 0;
        x->fpos = x->last_fpos = 0;
        
		x->sync = 0;
		x->mute = 0;
		x->in2_connected = 0;
		x->in3_connected = 0;
		x->playthrough = 0;
		x->frame_increment = 0.0; // frozen by default
		x->verbose = 0;
		//post("setting memory FIRST TIME ONLY");
       // post("frame count here is: %d", x->framecount);
       // post("N here is %d", fft->N);
		//x->loveboat = (double **) sysmem_newhandleclear(x->framecount * sizeof(double *));
        x->loveboat = (double **) sysmem_newptrclear(x->framecount * sizeof(double *));
		for(i=0;i < x->framecount; i++){
			x->loveboat[i] = (double *) sysmem_newptrclear((fft->N + 2) * sizeof(double));
			if(x->loveboat[i] == NULL){
				error("%s: memory error",OBJECT_NAME);
				return;
			}
		}
	}
    else if((x->framecount == x->last_framecount) && (fft->initialized != 0)){
        //post("our frame count here is: %d", x->framecount);
        //post("no change in framecount size, so no memory realloc");
        x->x_obj.z_disabled = 0;
        return;
    }
    else if(fft->initialized == 1) {
        //post("setting memory after already initialized");
        //post("frame count: %d last frame count: %d", x->framecount, x->last_framecount);
        //post("N here is %d", fft->N);
        /*
         x->loveboat = (double **) sysmem_newhandleclear(x->framecount * sizeof(double *));
         */
        
        
        if(x->framecount != x->last_framecount) {
            // free individual oldies
            for(i = 0; i < x->last_framecount; i++){
                sysmem_freeptr(x->loveboat[i]);
            }
            x->loveboat = (double**)sysmem_resizeptrclear(x->loveboat, x->framecount * sizeof(double *));
            
            for(i=0;i < x->framecount; i++){
                x->loveboat[i] = (double *) sysmem_newptrclear((fft->N + 2) * sizeof(double));
                if(x->loveboat[i] == NULL){
                    error("%s: memory error",OBJECT_NAME);
                    return;
                }
            }
        } else {
            //post("framecount has not changed, so not resizing sample memory");
        }
        
	}
	
	if(! fftease_msp_sanity_check(fft, OBJECT_NAME)){
		// return 0;
		post("residency~ failed sanity test in Init");
		x->failed_init = 1;
	} else {
		// post("residency~ initialized okay");
		x->failed_init = 0;
	}
	
	if (fft->D <= 0.0 || fft->R <= 0.0){
		error("%s: bad decimation size or bad sampling rate - cannot proceed",OBJECT_NAME);
		post("D: %d R: %d",fft->D, fft->R);
		return;
	}
    x->last_framecount = x->framecount;
    x->x_obj.z_disabled = 0;
}




void do_residency(t_residency *x)
{
	int i;
	double fframe = x->current_frame ;
	double last_fpos = x->last_fpos ;
	int framecount = x->framecount;
	double fincr = x->frame_increment;
	double fpos = x->fpos;
	double force_pos = x->force_pos;
    double frak;
    long index1, index2;
    
	t_fftease *fft = x->fft;
    
	if(x->acquire_stop){
		x->acquire_stop = 0;
		fpos = (double) x->frames_read / (double) framecount;
		last_fpos = fpos;
		fframe = x->frames_read;
		if(x->verbose){
			post("residency: data acquisition stopped");
		}
	}
	else if(x->read_me) { // state for sampling to buffer
		if(x->frames_read >= framecount){ // termination condition
			x->read_me = 0;
			if(x->verbose){
				post("residency: data acquisition completed");
			}
		}
		else { // convert and store in one frame
			fold(fft);
			rdft(fft,1);
			convert(fft);
			for(i= 0; i < fft->N + 2; i++){
				x->loveboat[x->frames_read][i] = fft->channel[i];
			}
			++(x->frames_read);
            
		}
	}
	else { // a sample is now in the buffer
		if(fpos < 0)
			fpos = 0;
		if(fpos > 1)
			fpos = 1;
		
		if(force_pos >= 0.0 && force_pos < 1.0){
			//	post("forcing frame to %f", force_pos);
			fframe =  force_pos * (float) framecount;
			last_fpos = fpos = force_pos;
			x->force_pos = -1.0;
		}
		else if(fpos != last_fpos){
			fframe =  fpos * (float) framecount;
			last_fpos = fpos;
		}
		fframe += fincr;
        
		while(fframe >= framecount) {
			fframe -= framecount;
		}
		while( fframe < 0. ) {
			fframe += framecount;
		}
		if(x->framecount > 0) {
			x->sync = fframe/(float)x->framecount;
		}
        // branch to interpolation here:
        // need memory for a composite frame
        // special cases: fframe >= (framecount - 1)
        // or fframe == 0
        if(x->interpolation_attr == 1){
            index1 = floor(fframe);
            index2 = (index1+1) % x->framecount;
            frak = fframe - (double)index1;
            for(i= 0; i < fft->N + 2; i++){
                fft->channel[i] =
                x->loveboat[index1][i] + frak * (x->loveboat[index2][i] - x->loveboat[index1][i]);
            }
        }
        /*
        else if(x->interpolation_attr == 2) {
            index1 = floor(fframe);
            index2 = (index1+1) % x->framecount;
            frak = fframe - (double)index1;
            frak *= PI / 2;
            mult1 = sin(frak);
            mult2 = cos(frak);
            for(i= 0; i < fft->N + 2; i+=2){
                fft->channel[i] =
                    (mult1 * x->loveboat[index1][i]) + (mult2 * x->loveboat[index2][i]);
                fft->channel[i+1] = x->loveboat[index1][i+1];
            }
        }
        else if(x->interpolation_attr == 1){
            index1 = floor(fframe);
            index2 = (index1+1) % x->framecount;
            frak = fframe - (double)index1;
            for(i= 0; i < fft->N + 2; i+=2){
                fft->channel[i] =
                x->loveboat[index1][i] + frak * (x->loveboat[index2][i] - x->loveboat[index1][i]);
                fft->channel[i+1] = x->loveboat[index1][i+1];
            }
        }
         */
        else {
            for(i= 0; i < fft->N + 2; i++){
                fft->channel[i]= x->loveboat[(int)fframe][i];
            }
        }
		if(fft->obank_flag){
			oscbank(fft);
		} else {
			unconvert(fft);
			rdft(fft,FFT_INVERSE);
			overlapadd(fft);
		}
	}
	/* restore state variables */
	
	x->current_frame = fframe;
	x->frame_increment = fincr;
	x->fpos = fpos;
	x->last_fpos = last_fpos;
}

void residency_perform64(t_residency *x, t_object *dsp64, double **ins,
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
	double mult = fft->mult;
	
	// NB increment and position are not set correctly here - take note of connections!!
	// also rewrite do_residency function to use internal values
    
	if (x->in2_connected) {
		x->frame_increment = *increment;
	}
	if (x->in3_connected) {
		x->fpos = *position;
	}
	if( fft->obank_flag )
		mult *= FFTEASE_OSCBANK_SCALAR;
	
	if(x->mute || x->x_obj.z_disabled){
		for(i=0; i < vectorsize; i++){ MSPOutputVector[i] = 0.0; }
        for(i=0; i < vectorsize; i++){ vec_sync[i] = 0.0; }
		return;
	}
	
	if(x->acquire_stop){
		// will reset flag inside do_residency
        for(i=0; i < vectorsize; i++){ output[i] = 0.0; }
		x->read_me = 0;
	}
	for ( i = 0; i < MSPVectorSize; i++ ){
		vec_sync[i] = x->sync;
	}
	if(x->framecount > 0 && x->read_me )
		x->sync = (double)x->frames_read/(double)x->framecount;
	
	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
		do_residency(x);
        
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(MSPInputVector + (D*i), input + (Nw-D), D * sizeof(t_double));
            
			do_residency(x);
			
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
            
			do_residency(x);
			
			for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
}

void residency_float(t_residency *x, double f) // Look at floats at inlets
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


void residency_acquire_sample(t_residency *x)
{
	x->read_me = 1;
	x->frames_read = 0;
	if(x->verbose)
		post("beginning spectral data acquisition");
	return;
}

void residency_mute(t_residency *x, t_floatarg tog)
{
	x->mute = (short) tog;
}

t_max_err get_size(t_residency *x, void *attr, long *ac, t_atom **av)
{
	if (ac && av) {
		char alloc;
		
		if (atom_alloc(ac, av, &alloc)) {
			return MAX_ERR_GENERIC;
		}
        x->size_attr =x->duration * 1000.0;
		atom_setfloat(*av, x->size_attr);
	}
	return MAX_ERR_NONE;
}


t_max_err set_size(t_residency *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->duration = (double)val/1000.0;
//        post("duration set to %f", x->duration);
		residency_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_fftsize(t_residency *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_residency *x, void *attr, long ac, t_atom *av)
{
	
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->N = (int) val;
        x->fft->N2 = x->fft->N * x->fft->winfac;
		residency_init(x);
	}
	return MAX_ERR_NONE;
}

t_max_err get_overlap(t_residency *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_residency *x, void *attr, long ac, t_atom *av)
{
	if (ac && av) {
		long val = atom_getlong(av);
		x->fft->overlap = (int) val;
		residency_init(x);
	}
	return MAX_ERR_NONE;
}


void residency_dsp64(t_residency *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    t_fftease *fft = x->fft;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
    //    x->fft->initialized = 0;
	if(!samplerate)
        return;
	x->in2_connected = count[1];
	x->in3_connected = count[2];
	
	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
	}
    if(reset_required){
        residency_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,residency_perform64,0,NULL);
}

void residency_assist(t_residency *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal/bang) Input, Sampling Trigger"); break;
			case 1: sprintf(dst,"(signal/float) Frame Increment");break;
			case 2:sprintf(dst,"(signal/float) Frame Position [0-1]");break;
				
		}
	} else if (msg==2) {
		switch(arg){
			case 0: sprintf(dst,"(signal) Output"); break;
			case 1: sprintf(dst,"(signal) Frame Sync"); break;
		}
	}
}
