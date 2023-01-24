/* FFTease for MaxMSP */

#include "fftease.h"

static t_class *loopsea_class;

#define OBJECT_NAME "loopsea~"


typedef struct _loopsea
{
    t_pxobject x_obj; //  t_object in Pure Data version
    t_double x_f;
    t_fftease *fft;
    t_double *frame_incr;
    t_double *store_incr;
    t_double *frame_phase;
    t_double *tbank; // bank of transposition factors
    t_double *interval_bank; // user provided intervals
    int interval_count; // how many intervals were provided
    t_double frameloc;
    t_double **loveboat;
    t_double current_frame;
    long *start_frames;
    long *end_frames;
    long framecount;
    long last_framecount;
    t_double frame_increment ;
    t_double fpos;
    t_double last_fpos;
    t_double tadv;
    int restart_loops_flag; // indicator to reset all loops from their start frames
    int tbank_flag; // indicator that we are using bin-level transposition factors
    int read_me;
    long frames_read;
    short mute;
    short playthrough;
    short lock;
    t_double duration;
    t_double sync;
    long interpolation_attr;
    void *listo; // list outlet
    t_atom *data; // storage for sending loop data to the outlet
    long fftsize_attr;
    long overlap_attr;
} t_loopsea;


static void *loopsea_new(t_symbol *msg, short argc, t_atom *argv);
static void loopsea_acquire_sample (t_loopsea *x) ;
static void loopsea_mute(t_loopsea *x, t_floatarg tog);
static void loopsea_setspeed( t_loopsea *x,  t_floatarg speed );
static void loopsea_free( t_loopsea *x );
static void loopsea_init(t_loopsea *x);
static void loopsea_randspeed(t_loopsea *x, t_symbol *msg, short argc, t_atom *argv);
static void loopsea_playthrough(t_loopsea *x, t_floatarg state);
static void loopsea_transpose(t_loopsea *x, t_floatarg tf);
static void loopsea_synthresh(t_loopsea *x, t_floatarg thresh);
static void loopsea_oscbank(t_loopsea *x, t_floatarg flag);
static void loopsea_setloops(t_loopsea *x, t_floatarg minloop, t_floatarg maxloop);
static void loopsea_restart_loops(t_loopsea *x);
static void loopsea_randtransp(t_loopsea *x, t_symbol *msg, short argc, t_atom *argv);
void loopsea_transp_choose(t_loopsea *x, t_symbol *msg, short argc, t_atom *argv);
void loopsea_flat_transpose(t_loopsea *x);
void loopsea_readloops(t_loopsea *x, t_symbol *msg, short argc, t_atom *argv);
void loopsea_printloops(t_loopsea *x);
void loopsea_assist(t_loopsea *x, void *b, long m, long a, char *s);
void loopsea_dsp64(t_loopsea *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void loopsea_fftinfo(t_loopsea *x);
t_max_err set_fftsize(t_loopsea *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_loopsea *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_loopsea *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_loopsea *x, void *attr, long *ac, t_atom **av);
void loopsea_dsp64(t_loopsea *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);


int C74_EXPORT main(void)
{
    
    t_class *c;
    c = class_new("fftz.loopsea~", (method)loopsea_new, (method)loopsea_free, sizeof(t_loopsea),0,A_GIMME,0);
    class_addmethod(c,(method)loopsea_dsp64, "dsp64", A_CANT, 0);
    class_addmethod(c,(method)loopsea_assist,"assist",A_CANT,0);
    class_addmethod(c,(method)loopsea_oscbank,"oscbank",A_FLOAT,0);
    class_addmethod(c,(method)loopsea_transpose,"transpose",A_FLOAT,0);
    class_addmethod(c,(method)loopsea_synthresh,"synthresh",A_FLOAT,0);
    class_addmethod(c,(method)loopsea_acquire_sample,"acquire_sample", 0);
    class_addmethod(c,(method)loopsea_randspeed, "randspeed", A_GIMME, 0);
    class_addmethod(c,(method)loopsea_randtransp, "randtransp", A_GIMME, 0);
    class_addmethod(c,(method)loopsea_transp_choose, "transp_choose", A_GIMME, 0);
    class_addmethod(c,(method)loopsea_setspeed, "setspeed",  A_FLOAT, 0);
//    class_addmethod(c,(method)loopsea_playthrough,"playthrough",  A_DEFFLOAT, 0);
    class_addmethod(c,(method)loopsea_setloops,"setloops",  A_FLOAT, A_FLOAT, 0);
    class_addmethod(c,(method)loopsea_restart_loops,"restart_loops", 0);
    class_addmethod(c,(method)loopsea_flat_transpose,"flat_transpose",0);
    class_addmethod(c,(method)loopsea_printloops,"printloops",0);
    class_addmethod(c,(method)loopsea_readloops,"readloops",A_GIMME,0);
    class_addmethod(c,(method)loopsea_mute,"mute",A_FLOAT,0);
    class_addmethod(c,(method)loopsea_fftinfo,"fftinfo",0);
    CLASS_ATTR_LONG(c, "fftsize", 0, t_loopsea, fftsize_attr);
    CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
    CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");
    CLASS_ATTR_LONG(c, "overlap", 0, t_loopsea, overlap_attr);
    CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
    CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");
    CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
    CLASS_ATTR_ORDER(c, "overlap",    0, "2");
    
    class_dspinit(c);
    class_register(CLASS_BOX, c);
    loopsea_class = c;
    post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
    return 0;
}

void loopsea_transpose(t_loopsea *x, t_floatarg tf)
{
    x->fft->P = tf;
}

void loopsea_synthresh(t_loopsea *x, t_floatarg thresh)
{
    x->fft->synt = thresh;
}

void loopsea_oscbank(t_loopsea *x, t_floatarg flag)
{
    x->fft->obank_flag = (short) flag;
}

void loopsea_store_incr(t_loopsea *x)
{
    t_fftease *fft = x->fft;

    int i;
    t_double *store_incr = x->store_incr;
    t_double *frame_incr = x->frame_incr;

    for(i = 0; i < fft->N2; i++){
        store_incr[i] = frame_incr[i];
    }
}

void loopsea_free(t_loopsea *x){
    int i ;

    dsp_free((t_pxobject *) x);
    if(x->fft->initialized == 1){
        for(i = 0; i < x->framecount; i++){
            sysmem_freeptr(x->loveboat[i]) ;
        }
        sysmem_freeptr(x->loveboat);
        sysmem_freeptr(x->frame_phase);
        sysmem_freeptr(x->frame_incr);
        sysmem_freeptr(x->store_incr);
        sysmem_freeptr(x->start_frames);
        sysmem_freeptr(x->end_frames);
        sysmem_freeptr(x->tbank);
        sysmem_freeptr(x->interval_bank);
        sysmem_freeptr(x->data);
        fftease_free(x->fft);
        sysmem_freeptr(x->fft);
    }
    
}

void loopsea_setspeed( t_loopsea *x,  t_floatarg speed )
{
    t_fftease *fft = x->fft;
    if(! x->fft->init_status){
        return;
    }
    int i;
    for( i = 0; i < fft->N2; i++ ){
        x->frame_incr[i] = speed;
    }
}

void loopsea_setloops(t_loopsea *x, t_floatarg minloop, t_floatarg maxloop)
{
    long minframes, maxframes, loopframes, loopstart, loopend;
    int i;
    long framecount = x->framecount;
    long *start_frames = x->start_frames;
    long *end_frames = x->end_frames;
    t_fftease *fft = x->fft;
    t_double *frame_phase = x->frame_phase;
    
    // convert ms. to frames for min and max loop size

    minframes = (minloop/1000.0) / x->tadv;
    maxframes = (maxloop/1000.0) / x->tadv;

    if( (minframes < 0) || (minframes > framecount) ){
        minframes = 0;
    }
    if(maxframes > framecount){
        maxframes = framecount;
    }
    for( i = 0; i < fft->N2; i++ ){
        loopframes = (long) floor( fftease_randf((t_double)minframes, (t_double)maxframes) );
        loopstart = (long) floor( fftease_randf(0.0, (t_double)(framecount - loopframes)) );
        loopend = loopstart + loopframes;
        start_frames[i] = loopstart;
        end_frames[i] = loopend;
        frame_phase[i] = (t_double) loopstart;
    }
}


void loopsea_restart_loops(t_loopsea *x)
{
    x->restart_loops_flag = 1;
}

void loopsea_init(t_loopsea *x)
{
    int i;
    short initialized = x->fft->initialized;
    t_fftease  *fft = x->fft;
    fftease_init(fft);
    if(!fftease_msp_sanity_check(fft,OBJECT_NAME)){
        return;
    }

    x->current_frame = x->framecount = 0;
    x->fpos = x->last_fpos = 0;
    x->tadv = (float)fft->D/(float)fft->R;
    if(x->duration < 0.1){
        x->duration = 0.1;
    }
    x->framecount =  x->duration/x->tadv;
    x->read_me = 0;

    // post("running init function. Framecount is %d", x->framecount);
    if(! initialized ){
       // post("startup init");
        x->frame_increment = 1.0 ;
        x->mute = 0;
        x->playthrough = 0;
        x->sync = 0;
        x->frames_read = 0;
        x->frame_incr = (t_double *) sysmem_newptrclear(fft->N2 * sizeof(t_double));
        x->store_incr = (t_double *) sysmem_newptrclear(fft->N2 * sizeof(t_double));
        x->frame_phase = (t_double *) sysmem_newptrclear(fft->N2 * sizeof(t_double));
        x->tbank = (t_double *) sysmem_newptrclear(fft->N2 * sizeof(t_double));
        x->start_frames = (long *) sysmem_newptrclear(fft->N2 * sizeof(long));
        x->end_frames = (long *) sysmem_newptrclear(fft->N2 * sizeof(long));
        x->interval_bank = (t_double *) sysmem_newptrclear(128 * sizeof(t_double));
        x->loveboat = (t_double **) sysmem_newptrclear(x->framecount * sizeof(t_double *));
        x->data = (t_atom *) sysmem_newptrclear(((fft->N2 * 3) + 1) * sizeof(t_atom));
        for(i=0; i < x->framecount; i++){
            x->loveboat[i] = (t_double *) sysmem_newptrclear((fft->N+2) * sizeof(t_double));
            if(x->loveboat[i] == NULL){
                error("%s: Insufficient Memory!",OBJECT_NAME);
                return;
            }
        }
    }
    else {
      //  post("recap init");
        x->frame_incr = (t_double *) sysmem_resizeptrclear(x->frame_incr, fft->N2 * sizeof(t_double));
        x->store_incr = (t_double *) sysmem_resizeptrclear(x->store_incr, fft->N2 * sizeof(t_double));
        x->frame_phase = (t_double *) sysmem_resizeptrclear(x->frame_phase, fft->N2 * sizeof(t_double));
        x->tbank = (t_double *) sysmem_resizeptrclear(x->tbank, fft->N2 * sizeof(t_double));
        x->start_frames = (long *) sysmem_resizeptrclear(x->start_frames, fft->N2 * sizeof(long));
        x->end_frames = (long *) sysmem_resizeptrclear(x->end_frames, fft->N2 * sizeof(long));
        
        for(i = 0; i < x->last_framecount; i++){
            sysmem_freeptr(x->loveboat[i]);
        }
        x->loveboat = (t_double **)sysmem_resizeptrclear(x->loveboat, x->framecount * sizeof(t_double*));
        for(i=0; i < x->framecount; i++){
            x->loveboat[i] = (t_double *) sysmem_newptrclear((fft->N+2) * sizeof(t_double));
            if(x->loveboat[i] == NULL){
                error("%s: Insufficient Memory!",OBJECT_NAME);
                return;
            }
        }
    }
    loopsea_setloops(x, 50.0, x->duration * 1000.0);
    x->last_framecount = x->framecount;
    // safety, probably not necessary:
    for(i = 0; i < fft->N2; i++){
        x->tbank[i] = 1.0;
    }
    //post("new init status: %d",x->fft->initialized);
}

void *loopsea_new(t_symbol *msg, short argc, t_atom *argv)
{
// t_fftease *fft;
    t_loopsea *x = (t_loopsea *)object_alloc(loopsea_class);

    x->listo = listout((t_pxobject *)x);
    dsp_setup((t_pxobject *)x,1);
    outlet_new((t_pxobject *)x, "signal");
    outlet_new((t_pxobject *)x, "signal");

    x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
    x->fft->R = sys_getsr();
    x->fft->MSPVectorSize = sys_getblksize();
    // fft = x->fft;
    x->fft->initialized = 0;

    srand(clock()); // needed ?
    x->restart_loops_flag = 0;
    x->fft->N = FFTEASE_DEFAULT_FFTSIZE;
    x->fft->overlap = FFTEASE_DEFAULT_OVERLAP;
    x->fft->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ x->duration = atom_getfloatarg(0, argc, argv) / 1000.0; }
    else {
        post("%s: no duration given, using a default of 5000 ms",OBJECT_NAME);
        x->duration = 5.0;
    }
    attr_args_process(x, argc, argv);
    loopsea_init(x);
    return x;
}

