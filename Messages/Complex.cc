#include "Complex.h"
#include "VMEHeader.h"

using namespace SideCar::Messages;

MetaTypeInfo Complex::metaTypeInfo_(MetaTypeInfo::Value::kComplex, "Complex", &Complex::CDRLoader,
                                    &Complex::XMLLoader);

const MetaTypeInfo&
Complex::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

Complex::Ref
Complex::Make(const std::string& producer, const VMEDataMessage& vme, size_t count)
{
    Ref ref(new Complex(producer, vme, count));
    return ref;
}

Complex::Ref
Complex::Make(const std::string& producer, const VMEDataMessage& vme, const DatumType* first,
              const DatumType* end)
{
    Ref ref(new Complex(producer, vme, end - first));
    Container& c(ref->getData());
    while (first != end) {
        c.push_back(*first++);
    }
    return ref;
}

Complex::Ref
Complex::Make(const std::string& producer, const VMEDataMessage& vme, const Container& data)
{
    Ref ref(new Complex(producer, vme, data));
    return ref;
}

Complex::Ref
Complex::Make(const std::string& producer, const PRIMessage::Ref& basis)
{
    Ref ref(new Complex(producer, basis));
    return ref;
}

Complex::Ref
Complex::Make(ACE_InputCDR& cdr)
{
    Ref ref(new Complex);
    ref->load(cdr);
    return ref;
}

Header::Ref
Complex::CDRLoader(ACE_InputCDR& cdr)
{
    return Make(cdr);
}

Header::Ref
Complex::XMLLoader(const std::string& producer, XmlStreamReader& xsr)
{
    Ref ref(new Complex(producer));
    ref->loadXML(xsr);
    return ref;
}

Complex::Complex()
    : Super(GetMetaTypeInfo())
{
    ;
}

Complex::Complex(const std::string& producer)
    : Super(producer, GetMetaTypeInfo())
{
    ;
}

Complex::Complex(const std::string& producer, const VMEDataMessage& vme, size_t size)
    : Super(producer, GetMetaTypeInfo(), vme, size)
{
    ;
}

Complex::Complex(const std::string& producer, const VMEDataMessage& vme, const Container& data)
    : Super(producer, GetMetaTypeInfo(), vme, data)
{
    ;
}

Complex::Complex(const std::string& producer, const PRIMessage::Ref& basis)
    : Super(producer, GetMetaTypeInfo(), basis, basis->size())
{
    ;
}
