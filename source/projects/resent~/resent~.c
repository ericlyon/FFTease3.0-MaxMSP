#include "fftease.h"

static t_class *resent_class;

#define OBJECT_NAME "resent~"


typedef struct _resent
{
    t_pxobject x_obj;
    t_fftease *fft;
    double *frame_incr;
    double *store_incr;
    double *frame_phase;
    double frameloc;
    double **loveboat;
    double current_frame;
    long framecount;
    long last_framecount;
    //
    double frame_increment ;
    double fpos;
    double last_fpos;
    double tadv;
    int read_me;
    long frames_read;
    short mute;
    void *m_clock;
    void *m_bang;
    short playthrough;
    short lock;
    double duration;
    short verbose;
    double sync;
    long fftsize_attr;
    long overlap_attr;
    double size_attr;
    long interpolation_attr;
} t_resent;

void *resent_new(t_symbol *msg, short argc, t_atom *argv);
void resent_assist(t_resent *x, void *b, long m, long a, char *s);
void resent_acquire_sample (t_resent *x) ;
void resent_mute(t_resent *x, t_floatarg tog);
void resent_bin(t_resent *x, t_floatarg fbin, t_floatarg speed);
void resent_setphase(t_resent *x, t_floatarg phase);
void resent_addphase(t_resent *x, t_floatarg phase);
void resent_setspeed( t_resent *x,  t_floatarg speed );
void resent_addspeed( t_resent *x,  t_floatarg speed );
void resent_size( t_resent *x,  t_floatarg size_ms );
void resent_free( t_resent *x );
void resent_store_incr( t_resent *x );
void resent_setspeed_and_phase( t_resent *x,  t_floatarg speed, t_floatarg phase );
void resent_tick(t_resent *x);
void resent_fftinfo(t_resent *x);
void resent_init(t_resent *x);
void resent_linephase(t_resent *x, t_symbol *msg, short argc, t_atom *argv);
void resent_linespeed(t_resent *x, t_symbol *msg, short argc, t_atom *argv);
void resent_randphase(t_resent *x, t_symbol *msg, short argc, t_atom *argv);
void resent_randspeed(t_resent *x, t_symbol *msg, short argc, t_atom *argv);
void resent_playthrough(t_resent *x, t_floatarg state);
double fftease_randf(double min, double max);
void resent_winfac(t_resent *x, t_floatarg factor);
void resent_fftinfo(t_resent *x);
void resent_winfac(t_resent *x, t_floatarg f);
void resent_transpose(t_resent *x, t_floatarg tf);
void resent_synthresh(t_resent *x, t_floatarg thresh);
void resent_oscbank(t_resent *x, t_floatarg flag);
t_max_err set_fftsize(t_resent *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_resent *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_resent *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_resent *x, void *attr, long *ac, t_atom **av);
t_max_err set_size(t_resent *x, void *attr, long ac, t_atom *av);
t_max_err get_size(t_resent *x, void *attr, long *ac, t_atom **av);
t_max_err set_interpolation(t_resent *x, void *attr, long ac, t_atom *av);
t_max_err get_interpolation(t_resent *x, void *attr, long *ac, t_atom **av);
void resent_dsp64(t_resent *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void resent_perform64(t_resent *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam);

int C74_EXPORT main(void)
{
    t_class *c;
    c = class_new("fftz.resent~", (method)resent_new, (method)resent_free, sizeof(t_resent),0,A_GIMME,0);
    
    class_addmethod(c,(method)resent_dsp64, "dsp64", A_CANT, 0);
    class_addmethod(c,(method)resent_assist,"assist",A_CANT,0);
    class_addmethod(c,(method)resent_acquire_sample,"acquire_sample", 0);
    class_addmethod(c,(method)resent_acquire_sample,"bang", 0);
    class_addmethod(c,(method)resent_mute, "mute", A_FLOAT, 0);
    class_addmethod(c,(method)resent_linespeed, "linespeed", A_GIMME, 0);
    class_addmethod(c,(method)resent_linephase, "linephase", A_GIMME, 0);
    class_addmethod(c,(method)resent_randspeed, "randspeed", A_GIMME, 0);
    class_addmethod(c,(method)resent_randphase, "randphase", A_GIMME, 0);
    class_addmethod(c,(method)resent_bin, "bin", A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(c,(method)resent_setphase, "setphase",  A_DEFFLOAT, 0);
    class_addmethod(c,(method)resent_addphase, "addphase",  A_DEFFLOAT, 0);
    class_addmethod(c,(method)resent_setspeed, "setspeed",  A_DEFFLOAT, 0);
    class_addmethod(c,(method)resent_addspeed, "addspeed",  A_DEFFLOAT, 0);
    class_addmethod(c,(method)resent_playthrough, "playthrough",  A_DEFFLOAT, 0);
    class_addmethod(c,(method)resent_store_incr, "store_incr",0);
    class_addmethod(c,(method)resent_setspeed_and_phase, "setspeed_and_phase",  A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(c,(method)resent_winfac,"winfac",A_DEFFLOAT,0);
    class_addmethod(c,(method)resent_fftinfo,"fftinfo",0);
    class_addmethod(c,(method)resent_oscbank,"oscbank",A_FLOAT,0);
    class_addmethod(c,(method)resent_transpose,"transpose",A_FLOAT,0);
    class_addmethod(c,(method)resent_synthresh,"synthresh",A_FLOAT,0);
    
    CLASS_ATTR_FLOAT(c, "fftsize", 0, t_resent, fftsize_attr);
    CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
    CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");
    
    CLASS_ATTR_FLOAT(c, "overlap", 0, t_resent, overlap_attr);
    CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
    CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");
    
    CLASS_ATTR_DOUBLE(c, "size", 0, t_resent, size_attr);
    CLASS_ATTR_ACCESSORS(c, "size", (method)get_size, (method)set_size);
    CLASS_ATTR_LABEL(c, "size", 0, "Size");
    
    CLASS_ATTR_LONG(c, "interpolation", 0, t_resent, interpolation_attr);
    CLASS_ATTR_ACCESSORS(c, "interpolation", (method)get_interpolation, (method)set_interpolation);
    CLASS_ATTR_ENUMINDEX(c, "interpolation", 0, "OFF ON");
    CLASS_ATTR_LABEL(c, "interpolation", 0, "Interpolation");
    
    CLASS_ATTR_ORDER(c, "size", 0, "1");
    CLASS_ATTR_ORDER(c, "fftsize", 0, "2");
    CLASS_ATTR_ORDER(c, "overlap", 0, "3");
    CLASS_ATTR_ORDER(c, "Interpolation", 0, "4");
    
    class_dspinit(c);
    class_register(CLASS_BOX, c);
    resent_class = c;
    post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
    return 0;
    
}

void resent_verbose(t_resent *x, t_floatarg t)
{
    x->verbose = (short)t;
}


void resent_fftsize(t_resent *x, t_floatarg f)
{
    x->fft->N = (int) f;
    resent_init(x);
}

void resent_overlap(t_resent *x, t_floatarg f)
{
    x->fft->overlap = (int) f;
    resent_init(x);
}

void resent_winfac(t_resent *x, t_floatarg f)
{
    x->fft->winfac = (int) f;
    resent_init(x);
}

void resent_fftinfo(t_resent *x)
{
    fftease_fftinfo(x->fft, OBJECT_NAME );
}

void resent_transpose(t_resent *x, t_floatarg tf)
{
    x->fft->P = tf;
}

void resent_synthresh(t_resent *x, t_floatarg thresh)
{
    x->fft->synt = thresh;
}

void resent_oscbank(t_resent *x, t_floatarg flag)
{
    x->fft->obank_flag = (short) flag;
}

void resent_store_incr(t_resent *x)
{
    t_fftease *fft = x->fft;
    
    int i;
    double *store_incr = x->store_incr;
    double *frame_incr = x->frame_incr;
    
    for(i = 0; i < fft->N2; i++){
        store_incr[i] = frame_incr[i];
    }
}

void resent_free(t_resent *x){
    int i ;
    dsp_free((t_pxobject *) x);
    
    if(x->fft->initialized){
        for(i = 0; i < x->framecount; i++){
            sysmem_freeptr(x->loveboat[i]) ;
        }
        sysmem_freeptr(x->loveboat);
        sysmem_freeptr(x->frame_phase);
        sysmem_freeptr(x->frame_incr);
        sysmem_freeptr(x->store_incr);
    }
    fftease_free(x->fft);
    sysmem_freeptr(x->fft);
}

void resent_bin(t_resent *x, t_floatarg fbin, t_floatarg speed)
{
    t_fftease *fft = x->fft;
    
    int bin_num = (int) fbin;
    
    if(bin_num >= 0 && bin_num < fft->N2){
        x->frame_incr[bin_num] = speed ;
    } else {
        post("resent~: bin %d is out of range", bin_num);
    }
}

void resent_setphase( t_resent *x,  t_floatarg phase)
{
    t_fftease *fft = x->fft;
    
    double scaled_phase;
    int i;
    
    if( phase < 0. )
        phase = 0. ;
    if( phase > 1. )
        phase = 1.;
    scaled_phase = phase * (float) x->framecount ;
    for( i = 0; i < fft->N2; i++ ){
        x->frame_phase[i] = scaled_phase ;
    }
    
}

void resent_addphase( t_resent *x,  t_floatarg phase )
{
    t_fftease *fft = x->fft;
    
    double scaled_phase ;
    double *frame_phase = x->frame_phase;
    int framecount =  x->framecount;
    int i;
    
    if( phase < 0. )
        phase = 0. ;
    if( phase > 1. )
        phase = 1.;
    scaled_phase = phase * (float) framecount ;
    for( i = 0; i < fft->N2; i++ ){
        frame_phase[i] += scaled_phase ;
        while( frame_phase[i] < 0 )
            frame_phase[i] += framecount;
        while( frame_phase[i] > framecount - 1 )
            frame_phase[i] -= framecount ;
    }
}

void resent_setspeed( t_resent *x,  t_floatarg speed )
{
    t_fftease *fft = x->fft;
    if(! x->fft->init_status)
        return;
    
    int i;
    
    for( i = 0; i < fft->N2; i++ ){
        
        x->frame_incr[i] = speed ;
    }
    // post("speed reset to %f",speed);
    
}

void resent_addspeed( t_resent *x,  t_floatarg speed )
{
    t_fftease *fft = x->fft;
    
    int i;
    double *store_incr = x->store_incr;
    double *frame_incr = x->frame_incr;
    
    for( i = 0; i < fft->N2; i++ ){
        frame_incr[i] = store_incr[i] + speed ;
    }
    
    
}
void resent_setspeed_and_phase( t_resent *x,  t_floatarg speed, t_floatarg phase )
{
    t_fftease *fft = x->fft;
    
    double scaled_phase;
    int i;
    if( phase < 0. )
        phase = 0. ;
    if( phase > 1. )
        phase = 1.;
    
    scaled_phase = phase * (double) x->framecount ;
    for( i = 0; i < fft->N2; i++ ){
        x->frame_phase[i] = scaled_phase ;
        x->frame_incr[i] = speed ;
    }
    //  post("ssap: speed reset to %f, phase reset to %f",speed,phase);
    
}

void resent_assist (t_resent *x, void *b, long msg, long arg, char *dst)
{
    if (msg==1) {
        switch (arg) {
            case 0:
                sprintf(dst,"(signal/bang) Input, Sample Trigger");
                break;
        }
    } else if (msg==2) {
        switch( arg){
            case 0:
                sprintf(dst,"(signal) Output ");
                break;
            case 1:
                sprintf(dst,"(signal) Recording Sync");
                break;
        }
        
    }
}

void resent_tick(t_resent *x) {
    outlet_bang(x->m_bang);
}

void resent_init(t_resent *x)
{
    int i;
    short initialized = x->fft->initialized;
    t_fftease  *fft = x->fft;
    x->x_obj.z_disabled = 1;
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
    x->framecount =  x->duration/x->tadv ;
    x->read_me = 0;
    
    if(! initialized ){
        x->frame_increment = 1.0 ;
        x->mute = 0;
        x->playthrough = 0;
        x->sync = 0;
        x->frames_read = 0;
        x->frame_incr = (double *) sysmem_newptrclear(fft->N2 * sizeof(double));
        x->store_incr = (double *) sysmem_newptrclear(fft->N2 * sizeof(double));
        x->frame_phase = (double *) sysmem_newptrclear(fft->N2 * sizeof(double));
        x->loveboat = (double **) sysmem_newptrclear(x->framecount * sizeof(double *));
        for(i=0; i < x->framecount; i++){
            x->loveboat[i] = (double *) sysmem_newptrclear((fft->N+2) * sizeof(double));
            if(x->loveboat[i] == NULL){
                error("%s: Insufficient Memory!",OBJECT_NAME);
                return;
            }
        }
    }
    else { /* this could fail or might not actually release memory - test it!! */
        //		post("running memory reinit");
        x->frame_incr = (double *) sysmem_resizeptrclear(x->frame_incr, fft->N2 * sizeof(double));
        x->store_incr = (double *) sysmem_resizeptrclear(x->store_incr, fft->N2 * sizeof(double));
        x->frame_phase = (double *) sysmem_resizeptrclear(x->frame_phase, fft->N2 * sizeof(double));
        /*
         for(i = 0; i < x->framecount; i++){
         free(x->loveboat[i]) ;
         }
         free(x->loveboat);
         x->loveboat = (float **) calloc(x->framecount, sizeof(float *));
         */
        for(i = 0; i < x->last_framecount; i++){
            sysmem_freeptr(x->loveboat[i]) ;
        }
        x->loveboat = (double **)sysmem_resizeptrclear(x->loveboat, x->framecount * sizeof(double*));
        for(i=0; i < x->framecount; i++){
            x->loveboat[i] = (double *) sysmem_newptrclear((fft->N+2) * sizeof(double));
            if(x->loveboat[i] == NULL){
                error("%s: Insufficient Memory!",OBJECT_NAME);
                return;
            }
        }
    }
    x->last_framecount = x->framecount;
    x->x_obj.z_disabled = 0;
}

void *resent_new(t_symbol *msg, short argc, t_atom *argv)
{
    t_fftease *fft;
    t_resent *x = (t_resent *)object_alloc(resent_class);
    dsp_setup((t_pxobject *)x,1);
    outlet_new((t_pxobject *)x, "signal");
    outlet_new((t_pxobject *)x, "signal");
    
    x->fft = (t_fftease *) sysmem_newptrclear( sizeof(t_fftease) );
    fft = x->fft;
    fft->initialized = 0;
    fft->R = sys_getsr();
    fft->MSPVectorSize = sys_getblksize();
    
    srand(clock()); // needed ?
    x->duration = 1.0;
    
    fft->N = FFTEASE_DEFAULT_FFTSIZE;
    fft->overlap = FFTEASE_DEFAULT_OVERLAP;
    fft->winfac = FFTEASE_DEFAULT_WINFAC;
    x->interpolation_attr = 0; // off by default
    attr_args_process(x, argc, argv);
    resent_init(x);
    return x;
}

void do_resent(t_resent *x)
{
    t_fftease *fft = x->fft;
    
    int iphase, amp, freq, i;
    int N = fft->N;
    int N2 = fft->N2;
    double fframe = x->current_frame ;
    double last_fpos = x->last_fpos ;
    int framecount = x->framecount;
    double *frame_incr = x->frame_incr;
    double *frame_phase = x->frame_phase;
    double *channel = fft->channel;
    double frak;
    long iphase1, iphase2;
    
    if(x->read_me && x->framecount > 0){
        
        fold(fft);
        rdft(fft,FFT_FORWARD);
        convert(fft);
        // use memcopy
        for(i = 0; i < N; i++){
            x->loveboat[x->frames_read][i] = channel[i];
        }
        x->frames_read++;
        if(x->frames_read >= x->framecount){
            x->read_me = 0;
            // post("sample acquisition completed");
        }
        x->sync = (double) x->frames_read / (double) x->framecount;
    }
    else {
        if(x->interpolation_attr == 1){
            for( i = 0 ; i < N2; i++ ){
                amp = i<<1;
                freq = amp + 1;
                iphase1 = floor( frame_phase[i] );
                frak = frame_phase[i] - iphase1;
                if( iphase1 < 0 )
                    iphase1 = 0;
                if( iphase1 > framecount - 1 )
                    iphase1 = framecount - 1;
                iphase2 = (iphase1 + 1) % framecount;
                channel[amp] = x->loveboat[iphase1][amp] + (frak *
                                                            (x->loveboat[iphase2][amp] - x->loveboat[iphase1][amp]));
                channel[freq] = x->loveboat[iphase1][freq] + (frak *
                                                              (x->loveboat[iphase2][freq] - x->loveboat[iphase1][freq]));
                frame_phase[i] += frame_incr[i] ;
                while( frame_phase[i] > framecount - 1)
                    frame_phase[i] -= framecount;
                while( frame_phase[i] < 0. )
                    frame_phase[i] += framecount;
            }
        }
        else {
            for( i = 0 ; i < N2; i++ ){
                amp = i<<1;
                freq = amp + 1 ;
                iphase = frame_phase[i];
                if( iphase < 0 )
                    iphase = 0;
                if( iphase > framecount - 1 )
                    iphase = framecount - 1;
                channel[amp] = x->loveboat[iphase][amp];
                channel[freq] = x->loveboat[iphase][freq];
                frame_phase[i] += frame_incr[i] ;
                while( frame_phase[i] > framecount - 1)
                    frame_phase[i] -= framecount;
                while( frame_phase[i] < 0. )
                    frame_phase[i] += framecount;
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
    x->last_fpos = last_fpos;
    
}

void resent_perform64(t_resent *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam)
{
    int i, j;
    //////////////////////////////////////////////
    double *MSPInputVector = ins[0];
    double *MSPOutputVector = outs[0];
    double *sync_vec = outs[1];
    
    /* dereference structure */
    
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
    
    if( fft->obank_flag )
        mult *= FFTEASE_OSCBANK_SCALAR;
    
    if(x->mute || x->x_obj.z_disabled){
        for(i=0; i < vectorsize; i++){ MSPOutputVector[i] = 0.0; }
        for(i=0; i < vectorsize; i++){ sync_vec[i] = 0.0; }
        return;
    }
    if(x->playthrough && x->read_me){
        for (i = 0; i < MSPVectorSize; i++) {
            MSPOutputVector[i] = MSPInputVector[i] * 0.5; // scale down
            
        }
        for(i=0; i < vectorsize; i++){ sync_vec[i] = 0.0; }
        return;
    }
    // HERE IS THE GOOD STUFF
    //do_resent(x);
    if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector, input + (Nw - D), D * sizeof(t_double));
        
        do_resent(x);
        
        for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
    }
    else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
        for( i = 0; i < operationRepeat; i++ ){
            sysmem_copyptr(input + D, input, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(MSPInputVector + (D*i), input + (Nw-D), D * sizeof(t_double));
            
            do_resent(x);
            
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
            
            do_resent(x);
            
            for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
        }
        fft->operationCount = operationCount;
    }
    
    for ( i = 0; i < MSPVectorSize; i++ ){
        sync_vec[i] = x->sync;
    }
}

void resent_acquire_sample(t_resent *x)
{
    x->read_me = 1;
    x->frames_read = 0;
    //	post("%s: acquiring sample",OBJECT_NAME);
    return;
}

void resent_mute(t_resent *x, t_floatarg tog)
{
    x->mute = tog;
}

void resent_playthrough(t_resent *x, t_floatarg state)
{
    x->playthrough = state;
}

void resent_linephase(t_resent *x, t_symbol *msg, short argc, t_atom *argv)
{
    t_fftease *fft = x->fft;
    int bin1, bin2;
    float phase1, phase2, bindiff;
    int i;
    float m1, m2;
    
    bin1 = (int) atom_getfloatarg(0, argc, argv);
    phase1 = atom_getfloatarg(1, argc, argv) * x->framecount;
    bin2 = (int) atom_getfloatarg(2, argc, argv);
    phase2 = atom_getfloatarg(3, argc, argv) * x->framecount;
    
    if( bin1 > fft->N2 || bin2 > fft->N2 ){
        error("too high bin number");
        return;
    }
    bindiff = bin2 - bin1;
    if( bindiff < 1 ){
        error("make bin2 higher than bin 1, bye now");
        return;
    }
    for( i = bin1; i < bin2; i++ ){
        m2 = (float) i / bindiff;
        m1 = 1. - m2;
        x->frame_phase[i] = m1 * phase1 + m2 * phase2;
    }
}

void resent_randphase(t_resent *x, t_symbol *msg, short argc, t_atom *argv)
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

void resent_randspeed(t_resent *x, t_symbol *msg, short argc, t_atom *argv)
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

void resent_linespeed(t_resent *x, t_symbol *msg, short argc, t_atom *argv)
{
    t_fftease *fft = x->fft;
    int bin1, bin2;
    float speed1, speed2, bindiff;
    int i;
    float m1, m2;
    
    bin1 = (int) atom_getfloatarg(0, argc, argv);
    speed1 = atom_getfloatarg(1, argc, argv);
    bin2 = (int) atom_getfloatarg(2, argc, argv);
    speed2 = atom_getfloatarg(3, argc, argv);
    
    if( bin1 > fft->N2 || bin2 > fft->N2 ){
        error("too high bin number");
        return;
    }
    bindiff = bin2 - bin1;
    if( bindiff < 1 ){
        error("make bin2 higher than bin 1, bye now");
        return;
    }
    for( i = bin1; i < bin2; i++ ){
        m2 = (float) i / bindiff;
        m1 = 1. - m2;
        x->frame_incr[i] = m1 * speed1 + m2 * speed2;
    }
}


t_max_err get_fftsize(t_resent *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_resent *x, void *attr, long ac, t_atom *av)
{
    
    if (ac && av) {
        long val = atom_getlong(av);
        x->fft->N = (int) val;
        resent_init(x);
    }
    return MAX_ERR_NONE;
}

t_max_err get_overlap(t_resent *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_resent *x, void *attr, long ac, t_atom *av)
{	
    if (ac && av) {
        long val = atom_getlong(av);
        x->fft->overlap = (int) val;
        resent_init(x);
    }
    return MAX_ERR_NONE;
}



t_max_err get_interpolation(t_resent *x, void *attr, long *ac, t_atom **av)
{
    if (ac && av) {
        char alloc;
        
        if (atom_alloc(ac, av, &alloc)) {
            return MAX_ERR_GENERIC;
        }
        atom_setlong(*av, x->interpolation_attr);
    }
    return MAX_ERR_NONE;
}

t_max_err set_interpolation(t_resent *x, void *attr, long ac, t_atom *av)
{
    if (ac && av) {
        long val = atom_getlong(av);
        x->interpolation_attr = val;
    }
    return MAX_ERR_NONE;
}

t_max_err get_size(t_resent *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_size(t_resent *x, void *attr, long ac, t_atom *av)
{
    
    if (ac && av) {
        long val = atom_getlong(av);
        x->duration = (double)val/1000.0;
        //        post("duration set to %f", x->duration);
        resent_init(x);
    }
    return MAX_ERR_NONE;
}

void resent_dsp64(t_resent *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
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
        resent_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME))
        object_method(dsp64, gensym("dsp_add64"),x,resent_perform64,0,NULL);
}
