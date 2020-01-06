
#ifndef POINT_H_

/* Auxiliary data structure for position */
struct Point
{
  float x;
  float y;
  float z;

  Point(float x, float y, float z) : x(x), y(y), z(z) {};
};

#endif /* POINT_H_ */