static void do_loopsea(t_loopsea *x)
{
    t_fftease *fft = x->fft;

    int amp, freq, i;
    long minphase, maxphase, thisFramecount;
    int N = fft->N;
    int N2 = fft->N2;
    t_double fframe = x->current_frame ;
    t_double last_fpos = x->last_fpos ;
    int framecount = x->framecount;
    t_double *frame_incr = x->frame_incr;
    t_double *frame_phase = x->frame_phase;
    t_double *channel = fft->channel;
    long *start_frames = x->start_frames;
    long *end_frames = x->end_frames;
    int tbank_flag = x->tbank_flag;
    t_double *tbank = x->tbank;
    t_double frak;
    long iphase1, iphase2;

    //post("readme: %d framecount: %d", x->read_me, x->framecount);
    if(x->read_me && x->framecount > 0){
        //post("loopsea acquire on. Frames read: %d", x->frames_read);
        fold(fft);
        rdft(fft,FFT_FORWARD);
        convert(fft);
        for(i = 0; i < N; i++){
            x->loveboat[x->frames_read][i] = channel[i];
        }
        x->frames_read++;
        if(x->frames_read >= x->framecount){
            x->read_me = 0;
            // post("sample acquisition completed");
        }
        x->sync = (t_double) x->frames_read / (t_double) x->framecount;
    }
    else {
       // post("normal play mode");
        for( i = 0 ; i < N2; i++ ){
            amp = i<<1;
            freq = amp + 1;
            minphase = start_frames[i];
            maxphase = end_frames[i];
            thisFramecount = maxphase - minphase;
            iphase1 = floor( frame_phase[i] );
            frak = frame_phase[i] - iphase1;
            if(iphase1 < minphase){
                iphase1 = minphase;
            }
            if( iphase1 > maxphase){
                iphase1 = maxphase;
            }
            //possible bug if increment is less than 0.0
            iphase2 = (iphase1 + 1) % framecount;
            
            // only interpolate if the fraction is greater than epsilon 0.0001
            if(frak < 0.0001 ){
                channel[amp] = x->loveboat[iphase1][amp];
                channel[freq] = x->loveboat[iphase1][freq];
            } else {
                channel[amp] = x->loveboat[iphase1][amp] + (frak *
                                                            (x->loveboat[iphase2][amp] - x->loveboat[iphase1][amp]));
                channel[freq] = x->loveboat[iphase1][freq] + (frak *
                                                              (x->loveboat[iphase2][freq] - x->loveboat[iphase1][freq]));
            }
            if(tbank_flag){
                channel[freq] *= tbank[i];
            }
            frame_phase[i] += frame_incr[i];
            while( frame_phase[i] > maxphase){
                frame_phase[i] -= thisFramecount;
            }
            while( frame_phase[i] < minphase ){
                frame_phase[i] += thisFramecount;
            }
        }
    }

    if(fft->obank_flag){
        oscbank(fft);
    } else {
        unconvert(fft);
        rdft(fft,FFT_INVERSE);
        overlapadd(fft);
    }

    /* restore state variables */

    x->current_frame = fframe;
    x->last_fpos = last_fpos;
}

