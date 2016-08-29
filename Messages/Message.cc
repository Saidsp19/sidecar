#include "Message.h"
#include "MetaTypeInfo.h"

using namespace SideCar::Messages;

MetaTypeInfo ShortSequence::metaTypeInfo_(
    MetaTypeInfo::kShortSequence,
    "ShortSequence",
    0,
    0,
    0
    );

const MetaTypeInfo&
ShortSequence::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

MetaTypeInfo IQSequence::metaTypeInfo_(
    MetaTypeInfo::kIQSequence,
    "IQSequence",
    0,
    0,
    0
    );

const MetaTypeInfo&
IQSequence::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

MetaTypeInfo BooleanSequence::metaTypeInfo_(
    MetaTypeInfo::kIQSequence,
    "BooleanSequence",
    0,
    0,
    0
    );

const MetaTypeInfo&
BooleanSequence::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}
