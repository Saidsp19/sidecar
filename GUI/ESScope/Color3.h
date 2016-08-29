#ifndef SIDECAR_GUI_ESSCOPE_COLOR3_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_COLOR3_H

#include <vector>

#include "GUI/Color.h"

#include "Quad2.h"

namespace SideCar {
namespace GUI {
namespace ESScope {

struct Color3 
{
    enum {
	kValuesPerColor = 3
    };

    Color3() : red(0.0), green(0.0), blue(0.0) {}

    Color3(GLfloat r, GLfloat g, GLfloat b)
	: red(r), green(g), blue(b) {}

    Color3(const Color& color)
	: red(color.red), green(color.green), blue(color.blue) {}

    GLfloat red, green, blue;
};

struct QuadColor
{
    QuadColor() {}

    QuadColor(const Color3& color)
	: x1y1(color), x2y1(color), x2y2(color), x1y2(color) {}

    Color3 x1y1, x2y1, x2y2, x1y2;
};

struct QuadColorVector : public std::vector<QuadColor>
{
    using Super = std::vector<QuadColor>;

    int GetByteOffset(int vertexOffset)
	{ return vertexOffset * sizeof(QuadColor); }

    QuadColorVector() : Super() {}

    QuadColorVector(size_t size) : Super(size) {}

    QuadColorVector(size_t size, const QuadColor init)
	: Super(size, init) {}

    size_t byteSize() const
	{ return size() * sizeof(QuadColor); }

    operator void*()
	{ return &Super::operator[](0).x1y1.red; }

    void add(const Color3& value)
	{ push_back(QuadColor(value)); }

    void add(const QuadColor& value)
	{ push_back(value); }
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

#endif