void loopsea_perform64(t_loopsea *x, t_object *dsp64, double **ins,
                           long numins, double **outs,long numouts, long vectorsize,
                           long flags, void *userparam)
{
    int i, j;
    //////////////////////////////////////////////
//    t_loopsea *x = (t_loopsea *) (w[1]);
    t_double *MSPInputVector = ins[0];
    t_double *MSPOutputVector = outs[0];
    t_double *sync_vec = outs[1];

    /* dereference structure */

    t_fftease *fft = x->fft;
    int D = fft->D;
    int Nw = fft->Nw;
    t_double *input = fft->input;
    t_double *output = fft->output;
    t_double mult = fft->mult;
    int MSPVectorSize = fft->MSPVectorSize;
    t_double *internalInputVector = fft->internalInputVector;
    t_double *internalOutputVector = fft->internalOutputVector;
    int operationRepeat = fft->operationRepeat;
    int operationCount = fft->operationCount;

    //post("entering Perform64");
    if(x->mute){
        //post("muted");
        for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
        for(i=0; i < MSPVectorSize; i++){ sync_vec[i] = 0.0; }
        return;
    }
    if( fft->obank_flag ){
        mult *= FFTEASE_OSCBANK_SCALAR;
    }
    /*
    if(x->playthrough && x->read_me){
        for (i = 0; i < MSPVectorSize; i++) {
            MSPOutputVector[i] = MSPInputVector[i] * 0.5; // scale down
        }
        for(i=0; i < MSPVectorSize; i++){ sync_vec[i] = 0.0; }
        return;
    }
    */
    if(x->restart_loops_flag == 1){
        for( i = 0; i < x->fft->N2; i++ ){
            x->frame_phase[i] = (t_double) x->start_frames[i];
        }
        x->restart_loops_flag = 0;
    }

    
    if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
        do_loopsea(x);
        
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
            
            do_loopsea(x);
            
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
            
            do_loopsea(x);
            
            for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
        }
        fft->operationCount = operationCount;
    }
    for ( i = 0; i < MSPVectorSize; i++ ){
        sync_vec[i] = x->sync;
    }
    return;
}

