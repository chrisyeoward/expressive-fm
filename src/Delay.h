#ifndef DELAY_H_

#define DELAY_BUF_LEN 44100
#include <Channel.h>

class Delay
{
  public:
    Delay(){};

    void setup(float audioSampleRate, float delayLengthS);
    float render(int channel, float inputSample);

  private:

    int delayLength = DELAY_BUF_LEN;
    int readPointer[2] = {1, 1};
    int writePointer[2] = {};
    float buffer[2][DELAY_BUF_LEN] = {};
    float feedbackGain = 0.5;

    float renderLeft(float inputSample);
    float renderRight();
};

#endif /* DELAY_H_ */
