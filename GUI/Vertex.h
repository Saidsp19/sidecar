#ifndef SIDECAR_GUI_VERTEX_H // -*- C++ -*-
#define SIDECAR_GUI_VERTEX_H


#include <vector>

#include "QtOpenGL/QtOpenGL"

namespace SideCar {
namespace GUI {

struct Vertex
{
    Vertex() {}

    Vertex(GLfloat tx, GLfloat ty) : x(tx), y(ty) {}

    Vertex& operator*=(float rhs)
	{
	    x *= rhs;
	    y *= rhs;
	    return *this;
	}

    Vertex& operator+=(const Vertex& rhs)
	{
	    x += rhs.x;
	    y += rhs.y;
	    return *this;
	}

    GLfloat x, y;
};

inline Vertex
operator*(const Vertex& lhs, float rhs)
{ return Vertex(lhs.x * rhs, lhs.y * rhs); }

inline Vertex
operator+(const Vertex& lhs, const Vertex& rhs)
{ return Vertex(lhs.x + rhs.x, lhs.y + rhs.y); }

using VertexVector = std::vector<Vertex>;

} // end namespace GUI
} // end namespace SideCar

#endif
