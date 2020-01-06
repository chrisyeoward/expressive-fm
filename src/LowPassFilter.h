#ifndef LOWPASS_FILTER_H_

#define LOWPASS_FILTER_H_
#include <Filter.h>

/* Inherits from Filter class */
class LowPassFilter : public Filter
{

  public:
	LowPassFilter(){};

  /* Sets the filter coefficients for the second
  *  order Butterworth LP filter
  */
  void setup(float Fs, float cutOffFrequency) {
		float t = tan((M_PI*cutOffFrequency)/Fs);
 	 a[0] = pow(t,2) + t*sqrt(2) + 1;
 	 a[1] = 2*(pow(t,2)-1);
 	 a[2] = pow(t,2) - t*sqrt(2) + 1;


	 b[0] = pow(t,2);
	 b[1] = 2*pow(t,2);
	 b[2] = pow(t,2);

    for (int i=0; i<FILTER_ORDER; i++) {
      feedback[i] = 0;
      feedforward[i] = 0;
    }
  }
};

#endif /* LOWPASS_FILTER_H_ */
