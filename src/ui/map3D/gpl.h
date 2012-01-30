#ifndef GPL_H
#define GPL_H

#include <cmath>
#include <QString>

namespace qgc
{

template<class T>
const T clamp(const T& v, const T& a, const T& b)
{
    return qMin(b, qMax(a, v));
}

double hypot3(double x, double y, double z);
float hypot3f(float x, float y, float z);

template<class T>
const T normalizeTheta(const T& theta)
{
	T normTheta = theta;

    while (normTheta < - M_PI)
	{
		normTheta += 2.0 * M_PI;
	}
	while (normTheta > M_PI)
	{
		normTheta -= 2.0 * M_PI;
	}

	return normTheta;
}

double d2r(double deg);
float d2r(float deg);
double r2d(double rad);
float r2d(float rad);

template<class T>
const T square(const T& x)
{
	return x * x;
}

bool colormap(const QString& name, unsigned char idx,
			  float& r, float& g, float& b);

}

#endif
