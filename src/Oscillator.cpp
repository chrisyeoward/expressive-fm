#include <Oscillator.h>

void Oscillator::setup(float audioSampleRate) {
  sampleRate = audioSampleRate;
};

/* Renders the next sample of the oscillator
*  depending on the current amplitude and
*  frequency of the oscillator
*/
float Oscillator::getNextSample() {

  float output = amplitude * sinf(phase);

  phase += 2.0 * M_PI * frequency / sampleRate;

  if(phase > 2.0 * M_PI) phase -= 2.0 * M_PI;

  return output;
};

/* Used for updating the frequency
*/
void Oscillator::setFrequency(float frequency) {
  this->frequency = frequency;
};

/* Used for updating the amplitude
*/
void Oscillator::setAmplitude(float amplitude) {
  this->amplitude = amplitude;
};
