#ifndef SIDECAR_GUI_VERTEXGENERATOR_H // -*- C++ -*-
#define SIDECAR_GUI_VERTEXGENERATOR_H

#include <deque>
#include <vector>

#include "GUI/VertexColorArray.h"
#include "Messages/PRIMessage.h"

namespace SideCar {
namespace GUI {

class OffscreenBuffer;

using MessageVector = std::vector<Messages::PRIMessage::Ref>;

/** Abstract base class that defines the interface for classes that generate Vertex and Color values from
    Messages::PRIMessage data. Instances contain a FIFO queue of Messages::PRIMessage references, which gets
    processes by the processQueue() method, the result of which are Vertex and Color values appended to the
    internal VertexColorArray container. These values may then get drawn on an OffscreenBuffer object using the
    renderInto() method. The rest of the API provides means to query and clear the message queue and point
    container.
*/
class VertexGenerator {
public:
    VertexGenerator();

    virtual ~VertexGenerator();

    void add(const Messages::PRIMessage::Ref& msg);

    void add(const MessageVector& msgs);

    void flushQueue() { queue_.clear(); }

    void flushPoints() { points_.clear(); }

    void flushAll()
    {
        flushQueue();
        flushPoints();
    }

    bool isFinished() const { return queue_.empty(); }

    bool hasData() const { return !queue_.empty(); }

    void processQueue(int limit = 100);

    bool hasPoints() const { return !points_.empty(); }

    void renderInto(OffscreenBuffer* buffer);

protected:
    virtual void renderMessage(const Messages::PRIMessage::Ref& msg, VertexColorArray& points) = 0;

private:
    using MessageQueue = std::deque<Messages::PRIMessage::Ref>;
    MessageQueue queue_;
    VertexColorArray points_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
