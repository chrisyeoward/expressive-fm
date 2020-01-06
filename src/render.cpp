
#include <Bela.h>
#include <Oscillator.h>
#include <LowPassFilter.h>
#include <MatrixSensor.h>
#include <Delay.h>

#define PRESSURE_THRESHOLD 0.1

int gAudioFramesPerAnalogFrame;

enum ProgramState {
  kCalibratingCarrier = 0,
  kCalibratingModulator,
  kRendering,
};

ProgramState synthState = kCalibratingCarrier;

/** Input and output pin configuration **/
int gCarrierMatrixAnalogInputs[MATRIX_DIMENSION] = {0, 1, 2, 3};
int gModulatorMatrixAnalogInputs[MATRIX_DIMENSION] = {4, 5, 6, 7};

int gCarrierDigitalOutputPins[MATRIX_DIMENSION] = {P8_07, P8_08, P8_09, P8_10};
int gModulatorDigitalOutputPins[MATRIX_DIMENSION] = {P8_27, P8_28, P8_29, P8_30};

int gCurrentActiveDigitalOutputRow = 0;

/* Carrier oscillator configuration */
float gCarrierAmplitude = 0.0;
float gCarrierTuning = 220.0; // Remains constant
float gCarrierFrequency = gCarrierTuning;

Oscillator carrierOsc(gCarrierTuning);
MatrixSensor carrierMatrix(gCarrierMatrixAnalogInputs);
LowPassFilter carrierAmplitudeEnvFilter; // LP for the envelope

/* Modulator oscillator configuration */
float gModulatorAmplitude = 0.0;
float gModulatorFrequency = gCarrierFrequency;

Oscillator modulatorOsc(gModulatorFrequency);
MatrixSensor modulatorMatrix(gModulatorMatrixAnalogInputs);

/* Ping pong setup and config */
Delay pingPong;
LowPassFilter delayMixEnvFilter; // Low pass filter for the delay to avoid clicks on note off
float gDelayWetMix = 0;

void handleCarrierMatrixInput();
void handleModulatorMatrixInput();

bool setup(BelaContext *context, void *userData)
{
  carrierOsc.setup(context->audioSampleRate);
  carrierMatrix.setup(context);
  carrierAmplitudeEnvFilter.setup(context->audioSampleRate, 30);

  modulatorOsc.setup(context->audioSampleRate);
  modulatorMatrix.setup(context);

  pingPong.setup(context->audioSampleRate, 0.5); // Sets ping pong time (in S)
  delayMixEnvFilter.setup(context->audioSampleRate, 30);

  /* Initiliase GPIO pins */
  for (int pin = 0; pin < MATRIX_DIMENSION; pin++) {
    pinMode(context, 0, gCarrierDigitalOutputPins[pin], OUTPUT);
    pinMode(context, 0, gModulatorDigitalOutputPins[pin], OUTPUT);
  }

	gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

	return true;
}

void render(BelaContext *context, void *userData)
{
  /* Reset all pins each render cycle to avoid output pin status bleed */
  for (int pin = 0; pin < MATRIX_DIMENSION; pin++) {
    digitalWrite(context, 0, gCarrierDigitalOutputPins[pin], GPIO_LOW);
    digitalWrite(context, 0, gModulatorDigitalOutputPins[pin], GPIO_LOW);
  }

	for(unsigned int n = 0; n < context->audioFrames; n++) {

    /* Set current active row on for carrier and modulator matrix */
    digitalWriteOnce(context, n, gCarrierDigitalOutputPins[gCurrentActiveDigitalOutputRow], GPIO_HIGH);
    digitalWriteOnce(context, n, gModulatorDigitalOutputPins[gCurrentActiveDigitalOutputRow], GPIO_HIGH);

    if(n % gAudioFramesPerAnalogFrame == 0) {
      int analogFrame =  n/gAudioFramesPerAnalogFrame;

      /* Set calibrate matrix if uncalibrated, else render position */
      switch(synthState) {
        case kCalibratingCarrier:
        carrierMatrix.calibrate(analogFrame);
        synthState = carrierMatrix.isCalibrating() ? kCalibratingCarrier : kCalibratingModulator; // Check if finished calibrating
        break;

        case kCalibratingModulator:
        modulatorMatrix.calibrate(analogFrame);
        synthState = modulatorMatrix.isCalibrating() ? kCalibratingModulator : kRendering; // Check if finished calibrating
        break;

        case kRendering:
        carrierMatrix.takeNextSample(analogFrame, gCurrentActiveDigitalOutputRow); // Add next sample to matrix frame
        modulatorMatrix.takeNextSample(analogFrame, gCurrentActiveDigitalOutputRow); // Add next sample to matrix frame
        gCurrentActiveDigitalOutputRow++;

        if(gCurrentActiveDigitalOutputRow >= MATRIX_DIMENSION) { // Immediately process position if has reached the end of the matrix digital pins
          handleCarrierMatrixInput(); // Change pitch, amp and delay based on position
          handleModulatorMatrixInput(); // Change pitch and amp based on position

          gCurrentActiveDigitalOutputRow = 0; // Reset row
        }
        break;
      }
    }

    /* Update modulator wave parameters first */
    modulatorOsc.setAmplitude(gModulatorAmplitude);
    modulatorOsc.setFrequency(gModulatorFrequency);

    /* Update carrier wave parameters first */
    carrierOsc.setAmplitude(gCarrierAmplitude);
    carrierOsc.setFrequency(gCarrierFrequency + modulatorOsc.getNextSample());

    /* Render carrier wave signal */
    float synthOut = carrierOsc.getNextSample();

    for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
      float outputGainAttenuation = 0.5; // Attenuation factor to avoid distortion

      /* Renders output based on the delay mix.
      *  Note the gDelayWetMix is not applied to the function's value, but the ingoing signal,
      *  such that the delay acts more as a send, rather than absolute dry/wet
      */
      float out = (1 - gDelayWetMix) * synthOut + pingPong.render(channel, gDelayWetMix * synthOut);
      audioWrite(context, n, channel, outputGainAttenuation * out);
    };
  }
}

void cleanup(BelaContext *context, void *userData)
{

}

void handleCarrierMatrixInput(){
  Point carrierMatrixPosition = carrierMatrix.readPosition();

  /* If downward pressure is below threshold */
  if(carrierMatrixPosition.z > PRESSURE_THRESHOLD) {
    gCarrierAmplitude = carrierMatrixPosition.z; // Z controls amplitude
    gDelayWetMix = carrierMatrixPosition.y; // Y Delay mix
    gCarrierFrequency = gCarrierTuning * (pow(2, carrierMatrixPosition.x)); // X pitch
  } else { // If not below threshold
    gCarrierAmplitude = 0; // Set amp to 0, effectively a gate
    gDelayWetMix = 0; // Set delay mix to 0, fully dry.
  }
  gCarrierAmplitude = carrierAmplitudeEnvFilter.applyTo(gCarrierAmplitude);
  gDelayWetMix = delayMixEnvFilter.applyTo(gDelayWetMix);
};

void handleModulatorMatrixInput(){
  Point modulatorMatrixPosition = modulatorMatrix.readPosition();

  gModulatorFrequency = gCarrierFrequency * (modulatorMatrixPosition.x * 2); // Scale frequency to be between 0 -> 2 * carrier frequency
  gModulatorAmplitude = (10*gCarrierFrequency) * modulatorMatrixPosition.z; // Set modulation index
};
