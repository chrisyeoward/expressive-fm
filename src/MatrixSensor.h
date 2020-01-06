#ifndef MATRIX_SENSOR_H_
#include <cmath>
#include <Bela.h>
#include <Point.h>
#include <algorithm>
#include <Scope.h>
#include <cmath>
#include <LowPassFilter.h>

#define MATRIX_DIMENSION 4
#define CALIBRATION_DURATION_S 5

enum CalibrationStates {
  kInit = 1,
  kMin = 2,
  kMax = 3,
  kFinished = 0,
};

class MatrixSensor
{
  public:
    explicit MatrixSensor(int (&analogPins)[MATRIX_DIMENSION]) : analogInputPins(analogPins) {};

    void setup(BelaContext* context);

    void calibrate(int analogFrame);

    bool isCalibrating();

    void takeNextSample(int n, int currentRow);

    Point readPosition();

    float matrixValues[MATRIX_DIMENSION][MATRIX_DIMENSION] = {};

  private:
    BelaContext* belaContext;

    CalibrationStates calState = kInit; // State of matrix

    int calibrationCount = 0; // Count of no. samples that have passed in current calibration state
    int calibrationDuration = 22050*CALIBRATION_DURATION_S; // Default calibration length in S.

    float inputMinimums[MATRIX_DIMENSION] = {};
    float inputMaximums[MATRIX_DIMENSION] = {};

    int (&analogInputPins)[MATRIX_DIMENSION]; // Analog pins that the matrix will read from

    int activeNodeX = 0;
    int activeNodeY = 0;
    float currentPressureInFrame = 0.0;

    float readX();
    float readY();

    /* LP filters for noise reduction */
    LowPassFilter xAxisNoiseFilter;
    LowPassFilter yAxisNoiseFilter;
    LowPassFilter zAxisNoiseFilter;
};

#endif /* MATRIX_SENSOR_H_ */
