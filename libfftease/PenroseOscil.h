double frequencyToIncrement( double samplingRate, double frequency, int bufferLength );
void makeSineBuffer( double *buffer, int bufferLength );
double bufferOscil( double *phase, double increment, double *buffer, int bufferLength );
