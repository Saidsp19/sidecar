#include <algorithm>

#include "Video.h"
#include "VMEHeader.h"

using namespace SideCar::Messages;

MetaTypeInfo Video::metaTypeInfo_(MetaTypeInfo::Value::kVideo, "Video", &Video::CDRLoader, &Video::XMLLoader);

const MetaTypeInfo&
Video::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

Video::Ref
Video::Make(const std::string& producer, const VMEDataMessage& vme, size_t count)
{
    Ref ref(new Video(producer, vme, count));
    return ref;
}

Video::Ref
Video::Make(const std::string& producer, const VMEDataMessage& vme, const DatumType* first, const DatumType* end)
{
    Ref ref(new Video(producer, vme, end - first));
    Container& c(ref->getData());

    // !!! TODO: measure best way to initialize
    // c.resize(end - first, 0);
    // std::copy(first, end, c.begin());
    while (first != end) c.push_back(*first++);
    return ref;
}

Video::Ref
Video::Make(const std::string& producer, const VMEDataMessage& vme, const Container& data)
{
    Ref ref(new Video(producer, vme, data));
    return ref;
}

Video::Ref
Video::Make(const std::string& producer, const PRIMessage::Ref& basis)
{
    Ref ref(new Video(producer, basis));
    return ref;
}

Video::Ref
Video::Make(ACE_InputCDR& cdr)
{
    Ref ref(new Video);
    ref->load(cdr);
    return ref;
}

Header::Ref
Video::CDRLoader(ACE_InputCDR& cdr)
{
    return Make(cdr);
}

Header::Ref
Video::XMLLoader(const std::string& producer, XmlStreamReader& xsr)
{
    Ref ref(new Video(producer));
    ref->loadXML(xsr);
    return ref;
}

Video::Video(const std::string& producer, const VMEDataMessage& vme, size_t size)
    : Super(producer, GetMetaTypeInfo(), vme, size)
{
    ;
}

Video::Video(const std::string& producer, const VMEDataMessage& vme, const Container& data)
    : Super(producer, GetMetaTypeInfo(), vme, data)
{
    ;
}

Video::Video(const std::string& producer, const PRIMessage::Ref& basis)
    : Super(producer, GetMetaTypeInfo(), basis, basis->size())
{
    ;
}

Video::Video(const std::string& producer)
    : Super(producer, GetMetaTypeInfo())
{
    ;
}

Video::Video()
    : Super(GetMetaTypeInfo())
{
    ;
}

Video::~Video()
{
    ;
}
