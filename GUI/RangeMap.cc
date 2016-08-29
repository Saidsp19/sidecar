#include "QtOpenGL/QGLWidget"

#include "Messages/RadarConfig.h"
#include "Utils/Utils.h"

#include "RangeMap.h"

using namespace SideCar::GUI;

struct LatLon
{
    double lat;
    double lon;
};

/** TODO: this needs to be loaded from a file...
*/
const RangeMap::LatLon RangeMap::map_[] = {
    { 0.0,0.0 },
    { 0.0,0.0 },
    { 0.0,0.0 },
    { 0.0,0.0 },
};

RangeMap::RangeMap()
    : vertices_()
{
    ;
}

void
RangeMap::makeVertex(const LatLon& latLon, Vertex& vertex)
{
    static const double rlat = Messages::RadarConfig::GetSiteLatitude();
    static const double rlon = Messages::RadarConfig::GetSiteLongitude();
    vertex.x = (latLon.lon - rlon) * 111.0 * ::cos(M_PI / 180.0 * latLon.lat);
    vertex.y = (latLon.lat - rlat) * 111.0;
}

void
RangeMap::addVertex(const Vertex& vertex)
{
    vertices_.push_back(vertex);
}

void
RangeMap::generateMap()
{
    Vertex v;
    for (auto index = 0; index < sizeof(map_) / sizeof(LatLon); ++index) {
	makeVertex(map_[index], v);
	addVertex(v);
    }
}

void
RangeMap::render()
{
    if (vertices_.empty())
	generateMap();

    glBegin(GL_LINE_STRIP);
    for (auto index = 0; index < vertices_.size(); ++index) {
	glVertex2f(vertices_[index].x, vertices_[index].y);
    }
    glEnd();
}
