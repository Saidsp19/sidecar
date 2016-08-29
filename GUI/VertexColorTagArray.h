#ifndef SIDECAR_GUI_VERTEXCOLORTAGARRAY_H // -*- C++ -*-
#define SIDECAR_GUI_VERTEXCOLORTAGARRAY_H

#include <vector>

#include "QtCore/QString"

#include "GUI/Color.h"
#include "GUI/Vertex.h"

namespace SideCar {
namespace GUI {

/** Combination of Vertex and Color objects.
 */
struct VertexColorTag {

    VertexColorTag(const Vertex& v, const Color& c, const QString& t)
	: vertex(v), color(c), tag(t) {}

    Vertex vertex;
    Color color;
    QString tag;
};

/** Resizable array of Vertex/Color/QString tuple, interleaved in hopes of getting the best performance out of
    glVertexPointer and glColorPointer operations.
*/
class VertexColorTagArray 
{
public:

    VertexColorTagArray() : vertices_() {}

    bool empty() const { return vertices_.empty(); }

    /** Number of Vertex/Color pairs present in the array.

        \return array size
    */
    size_t size() const { return vertices_.size(); }

    /** Append a new VertexColorTag object with the given values

        \param vertex Vertex object to store

        \param color Color object to store

	\param tag text of the tag to store
    */
    void push_back(const Vertex& vertex, const Color& color,
                   const QString& tag)
	{ vertices_.push_back(VertexColorTag(vertex, color, tag)); }

    const VertexColorTag& operator[](size_t index) const
	{ return vertices_[index]; }

private:
    std::vector<VertexColorTag> vertices_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
