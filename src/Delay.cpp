#include <Delay.h>

/* Setup sets the delay length by placing the read pointer at
*  a position in the circular buffer that is behind the write
*  pointer by the delay amount
*/
void Delay::setup(float audioSampleRate, float delayLengthS) {
  delayLength = delayLengthS * audioSampleRate;
  readPointer[L] = ((writePointer[L] + DELAY_BUF_LEN) - delayLength) % DELAY_BUF_LEN; // Ensure sample is in range 0 -> buffer size
  readPointer[R] = ((writePointer[R] + DELAY_BUF_LEN) - delayLength) % DELAY_BUF_LEN;
};

/* Public render function
*/
float Delay::render(int channel, float input) {
  switch(channel) {
    case L:
    return renderLeft(input);
    break;

    case R:
    return renderRight();
    break;

    default:
    return 0.0;
  }
}

/* Private render function for the left channel
* Does not increment the left write pointer as
* the pointer position is required for output of the right channel
*/
float Delay::renderLeft(float inputSample) {
  float delayedSample = buffer[L][readPointer[L]]; // Read out sample at read pointer

  buffer[L][writePointer[L]] = inputSample; // Write in the current sample at write pointer position
  buffer[R][writePointer[R]] = delayedSample; // Write in the delayed sample to the right channel

  readPointer[L]++;
  readPointer[L] %= DELAY_BUF_LEN; // Wrap pointer

  writePointer[R]++;
  writePointer[R] %= DELAY_BUF_LEN; // Increment right channel write pointer, as the current position is no longer needed
  return delayedSample; // return sample from buffer
};

/* Private render function for the right channel
* Does not increment the right write pointer as
* that was already done in the left render
*/
float Delay::renderRight() {
  float delayedSample = buffer[R][readPointer[R]]; // Read out sample at read pointer

  buffer[L][writePointer[L]] += feedbackGain * delayedSample; // Write in the delayed sample to the left channel, with feedback

  readPointer[R]++;
  readPointer[R] %= DELAY_BUF_LEN;

  writePointer[L]++;
  writePointer[L] %= DELAY_BUF_LEN;
  return delayedSample; // return sample from buffer
};
