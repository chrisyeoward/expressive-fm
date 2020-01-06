#include <MatrixSensor.h>

/* Interpolation function
*  Uses quadratic interpolation to return a point between -0.5 and 0.5,
*  estimating the position of the peak between values
*
*  alpha = index of peak - 1
*  beta = index of peak
*  gamma = index of peak + 1
*/
float interpolatePosition(float alpha, float beta, float gamma) {
  return (alpha - gamma) / (2 * (gamma - 2*beta + alpha));
}

/* Setup uses sampling rate to set filters and calibration duraction */
void MatrixSensor::setup(BelaContext *context) {
  belaContext = context;
  calibrationDuration = context->analogSampleRate * CALIBRATION_DURATION_S;

  xAxisNoiseFilter.setup(context->digitalSampleRate, 50);
  yAxisNoiseFilter.setup(context->digitalSampleRate, 50);
  zAxisNoiseFilter.setup(context->digitalSampleRate, 50);
};

/* Publid method to determines if the matrix has finished calibrating  */
bool MatrixSensor::isCalibrating(){
  return calState != kFinished;
}

/* Calibrate function called each frame
*  First the minima are measured,
*  then pressure is applied to all nodes along the first row
*  by an operator to calculate the maximum values for each input.
*/
void MatrixSensor::calibrate(int analogFrame){
  switch(calState) {
    case kInit:
    rt_printf("Calibrating minima, do nothing... \n");
    calState = kMin;

    case kMin:
    for (int column = 0; column < MATRIX_DIMENSION; column++) {
      float input;
      input = analogRead(belaContext, analogFrame, analogInputPins[column]);
      if (input > inputMinimums[column]) {
        inputMinimums[column] = input;
      }
    }
    calibrationCount++;
    if(calibrationCount >= calibrationDuration) {
      rt_printf("Finished calibrating minima, now push down to calibrate maxima...\n");
      calState = kMax;
      calibrationCount = 0;
    }
    break;

    case kMax:
    for (int column = 0; column < MATRIX_DIMENSION; column++) {
      float input;
      input = analogRead(belaContext, analogFrame, analogInputPins[column]);
      if (input > inputMaximums[column]) {
        inputMaximums[column] = input;
      }
    }
    calibrationCount++;
    if(calibrationCount >= calibrationDuration) {
      rt_printf("Finished calibrating!\n");
      rt_printf("Maxs: %f %f %f %f\n", inputMaximums[0], inputMaximums[1], inputMaximums[2], inputMaximums[3]);
      rt_printf("Mins: %f %f %f %f\n", inputMinimums[0], inputMinimums[1], inputMinimums[2], inputMinimums[3]);
      calState = kFinished;
    }
    break;

    case kFinished:
    break;
  }
};


/* Sampling function
*  Called each frame with a different row value
*  Measures the analog inputs for the current frame
*  Determines the node position of the current touch
*/
void MatrixSensor::takeNextSample(int analogFrame, int currentRow) {
  for (int column = 0; column < MATRIX_DIMENSION; column++) {
    int row = analogInputPins[column] == 6 || analogInputPins[column] == 7 ? (currentRow + 1) % MATRIX_DIMENSION : currentRow;

    matrixValues[column][row] = map(analogRead(belaContext, analogFrame, analogInputPins[column]), inputMinimums[column], inputMaximums[column], 0, 1); // Maps the analog values to be from the calibrated maxima and minima, to be between 0 and 1.

    if(matrixValues[column][row] > currentPressureInFrame) { // If the value at the current node exceeds the greatest value measure so far
      currentPressureInFrame = matrixValues[column][row];
      activeNodeX = column; // Set position of touch to current node
      activeNodeY = row; // Set position of touch to current node
    }
  }
};


/* Calculates coordinates of touch in 3d
*  Called when sampling of the the matrix is complete and the frame is full
*  Determines the position of the touch
*  Initialises sampling values for next frame
*/
Point MatrixSensor::readPosition() {
  float x, y, z = zAxisNoiseFilter.applyTo(currentPressureInFrame);

  x = readX();
  y = readY();

  activeNodeX = 0;
  activeNodeY = 0;
  currentPressureInFrame = 0.0;

  return Point(x, y, z);
};

/* Calculates x position
*  Called when sampling of the the matrix is complete and the frame is full
*  Determines the position of the x position based on the node with the
*  highest pressure.
*  Uses values of the neighbouring nodes to interpolate.
*/
float MatrixSensor::readX() {
  float alpha, beta, gamma;
  beta = matrixValues[activeNodeX][activeNodeY];

  alpha = activeNodeX == 0 ? 0.0 : matrixValues[activeNodeX - 1][activeNodeY]; // If first node, set node - 1 to be 0
  gamma = activeNodeX >= (MATRIX_DIMENSION - 1) ? 0.0 : matrixValues[activeNodeX + 1][activeNodeY]; // If last node, set node + 1 to be 0
  float peakPosition = interpolatePosition(alpha, beta, gamma);
  float x = (float) activeNodeX + peakPosition;

  x /= MATRIX_DIMENSION - 1; // Normalise
  x = max(x, 0); // Clamp to never go below 0
  x = min(x, 1); // Clamp to never go above 1

  return xAxisNoiseFilter.applyTo(x); // Apply lowpass to reduce noise
}


/* Same as above
*/
float MatrixSensor::readY() {
  float alpha, beta, gamma;
  beta = matrixValues[activeNodeX][activeNodeY];

  alpha = activeNodeY == 0 ? 0.0 : matrixValues[activeNodeX][activeNodeY - 1];
  gamma = activeNodeY >= (MATRIX_DIMENSION - 1) ? 0.0 : matrixValues[activeNodeX][activeNodeY + 1];
  float peakPosition = interpolatePosition(alpha, beta, gamma);
  float y = (float) activeNodeY + peakPosition;

  y /= MATRIX_DIMENSION - 1;
  y = max(y, 0);
  y = min(y, 1);

  return yAxisNoiseFilter.applyTo(y);
}
