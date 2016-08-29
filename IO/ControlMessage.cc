#include "ControlMessage.h"
#include "MessageManager.h"

using namespace SideCar::IO;

ControlMessage::ControlMessage(Type type, size_t size)
    : data_(MessageManager::MakeControlMessage(type, size))
{
    ;
}
