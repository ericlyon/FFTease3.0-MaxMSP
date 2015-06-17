#include <math.h>
#include "PenroseOscil.h"


double frequencyToIncrement( double samplingRate, double frequency, int bufferLength ) {

  return (frequency / samplingRate) * (double) bufferLength;
} 

void makeSineBuffer( double *buffer, int bufferLength ) {
  
  int   i;

  float myTwoPi = 8. * atan(1.);

  for ( i=0; i <= bufferLength; i++ )
    *(buffer+i) = sin( myTwoPi * ((double) i / (double) bufferLength) );

  return;
}


double bufferOscil( double *phase, double increment, double *buffer,
                   int bufferLength )
{

  double sample;

  while ( *phase > bufferLength )
    *phase -= bufferLength;

  while ( *phase < 0. )
    *phase += bufferLength;

  sample = *( buffer + (int) (*phase) );

  *phase += increment;

  return sample;
}