void loopsea_acquire_sample(t_loopsea *x)
{
  //  post("initializing acquire sample mode");
    x->read_me = 1;
    x->frames_read = 0;
    // return;
}

void loopsea_mute(t_loopsea *x, t_floatarg tog)
{
    x->mute = tog;
}

void loopsea_playthrough(t_loopsea *x, t_floatarg state)
{
    x->playthrough = state;
}


void loopsea_randphase(t_loopsea *x, t_symbol *msg, short argc, t_atom *argv)
{
    t_fftease *fft = x->fft;

    float minphase, maxphase;
    int i;
    int framecount = x->framecount;

    minphase = atom_getfloatarg(0, argc, argv);
    maxphase = atom_getfloatarg(1, argc, argv);

    //  post("minphase %f maxphase %f",minphase, maxphase);
    if(minphase < 0.0)
        minphase = 0.0;
    if( maxphase > 1.0 )
        maxphase = 1.0;

    for( i = 0; i < fft->N2; i++ ){
        x->frame_phase[i] = (int) (fftease_randf( minphase, maxphase ) * (float) (framecount - 1) ) ;
    }
}

void loopsea_randspeed(t_loopsea *x, t_symbol *msg, short argc, t_atom *argv)
{
    t_fftease *fft = x->fft;

    float minspeed, maxspeed;
    int i;


    minspeed = atom_getfloatarg(0, argc, argv);
    maxspeed = atom_getfloatarg(1, argc, argv);

    for( i = 0; i < fft->N2; i++ ){
        x->frame_incr[i] = fftease_randf(minspeed, maxspeed);
    }
}



