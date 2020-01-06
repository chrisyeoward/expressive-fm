#ifndef FILTER_H_

#define FILTER_H_
#define FILTER_ORDER 2

class Filter
{

	//private difference eq member properties
  protected:
	float a[FILTER_ORDER+1] = {0,0,0};
	float b[FILTER_ORDER+1] = {0,0,0};

	//private previous member properties describing previous input and outputs
	float feedforward[FILTER_ORDER] = {0,0};
	float feedback[FILTER_ORDER] = {0,0};

  public:
	Filter(){};

  void setup(float Fs, float cutOffFrequency) {}

  /* applyTo() applies the filter to the current input sample value,
	* based on the values stored in the feedback and feedforward arrays.
	*
	* Calculates output using the general form of the 2nd order difference
	* equation: a0y[n] = b0x[n] + b1x[n-1] + b2x[n-2] - (a1y[n-1] + a2y[n-2])
	*/
	float applyTo(float input) {
		float ff = b[0]*input + b[1]*feedforward[0] + b[2]*feedforward[1];
		float fb = a[1]*feedback[0] + a[2]*feedback[1];

		float output = (ff - fb)/a[0];

		feedforward[1] = feedforward[0];
		feedforward[0] = input;

		feedback[1] = feedback[0];
		feedback[0] = output;

		return output;
	}
};

#endif /* FILTER_H_ */
