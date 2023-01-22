#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ext.h"
#include "z_dsp.h"
#include "buffer.h"
#include "ext_obex.h"


#define t_floatarg double
#define FFTEASE_ANNOUNCEMENT "<[ FFTease 3.0 ]>   |  "
#define FFTEASE_VERSION "FFTease 3.0 for Max 6.1"
#define FFTEASE_COMPILE_DATE "June 28, 2012"

#define fftease_version(objectname) post("%s: version %s compiled %s",objectname,FFTEASE_VERSION,FFTEASE_COMPILE_DATE);

#define MAX_N (1073741824)
#define MAX_N2 (MAX_N/2)
#define MAX_Nw (MAX_N*4)

#ifndef PI
#define PI 3.141592653589793115997963468544185161590576171875
#endif

#ifndef TWOPI
#define TWOPI 6.28318530717958623199592693708837032318115234375
#endif

#define BIGGER_THAN_MSP_VECTOR 0
#define SMALLER_THAN_MSP_VECTOR 1
#define EQUAL_TO_MSP_VECTOR 2

#define FFTEASE_DEFAULT_FFTSIZE 1024
#define FFTEASE_DEFAULT_OVERLAP 8
#define FFTEASE_DEFAULT_WINFAC 1
	
#define DEFAULT_FFTEASE_FFTSIZE 1024
#define FFTEASE_MAX_FFTSIZE 1073741824

#define FFTEASE_OSCBANK_SCALAR (0.25)
#define FFTEASE_OSCBANK_TABLESIZE (8192)
#define FFTEASE_BYPASS_GAIN (0.5)

#define FFT_FORWARD 1
#define FFT_INVERSE -1
typedef struct _fftease
{
	int R;
	int	N;
	int	N2;
	int	Nw;
	int	Nw2; 
	int	D; 
	int	in_count;
	int out_count;
	double *Wanal;
	double *Wsyn;	
	double *input;	
	double *Hwin;
	double *buffer;
	double *channel;
	double *output;
	// for convert
	double *c_lastphase_in;
	double *c_lastphase_out;
	double c_fundamental;
	double c_factor_in;
	double c_factor_out;
	// for oscbank
	int NP;
	double P;
	int L;
	int first;
	double Iinv;
	double *lastamp;
	double *lastfreq;
	double *bindex;
	double *table;
	double pitch_increment;
	double ffac;
	int hi_bin;
	int lo_bin;
	// for fast fft
	double mult; 
	double *trigland;
	int *bitshuffle;
	int overlap;
	int winfac;
    int last_overlap; // save values to test if memory reallocation needed
    int last_winfac;
    int last_N;
    int last_R;
	double synt;
	double *internalInputVector; // hold input data from smaller MSP buffers
	double *internalOutputVector; // hold output data for smaller MSP buffers
	int operationRepeat; // how many times to do whatever on each perform call
	int operationCount; // keep track of where we are in buffer operation
	int bufferStatus; // relations between MSP vector size and internal buffer size
	int MSPVectorSize; // what it says
	short obank_flag; // resynthesis method flag
	short init_status; // whether initialization has successfully occurred
	short noalias; // inhibit aliasing in oscbank mode
	double nyquist; // nyquest frequency == R/2
	short initialized; // set to 0 for the first time in new(); after that it will be 1
} t_fftease;



void convert(t_fftease *fft);
void unconvert(t_fftease *fft);

void rfft( double *x, int N, int forward );
void cfft( double *x, int NC, int forward );
void bitreverse( double *x, int N );
void fold( t_fftease *fft );
void init_rdft(int n, int *ip, double *w);
void rdft(t_fftease *fft, int isgn);
void bitrv2(int n, int *ip, double *a);
void cftsub(int n, double *a, double *w);
void rftsub(int n, double *a, int nc, double *c);
void makewt(int nw, int *ip, double *w);
void makect(int nc, int *ip, double *c);
void leanconvert(t_fftease *fft);
void leanunconvert(t_fftease *fft);
void makewindows( double *H, double *A, double *S, int Nw, int N, int I );
void makehamming( double *H, double *A, double *S, int Nw, int N, int I,int odd );
void makehanning( double *H, double *A, double *S, int Nw, int N, int I,int odd );
void overlapadd(t_fftease *fft);
void bloscbank( double *S, double *O, int D, double iD, double *lf, double *la,
	double *bindex, double *tab, int len, double synt, int lo, int hi );
void oscbank( t_fftease *fft );
double randf( double min, double max );
int randi( int min, int max );
int fftease_power_of_two(int test);

void freebytes2(void *fatso, size_t nbytes);
void *getbytes2(size_t nbytes);
void *resizebytes2(void *old, size_t oldsize, size_t newsize);
void limit_fftsize(int *N, int *Nw, char *OBJECT_NAME);
int fftease_fft_size(int testfft);
void fftease_free(t_fftease *fft);
void fftease_init(t_fftease *fft);
int fftease_winfac(int winfac);
int fftease_overlap(int overlap);
void fftease_set_fft_buffers(t_fftease *fft);
void fftease_fftinfo(t_fftease *fft, char *object_name);
int fftease_msp_sanity_check(t_fftease *fft, char *oname);
double fftease_randf(double min, double max);
void fftease_noalias(t_fftease* fft, short flag);
void fftease_oscbank_setbins(t_fftease *fft, double lowfreq, double highfreq);
void limited_oscbank(t_fftease *fft, int osclimit, double framethresh);
double fftease_randf(double min, double max);
int fftease_randi(int min, int max);
// Penrose extras
double frequencyToIncrement( double samplingRate, double frequency, int bufferLength );
void makeSineBuffer( double *buffer, int bufferLength );
double bufferOscil( double *phase, double increment, double *buffer, int bufferLength );
float rrand(int *seed);
float prand(int *seed);
