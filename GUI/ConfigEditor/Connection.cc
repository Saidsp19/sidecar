#include "Connection.h"
#include "ChannelItem.h"

using namespace SideCar::GUI::ConfigEditor;

Connection::Connection(ChannelItem* from, ChannelItem* to) :
    from_(from), to_(to), fromIndex_(from->addConnection(this)), toIndex_(to->addConnection(this))
{
    ;
}

Connection::~Connection()
{
    from_->removeConnection(fromIndex_);
    to_->removeConnection(toIndex_);
}
