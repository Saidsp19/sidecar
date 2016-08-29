#include "Utils.h"

double const Utils::kCircleRadians = M_PI * 2.0;

double
Utils::normalizeRadians(double value)
{
    while (value < 0.0) value += kCircleRadians;
    while (value >= 2 * M_PI) value -= kCircleRadians;
    return value;
}

double
Utils::normalizeDegrees(double value)
{
    while (value < 0.0) value += 360.0;
    while (value >= 360.0) value -= 360.0;
    return value;
}

uint32_t
Utils::leastPowerOf2(uint32_t value)
{
    value = value - 1;
    value |= (value >> 1);
    value |= (value >> 2);
    value |= (value >> 4);
    value |= (value >> 8);
    value |= (value >> 16);
    return value + 1;
}

double Utils::el_from_range_and_alt(double range, double altitude,double refLatitudeRadians) {
    
    if (range < altitude)  
        return 0;
    
    /*  Find the radius of curvature at the vector's origin.  */
    double ESQ = ((double) 0.00669437999013);
    double WGS84_SEMI_MAJOR_AXIS = 6378137.0;
    double SINE_OF_LAT = ::sin(refLatitudeRadians);
    double TEMP  = (1.0 - ESQ * SINE_OF_LAT * SINE_OF_LAT);
    double Q_DEN = ::pow(TEMP, 1.5);
    double Q_NUM = WGS84_SEMI_MAJOR_AXIS*(1.0 - ESQ);
    double CURVE_RADIUS = Q_NUM / Q_DEN;

    /*  Use the law of cosines to find the elevation.  */
    double HEIGHT = CURVE_RADIUS + altitude;
    double ELEVATION = (::acos((CURVE_RADIUS * CURVE_RADIUS + range * range - HEIGHT * HEIGHT) /
                               (2.0 * CURVE_RADIUS * range))) - (M_PI / 2.0);
    return ELEVATION;
}