void loopsea_printloops(t_loopsea *x)
{
    t_fftease *fft = x->fft;
    t_atom *data = x->data;
    t_symbol *readloops = gensym("readloops");
    long *start_frames = x->start_frames;
    long *end_frames = x->end_frames;
    t_double *tbank = x->tbank;
    t_double *frame_incr = x->frame_incr;
    int data_index = 0;
    int i;
    atom_setsym(data+data_index, readloops);
    data_index++;
    for(i = 0; i < fft->N2; i++){
        atom_setlong(data+data_index, start_frames[i]);
        data_index++;
        atom_setlong(data+data_index, (end_frames[i]));
        data_index++;
        atom_setfloat(data+data_index, tbank[i]);
        data_index++;
        atom_setfloat(data+data_index, frame_incr[i]);
        data_index++;
    }
    outlet_list(x->listo, 0, data_index, data);
}

void loopsea_readloops(t_loopsea *x, t_symbol *msg, short argc, t_atom *argv)
{
    int i,j;
    t_fftease *fft = x->fft;
    long *start_frames = x->start_frames;
    long *end_frames = x->end_frames;
    t_double *tbank = x->tbank;
    t_double *frame_incr = x->frame_incr;
    if(argc != fft->N2 * 4){
        post("loopsea~: Houston we have a problem: mismatch between expected size %d, and actual message size %d",fft->N2*3, argc);
        return;
    }
    for(i = 0, j = 0; i < fft->N2; i++, j += 4){
        start_frames[i] = (long) atom_getfloatarg(j,argc,argv);
        end_frames[i] = (long) atom_getfloatarg(j+1,argc,argv);
        tbank[i] = atom_getfloatarg(j+2,argc,argv);
        frame_incr[i] = atom_getfloatarg(j+3,argc,argv);
    }
}

