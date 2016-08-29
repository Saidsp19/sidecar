#include "Segments.h"

using std::endl;
using namespace SideCar::Messages;

// SegmentList
//
ACE_InputCDR& SegmentList::load(ACE_InputCDR& cdr)
{
    // read in the header info
    uint32_t tmp1;
    cdr >> tmp1;
    span = tmp1;
    cdr >> tmp1;
    numSegments = tmp1;

    // read in the segments
    cellCount=0;
    size_t tmp = numSegments;
    Segment s;
    while(tmp--) {
        cdr >> tmp1;
	s .azimuth = tmp1;
        cdr >> tmp1;
	s.start = tmp1;
        cdr >> tmp1;
	s.stop = tmp1;
        segments.push_back(s);
        cellCount+=s.stop-s.start+1;
    }

    return cdr;
}

ACE_OutputCDR& SegmentList::write(ACE_OutputCDR& cdr) const
{
    // write out the header info
    cdr << uint32_t(span);
    cdr << uint32_t(numSegments);

    // write out the segments
    std::list<Segment>::const_iterator s;
    std::list<Segment>::const_iterator stop=segments.end();
    for(s=segments.begin(); s!=stop; s++)
    {
        cdr << uint32_t(s->azimuth);
        cdr << uint32_t(s->start);
        cdr << uint32_t(s->stop);
    }

    return cdr;
}

std::ostream & SegmentList::print(std::ostream &os) const
{
    os  << "Span: " << span << endl
        << "Segment count: " << numSegments << endl;

    std::list<Segment>::const_iterator s;
    std::list<Segment>::const_iterator stop=segments.end();
    for(s=segments.begin(); s!=stop; s++)
    {
        os  << "segment(az=" << s->azimuth
            << ", start=" << s->start
            << ", stop=" << s->stop << ")" << endl;
    }

    return os;
}


// SegmentMessage
//
static const char *magic="SEG";
static uint32_t magic_number=*(uint32_t *)magic;

ACE_InputCDR&
SegmentMessage::load(ACE_InputCDR& cdr)
{
    uint32_t type;
    cdr >> type;
    if(type != magic_number)
        throw -1;
    cdr >> rangeMin_;
    cdr >> rangeFactor_;
    return list->load(cdr);
}

ACE_OutputCDR&
SegmentMessage::write(ACE_OutputCDR& cdr) const
{
    cdr << magic_number;
    cdr << rangeMin_;
    cdr << rangeFactor_;
    return list->write(cdr);
}

std::ostream & SegmentMessage::printData(std::ostream &os) const
{
    os << "Segment Message: rangeMin: " << rangeMin_ << " rangeFactor: "
       << rangeFactor_ << endl;
    return list->print(os);
}


MetaTypeInfo SegmentMessage::metaTypeInfo(MetaTypeInfo::Value::kSegmentMessage, "Segments",
                                          &SegmentMessage::Loader, 0);

const MetaTypeInfo & SegmentMessage::GetMetaTypeInfo()
{
    return metaTypeInfo;
}

Header::Ref SegmentMessage::Loader(ACE_InputCDR& cdr)
{
    Ref result(new SegmentMessage);
    result->load(cdr);
    return result;
}
