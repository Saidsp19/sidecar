#include "MessageType.h"

using namespace SideCar::GUI::ConfigEditor;

MessageType::HashTable MessageType::hash_;

MessageType*
MessageType::Make(const QString& name)
{
    if (hash_.contains(name))
	return 0;
    return new MessageType(name);
}

MessageType*
MessageType::Find(const QString& name)
{
    HashTable::iterator pos = hash_.find(name);
    return pos == hash_.end() ? 0 : pos.value();
}

MessageType::MessageType(const QString& name)
    : name_(name)
{
    hash_[name] = this;
}