void loopsea_randtransp(t_loopsea *x, t_symbol *msg, short argc, t_atom *argv)
{
    t_fftease *fft = x->fft;

    float minincr, maxincr;
    int i;

    minincr = atom_getfloatarg(0, argc, argv);
    maxincr = atom_getfloatarg(1, argc, argv);
    for( i = 0; i < fft->N2; i++ ){
        x->tbank[i] = fftease_randf(minincr, maxincr);
    }
    x->tbank_flag = 1;
}

void loopsea_flat_transpose(t_loopsea *x)
{
    x->tbank_flag = 0;
}

void loopsea_transp_choose(t_loopsea *x, t_symbol *msg, short argc, t_atom *argv)
{
    t_fftease *fft = x->fft;

//    float minincr, maxincr;
    int i, randex;
    int interval_count = x->interval_count;
    t_double *interval_bank = x->interval_bank;
    if(argc < 2){
        object_error((t_object *)x, "loopsea~: transp_choose must receive at least two interval choices");
        return;
    }
    if(argc > 128){
        object_error((t_object *)x, "loopsea~: transp_choose allows a maximum of 128 intervals");
        return;
    }
    interval_count = argc;
    for(i = 0; i < argc; i++){
        interval_bank[i] = atom_getfloatarg(i, argc, argv);
    }
    for( i = 0; i < fft->N2; i++ ){
        randex = fftease_randi(0, interval_count - 1);
        x->tbank[i] = interval_bank[randex];
    }
    x->tbank_flag = 1;
}

void loopsea_dsp64(t_loopsea *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    int reset_required = 0;
    t_fftease *fft = x->fft;
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
    if(!samplerate)
        return;
    
    if(fft->MSPVectorSize != maxvectorsize){
        //post("resetting FFT buffers");
        fft->MSPVectorSize = maxvectorsize;
        fftease_set_fft_buffers(fft);
    }
    if(fft->R != samplerate ){
        fft->R = samplerate;
    }
    if(reset_required){
        //post("initializing loopsea~ from DSP64");
        loopsea_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)){
        //post("adding loopsea_perform64");
        object_method(dsp64, gensym("dsp_add64"),x,loopsea_perform64,0,NULL);
    } else {
        post("loopsea~: dsp64 failed sanity test");
    }
}

t_max_err get_fftsize(t_loopsea *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_loopsea *x, void *attr, long ac, t_atom *av)
{
    
    if (ac && av) {
        long val = atom_getlong(av);
        x->fft->N = (int) val;
        loopsea_init(x);
    }
    return MAX_ERR_NONE;
}

t_max_err get_overlap(t_loopsea *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_loopsea *x, void *attr, long ac, t_atom *av)
{
    if (ac && av) {
        long val = atom_getlong(av);
        x->fft->overlap = (int) val;
        loopsea_init(x);
    }
    return MAX_ERR_NONE;
}

void loopsea_fftinfo( t_loopsea *x )
{
    fftease_fftinfo( x->fft, OBJECT_NAME );
}

void loopsea_assist (t_loopsea *x, void *b, long msg, long arg, char *dst)
{
    if (msg==1) {
        switch (arg) {
            case 0: sprintf(dst,"(signal) Input"); break;
        }
    } else if (msg==2) {
        switch (arg) {
            case 0: sprintf(dst,"(signal) Output"); break;
            case 1: sprintf(dst,"(signal) Interpolation Sync"); break;
            case 2: sprintf(dst,"(list) Current Loop Parameters"); break;
        }
    }
}
