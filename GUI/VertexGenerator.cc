#include <iterator>

#include "OffscreenBuffer.h"
#include "VertexGenerator.h"

using namespace SideCar::GUI;

VertexGenerator::VertexGenerator() : queue_(), points_()
{
    ;
}

VertexGenerator::~VertexGenerator()
{
    ;
}

void
VertexGenerator::add(const Messages::PRIMessage::Ref& msg)
{
    queue_.push_back(msg);
}

void
VertexGenerator::add(const MessageVector& msgs)
{
    flushQueue();
    std::copy(msgs.begin(), msgs.end(), std::back_inserter<>(queue_));
}

void
VertexGenerator::processQueue(int limit)
{
    // Change the limit value with knowledge of the queue size.
    //
    if (limit == -1 || limit > static_cast<int>(queue_.size())) { limit = queue_.size(); }

    while (limit--) {
        renderMessage(queue_.front(), points_);
        queue_.pop_front();
    }
}

void
VertexGenerator::renderInto(OffscreenBuffer* buffer)
{
    buffer->renderPoints(points_);
    points_.clear();
}
