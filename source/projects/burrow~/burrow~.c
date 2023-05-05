#include "fftease.h"

static t_class *burrow_class;

#define OBJECT_NAME "burrow~"

/* after adding fixes, window factors > 1 are defective. Is there
 a remaining bug, or is this a problem for FFT-only processors? */

/* A few changes:
 
 Threshold and Multiplier now have their own
 inlets, which accept (signal/float). The input
 is now linear, rather than in dB. Reasons for this:
 
 1) Linear input is the Max/MSP convention
 2) It is easy to convert from linear to dB in Max
 3) (My favorite) This cuts down on programmer overhead.
 
 */

typedef struct _burrow
{
    t_pxobject x_obj;
    t_fftease *fft;
    t_fftease *fft2; // for cross synthesis use
    int invert;
    double threshold;
    double multiplier;
    short connected[8];
    short mute;
    short bypass;
    long fftsize_attr;
    long overlap_attr;
} t_burrow;


/* msp function prototypes */

void *burrow_new(t_symbol *s, int argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *burrow_perform(t_int *w);
void burrow_dsp(t_burrow *x, t_signal **sp, short *count);
void burrow_assist(t_burrow *x, void *b, long m, long a, char *s);
void burrow_float(t_burrow *x, t_floatarg myFloat);
void burrow_init(t_burrow *x);
void burrow_free(t_burrow *x);
void burrow_invert(t_burrow *x, t_floatarg toggle);
void burrow_mute(t_burrow *x, t_floatarg toggle);
void burrow_fftinfo(t_burrow *x);
void burrow_tilde_setup(void);
void burrow_winfac(t_burrow *x, t_floatarg f);
void do_burrow(t_burrow *x);
void burrow_bypass(t_burrow *x, t_floatarg toggle);
t_max_err set_fftsize(t_burrow *x, void *attr, long ac, t_atom *av);
t_max_err get_fftsize(t_burrow *x, void *attr, long *ac, t_atom **av);
t_max_err set_overlap(t_burrow *x, void *attr, long ac, t_atom *av);
t_max_err get_overlap(t_burrow *x, void *attr, long *ac, t_atom **av);
void burrow_perform64(t_burrow *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam);
void burrow_dsp64(t_burrow *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);

int C74_EXPORT main(void)
{
    t_class *c;
    c = class_new("fftz.burrow~", (method)burrow_new, 
                  (method)burrow_free, sizeof(t_burrow), 0,A_GIMME, 0);
    class_addmethod(c,(method)burrow_dsp64, "dsp64", A_CANT, 0);
    class_addmethod(c,(method)burrow_assist,"assist",A_CANT,0);    
    class_addmethod(c,(method)burrow_invert,"invert", A_FLOAT, 0);
    class_addmethod(c,(method)burrow_mute,"mute", A_FLOAT, 0);
    class_addmethod(c,(method)burrow_winfac,"winfac",A_FLOAT,0);
    class_addmethod(c,(method)burrow_bypass,"bypass",A_FLOAT,0);
    class_addmethod(c,(method)burrow_fftinfo,"fftinfo", 0);
    class_addmethod(c,(method)burrow_float,"float", A_FLOAT, 0);
    
    CLASS_ATTR_LONG(c, "fftsize", 0, t_burrow, fftsize_attr);
    CLASS_ATTR_ACCESSORS(c, "fftsize", (method)get_fftsize, (method)set_fftsize);
    CLASS_ATTR_LABEL(c, "fftsize", 0, "FFT Size");	
    
    CLASS_ATTR_LONG(c, "overlap", 0, t_burrow, overlap_attr);
    CLASS_ATTR_ACCESSORS(c, "overlap", (method)get_overlap, (method)set_overlap);
    CLASS_ATTR_LABEL(c, "overlap", 0, "Overlap");	
    
    CLASS_ATTR_ORDER(c, "fftsize",    0, "1");
    CLASS_ATTR_ORDER(c, "overlap",    0, "2");
    class_dspinit(c);
    class_register(CLASS_BOX, c);
    burrow_class = c;
    post("%s%s", FFTEASE_ANNOUNCEMENT, OBJECT_NAME);
    return 0;
}

t_max_err get_fftsize(t_burrow *x, void *attr, long *ac, t_atom **av)
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

t_max_err set_fftsize(t_burrow *x, void *attr, long ac, t_atom *av)
{
    
    if (ac && av) {
        long val = atom_getlong(av);
        x->fft->N = (int) val;
        x->fft2->N = (int) val;
        burrow_init(x);
    }
    return MAX_ERR_NONE;
}

t_max_err get_overlap(t_burrow *x, void *attr, long *ac, t_atom **av)
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


t_max_err set_overlap(t_burrow *x, void *attr, long ac, t_atom *av)
{	
    if (ac && av) {
        long val = atom_getlong(av);
        x->fft->overlap = (int) val;
        x->fft2->overlap = (int) val;
        burrow_init(x);
    }
    return MAX_ERR_NONE;
}


/* float input handling routines (MSP only) */

void burrow_float(t_burrow *x, t_floatarg myFloat)
{
    int inlet = ((t_pxobject*)x)->z_in;
    if ( inlet == 2 ) // added two outlets so position is moved over
        x->threshold = myFloat; 
    
    if ( inlet == 3 )
        x->multiplier = myFloat;
}


void burrow_free(t_burrow *x)
{
    
    dsp_free((t_pxobject *) x);
    //t_fftease *fft = x->fft;
    //t_fftease *fft2 = x->fft2;
    fftease_free(x->fft);
    fftease_free(x->fft2);
}

void burrow_invert(t_burrow *x, t_floatarg toggle)
{
    x->invert = (short)toggle;
}

void burrow_mute(t_burrow *x, t_floatarg toggle)
{
    x->mute = (short)toggle;
}

void burrow_bypass(t_burrow *x, t_floatarg toggle)
{
    x->bypass = (short)toggle;
}

void burrow_winfac(t_burrow *x, t_floatarg f)
{
    x->fft->winfac = (int) f;
    x->fft2->winfac = (int) f;
    burrow_init(x);
}


void burrow_fftinfo( t_burrow *x )
{
    fftease_fftinfo(x->fft, OBJECT_NAME);
}


/* diagnostic messages for Max */

void burrow_assist (t_burrow *x, void *b, long msg, long arg, char *dst)
{
    
    if (msg == 1) {
        
        switch (arg) {
            case 0: sprintf(dst,"(signal) Source Sound"); break;
            case 1: sprintf(dst,"(signal) Burrow Filtering Sound"); break;
            case 2: sprintf(dst,"(signal/float) Filter Threshold"); break;
            case 3: sprintf(dst,"(signal/float) Filter Multiplier"); break;
        }
    }
    
    else {
        if (msg == 2)
            sprintf(dst,"(signal) Output");
    }
}

void burrow_init(t_burrow *x)
{
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    short initialized = fft->initialized;
    x->x_obj.z_disabled = 1;
    
    fftease_init(fft);
    fftease_init(fft2);
    
    if(!initialized){
        x->mute = 0;
        x->invert = 0;
    } 
    x->x_obj.z_disabled = 0;
    
}

void *burrow_new(t_symbol *s, int argc, t_atom *argv)
{
    t_fftease *fft, *fft2;
    t_burrow 	*x = (t_burrow *) object_alloc(burrow_class);
    dsp_setup((t_pxobject *)x,4);
    outlet_new((t_pxobject *)x, "signal");
    
    x->fft = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
    x->fft2 = (t_fftease *) sysmem_newptrclear(sizeof(t_fftease));
    
    fft = x->fft;
    fft2 = x->fft2;	
    fft->initialized = fft2->initialized = 0;
    x->threshold = 0.0;
    x->multiplier = 0.01;
    
    fft->N = FFTEASE_DEFAULT_FFTSIZE;
    fft->overlap = FFTEASE_DEFAULT_OVERLAP;
    fft->winfac = FFTEASE_DEFAULT_WINFAC;
    fft2->N = FFTEASE_DEFAULT_FFTSIZE;
    fft2->overlap = FFTEASE_DEFAULT_OVERLAP;
    fft2->winfac = FFTEASE_DEFAULT_WINFAC;	
    
    fft2->R = fft->R = sys_getsr();
    fft2->MSPVectorSize = fft->MSPVectorSize = sys_getblksize();
    
    attr_args_process(x, argc, argv);
    burrow_init(x);
    return x;
}


void burrow_perform64(t_burrow *x, t_object *dsp64, double **ins,
                      long numins, double **outs,long numouts, long vectorsize,
                      long flags, void *userparam)
{
    /* get our inlets and outlets */
    
    t_double *MSPInputVector1 = ins[0];
    t_double *MSPInputVector2 = ins[1];
    t_double *flt_threshold = ins[2];
    t_double *flt_multiplier = ins[3];
    t_double *MSPOutputVector = outs[0];
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    int MSPVectorSize = fft->MSPVectorSize;
    int operationRepeat = fft->operationRepeat;
    int operationCount = fft->operationCount;
    t_double *internalInputVector1 = fft->internalInputVector;
    t_double *internalInputVector2 = fft2->internalInputVector;
    t_double *internalOutputVector = fft->internalOutputVector;
    int D = fft->D;
    int Nw = fft->Nw;
    t_double *output = fft->output;
    t_double mult = fft->mult;
    
    if( fft->obank_flag )
        mult *= FFTEASE_OSCBANK_SCALAR;
    
    short *connected = x->connected;
    
    int
    i,j,
    invert = 0,
    threshold = 1.,
    multiplier = 1.;
    t_double
    *inputOne,
    *inputTwo,
    *bufferOne,
    *bufferTwo,
    *channelOne,
    *channelTwo;
    
    /* dereference structure  */
    
    inputOne = fft->input;
    inputTwo = fft2->input;
    bufferOne = fft->buffer;
    bufferTwo = fft2->buffer;
    
    channelOne = fft->channel;
    channelTwo = fft2->channel;
    multiplier = x->multiplier;
    threshold = x->threshold;
    
    invert = x->invert;
    
    mult = fft->mult;
    
    if(connected[2]){
        x->threshold = *flt_threshold;
    }
    
    if(connected[3]){
        x->multiplier = *flt_multiplier;
    }
    
    /* save some CPUs if muted */
    if(x->mute || x->x_obj.z_disabled){
        for(i=0; i < vectorsize; i++){
            MSPOutputVector[i] = 0.0;
        }
        return;
    }
    if (x->bypass) {
        for( j = 0; j < MSPVectorSize; j++) {
            *MSPOutputVector++ = *MSPInputVector1++ * FFTEASE_BYPASS_GAIN;
        }
        return;
    }
    // do_burrow(x);
    
    
    if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        sysmem_copyptr(inputOne + D, inputOne, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector1, inputOne + (Nw - D), D * sizeof(t_double));
        sysmem_copyptr(inputTwo + D, inputTwo, (Nw - D) * sizeof(t_double));
        sysmem_copyptr(MSPInputVector2, inputTwo + (Nw - D), D * sizeof(t_double));
        do_burrow(x);
        for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
    }
    else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
        for( i = 0; i < operationRepeat; i++ ){
            sysmem_copyptr(inputOne + D, inputOne, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(MSPInputVector1 + (D*i), inputOne + (Nw-D), D * sizeof(t_double));
            sysmem_copyptr(inputTwo + D, inputTwo, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(MSPInputVector2 + (D*i), inputTwo + (Nw-D), D * sizeof(t_double));
            do_burrow(x);
            for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw-D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
        }
    }
    else if( fft->bufferStatus == BIGGER_THAN_MSP_VECTOR ) {
        sysmem_copyptr(MSPInputVector1,internalInputVector1 + (operationCount * MSPVectorSize),MSPVectorSize * sizeof(t_double));
        sysmem_copyptr(MSPInputVector2,internalInputVector2 + (operationCount * MSPVectorSize),MSPVectorSize * sizeof(t_double));
        sysmem_copyptr(internalOutputVector + (operationCount * MSPVectorSize),MSPOutputVector,MSPVectorSize * sizeof(t_double));
        
        operationCount = (operationCount + 1) % operationRepeat;
        if( operationCount == 0 ) {
            sysmem_copyptr(inputOne + D, inputOne, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(internalInputVector1, inputOne + (Nw - D), D * sizeof(t_double));
            sysmem_copyptr(inputTwo + D, inputTwo, (Nw - D) * sizeof(t_double));
            sysmem_copyptr(internalInputVector2, inputTwo + (Nw - D), D * sizeof(t_double));
            do_burrow(x);
            for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            sysmem_copyptr(output + D, output, (Nw - D) * sizeof(t_double));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
        }
        fft->operationCount = operationCount;
    }
}

void do_burrow(t_burrow *x)
{
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    int i;
    int N2 = fft->N2;
    float a1, b1,
    a2, b2;
    int even, odd;
    
    /* dereference structure  */	
    
    t_double *bufferOne = fft->buffer;
    t_double *bufferTwo = fft2->buffer;
    
    t_double *channelOne = fft->channel;
    t_double *channelTwo = fft2->channel;
    t_double multiplier = x->multiplier;	
    t_double threshold = x->threshold;
    
    int invert = x->invert;
    
    
    /* apply hamming window and fold our window buffer into the fft buffer */ 
    
    fold(fft);
    fold(fft2);
    
    
    /* do an fft */ 
    
    rdft(fft, 1);
    rdft(fft2, 1);
    
    if (invert) {
        
        /* convert to polar coordinates from complex values */
        
        for ( i = 0; i <= N2; i++ ) {
            
            odd = ( even = i<<1 ) + 1;
            
            a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
            b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
            
            a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
            b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
            
            *(channelOne+even) = hypot( a1, b1 );
            *(channelOne+odd) = -atan2( b1, a1 );
            
            *(channelTwo+even) = hypot( a2, b2 );
            
            /* use simple threshold from second signal to trigger filtering */
            
            if ( *(channelTwo+even) < threshold )
                *(channelOne+even) *= multiplier;
            
        }  
    }
    
    else {
        
        /* convert to polar coordinates from complex values */
        
        for ( i = 0; i <= N2; i++ ) {
            
            odd = ( even = i<<1 ) + 1;
            
            a1 = ( i == N2 ? *(bufferOne+1) : *(bufferOne+even) );
            b1 = ( i == 0 || i == N2 ? 0. : *(bufferOne+odd) );
            
            a2 = ( i == N2 ? *(bufferTwo+1) : *(bufferTwo+even) );
            b2 = ( i == 0 || i == N2 ? 0. : *(bufferTwo+odd) );
            
            *(channelOne+even) = hypot( a1, b1 );
            *(channelOne+odd) = -atan2( b1, a1 );
            
            *(channelTwo+even) = hypot( a2, b2 );
            
            /* use simple threshold from second signal to trigger filtering */
            
            if ( *(channelTwo+even) > threshold )
                *(channelOne+even) *= multiplier;						
        }  
    }
    
    /* convert back to complex form, read for the inverse fft */
    
    for ( i = 0; i <= N2; i++ ) {
        
        odd = ( even = i<<1 ) + 1;
        
        *(bufferOne+even) = *(channelOne+even) * cos( *(channelOne+odd) );
        
        if ( i != N2 )
            *(bufferOne+odd) = -(*(channelOne+even)) * sin( *(channelOne+odd) );
    }
    
    
    /* do an inverse fft */
    
    rdft(fft, -1);
    
    /* dewindow our result */
    
    overlapadd(fft);
    
    // no oscbank option for this object
}

void burrow_dsp64(t_burrow *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
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
    for( i = 0; i < 4; i++ ){
        x->connected[i] = count[i];
        //  post(x->connected[i]);
    }
    if(fft->MSPVectorSize != maxvectorsize){
        fft->MSPVectorSize = maxvectorsize;
        fftease_set_fft_buffers(fft);
        fft2->MSPVectorSize = maxvectorsize;
        fftease_set_fft_buffers(fft2);
    }
    if(fft->R != samplerate ){
        fft->R = samplerate;
        fft2->R = samplerate;
    }
    if(reset_required){
        burrow_init(x);
    }
    object_method(dsp64, gensym("dsp_add64"),x,burrow_perform64,0,NULL);
}
