#ifndef SIDECAR_GUI_VERTEXCOLORARRAY_H // -*- C++ -*-
#define SIDECAR_GUI_VERTEXCOLORARRAY_H

#include <vector>

#include "GUI/Color.h"
#include "GUI/Vertex.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

/** Combination of Vertex and Color objects.
 */
struct VertexColor {

    VertexColor(const Vertex& v)
	: vertex(v) {}

    VertexColor(const Vertex& v, const Color& c)
	: vertex(v), color(c) {}

    Vertex vertex;
    Color color;
};

/** Resizable array of Vertex/Color pairs, interleaved in hopes of getting the best performance out of
    glVertexPointer and glColorPointer operations.
*/
class VertexColorArray 
{
public:

    /** Log device to use for VertexColorArray messages.

        \return Log device
    */
    static Logger::Log& Log();

    /** Constructor

        \param capacity initial capacity for the vector. 
    */
    VertexColorArray();

    bool empty() const { return vertices_.empty(); }

    void clear() { vertices_.clear(); }

    /** Invoke glDrawArrays with the given primitive type.

        \param type the primitive to draw
    */
    void draw(int type) const;

    /** Number of Vertex/Color pairs present in the array.

        \return array size
    */
    size_t size() const { return vertices_.size(); }

    /** Check if the given request can be handled by the current capacity of the array. If expansion is
        perfomed, this will invoke glVertexPointer and glColorPointer to use the new pointer values. NOTE:
        because of this side-effect, this method must be called inside a valid OpenGL context.

        \param request number of VertexColor objects to store
    */
    void checkCapacity(size_t request);

    /** Append a new VertexColor object with the given values

        \param vertex Vertex object to store

        \param color Color object to store
    */
    void push_back(const Vertex& vertex)
	{ vertices_.push_back(VertexColor(vertex)); }

    /** Append a new VertexColor object with the given values

        \param vertex Vertex object to store

        \param color Color object to store
    */
    void push_back(const Vertex& vertex, const Color& color)
	{ vertices_.push_back(VertexColor(vertex, color)); }

    VertexColor& operator[](size_t index)
	{ return vertices_[index]; }

private:
    std::vector<VertexColor> vertices_;
    size_t stride_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
