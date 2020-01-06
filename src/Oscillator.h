#ifndef OSCILLATOR_H_
#include <cmath>
#include <Bela.h>

class Oscillator
{
  public:
    explicit Oscillator(float frequency) : frequency(frequency) {};

    void setup(float audioSampleRate);

    float getNextSample();

    void setFrequency(float frequency);

    void setAmplitude(float amplitude);

  private:
    float frequency = 440;
    float amplitude = 0;
    float phase = 0.0;
    float sampleRate = 44100;
};

#endif /* OSCILLATOR_H_ */
