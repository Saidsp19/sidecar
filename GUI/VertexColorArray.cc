#include "QtOpenGL/QGLWidget"

#include "VertexColorArray.h"
#include "LogUtils.h"

using namespace SideCar::GUI;

Logger::Log&
VertexColorArray::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.VertexColorArray");
    return log_;
}

VertexColorArray::VertexColorArray() : vertices_(), stride_(0)
{
    static Logger::ProcLog log("VertexColorArray", Log());
    vertices_.reserve(4096);
    vertices_.push_back(Vertex(0, 0));
    vertices_.push_back(Vertex(0, 0));
    stride_ = reinterpret_cast<char*>(&vertices_[1]) - reinterpret_cast<char*>(&vertices_[0]);
    LOGINFO << "stride: " << stride_ << " sizeof(VertexColor): " << sizeof(VertexColor) << std::endl;
    vertices_.clear();
}

void
VertexColorArray::checkCapacity(size_t request)
{
    static Logger::ProcLog log("checkCapacity", Log());
    LOGINFO << "request: " << request << " capacity: " << vertices_.capacity() << " size: " << vertices_.size()
            << std::endl;

    // Verify that we can handle the request.
    //
    request += vertices_.size();
    size_t capacity = vertices_.capacity();
    if (request > capacity) {
        // If we have no capacity, resize to just handle the request.
        //
        if (capacity == 0) {
            capacity = request;
        } else {
            // Double capacity until it handles the size.
            //
            while (request > capacity) capacity *= 2;
        }

        LOGDEBUG << "expanding: " << request << " old capacity: " << vertices_.capacity()
                 << " new capacity: " << capacity << std::endl;

        vertices_.reserve(capacity);
    }
}

void
VertexColorArray::draw(int type) const
{
    Logger::ProcLog log("draw", Log());

    // Update the GL context to use any new pointer values resulting from the resizing. The stride value is the
    // number of bytes between successive entries in the array. This number is the same for the vertex and color
    // pointers; it is their respective starting positions that differ.
    //
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glVertexPointer(2, GL_FLOAT, stride_, &vertices_[0].vertex);
    glColorPointer(4, GL_FLOAT, stride_, &vertices_[0].color);
    glDrawArrays(type, 0, vertices_.size());
    glPopClientAttrib();
}
