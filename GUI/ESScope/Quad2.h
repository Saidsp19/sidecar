#ifndef SIDECAR_GUI_ESSCOPE_QUAD2_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_QUAD2_H

#include <vector>

#include "GUI/Vertex.h"

namespace SideCar {
namespace GUI {
namespace ESScope {

struct Quad2 {
    enum { kVerticesPerQuad = 4, kValuesPerVertex = 2 };

    Quad2() {}

    Quad2(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) : x1y1(x1, y1), x2y1(x2, y1), x2y2(x2, y2), x1y2(x1, y2) {}

    Vertex x1y1, x2y1, x2y2, x1y2;
};

struct Quad2Vector : public std::vector<Quad2> {
    using Super = std::vector<Quad2>;

    Quad2Vector() : Super() {}

    Quad2Vector(size_t size) : Super(size) {}

    size_t byteSize() const { return size() * sizeof(Quad2); }

    operator void*() { return &Super::operator[](0).x1y1.x; }

    void add(const Quad2& value) { push_back(value); }
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

#endif
