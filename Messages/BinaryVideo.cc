#include "Logger/Log.h"

#include "BinaryVideo.h"

using namespace SideCar::Messages;

MetaTypeInfo BinaryVideo::metaTypeInfo_(MetaTypeInfo::Value::kBinaryVideo, "BinaryVideo", &BinaryVideo::CDRLoader,
                                        &BinaryVideo::XMLLoader);

const MetaTypeInfo&
BinaryVideo::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

BinaryVideo::Ref
BinaryVideo::Make(const std::string& producer, const VMEDataMessage& vme, size_t count)
{
    Ref ref(new BinaryVideo(producer, vme, count));
    return ref;
}

BinaryVideo::Ref
BinaryVideo::Make(const std::string& producer, const Video::Ref& video)
{
    Ref ref(new BinaryVideo(producer, video));
    return ref;
}

BinaryVideo::Ref
BinaryVideo::Make(const std::string& producer, const BinaryVideo::Ref& binaryVideo)
{
    Ref ref(new BinaryVideo(producer, binaryVideo));
    return ref;
}

BinaryVideo::Ref
BinaryVideo::Make(ACE_InputCDR& cdr)
{
    Ref ref(new BinaryVideo);
    ref->load(cdr);
    return ref;
}

BinaryVideo::Ref
BinaryVideo::Make(const std::string& producer, const VMEDataMessage& vme, const DatumType* first, const DatumType* end)
{
    Ref ref(new BinaryVideo(producer, vme, end - first));
    Container& c(ref->getData());
    while (first != end) c.push_back(*first++);
    return ref;
}

Header::Ref
BinaryVideo::CDRLoader(ACE_InputCDR& cdr)
{
    return Make(cdr);
}

Header::Ref
BinaryVideo::XMLLoader(const std::string& producer, XmlStreamReader& xsr)
{
    Ref ref(new BinaryVideo(producer));
    ref->loadXML(xsr);
    return ref;
}

BinaryVideo::BinaryVideo() : Super(GetMetaTypeInfo())
{
    ;
}

BinaryVideo::BinaryVideo(const std::string& producer, const VMEDataMessage& vme, size_t size) :
    Super(producer, GetMetaTypeInfo(), vme, size)
{
    ;
}

BinaryVideo::BinaryVideo(const std::string& producer, const Video::Ref& basis) :
    Super(producer, GetMetaTypeInfo(), basis, basis->size())
{
    ;
}

BinaryVideo::BinaryVideo(const std::string& producer, const BinaryVideo::Ref& basis) :
    Super(producer, GetMetaTypeInfo(), basis, basis->size())
{
    ;
}

BinaryVideo::BinaryVideo(const std::string& producer) : Super(producer, GetMetaTypeInfo())
{
    ;
}

BinaryVideo::~BinaryVideo()
{
    ;
}

Video::Ref
BinaryVideo::getVideoBasis() const
{
    static Logger::ProcLog log("getVideoBasis", Log());
    Header::Ref basis(getBasis());
    Video::Ref video;
    while (basis) {
        LOGDEBUG << basis.get() << std::endl;
        if (basis->getMetaTypeInfo().isa(Messages::MetaTypeInfo::Value::kVideo)) {
            video = boost::dynamic_pointer_cast<Video>(basis);
            break;
        }
        basis = basis->getBasis();
    }

    return video;
}
