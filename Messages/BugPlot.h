#ifndef SIDECAR_MESSAGES_BUGPLOT_H // -*- C++ -*-
#define SIDECAR_MESSAGES_BUGPLOT_H

#include "boost/shared_ptr.hpp"

#include "IO/Printable.h"
#include "Messages/Attributes.h"
#include "Messages/Header.h"

namespace Logger { class Log; }

namespace SideCar {
namespace Messages {

class BugPlot : public Header, public IO::CDRStreamable<BugPlot>,
		public IO::Printable<BugPlot>
{
public:

    using Ref = boost::shared_ptr<BugPlot>;

    static Logger::Log& Log();

    /** Obtain the message type information for RawVideo objects.

        \return MetaTypeInfo reference
    */
    static const MetaTypeInfo& GetMetaTypeInfo();

    static Ref Make(const std::string& producer, double when, double range,
                    double azimuth, double elevation, const std::string& tag);

    static Ref Make(const QDomElement& header, const QDomElement& msg);

    /** Class factory that creates new reference-counted BugPlot message objects using data from an input CDR
	stream.

	\param cdr input CDR stream to read from

        \return reference to new RawVideo object
    */
    static Ref Make(ACE_InputCDR& cdr);

    /** Destructor.
     */
    ~BugPlot() {}

    /** Read in a BugPlot message from an CDR input stream. Override of Header::load().

        \param cdr stream to read from

        \return stream read from
    */
    ACE_InputCDR& load(ACE_InputCDR& cdr);

    /** Write out a BugPlot message to a CDR output stream. Override of Header::write().

        \param cdr stream to write to

        \return stream written to
    */
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const;

    /** Print out a textual representation of the BugPlot message. Override of Header::print().

        \param os C++ text stream to write to

        \return stream written to
    */
    std::ostream& printData(std::ostream& os) const;

    /** Print out a XML representation of the BugPlot message. Override of Header::printDataXML().

        \param os C++ text stream to write to

        \return stream written to
    */
    std::ostream& printDataXML(std::ostream& os) const;

    double getWhen() const { return when_; }

    double getRange() const { return range_; }

    double getAzimuth() const { return azimuth_; }

    double getElevation() const { return elevation_; }

    const std::string& getTag() const { return tag_; }

    size_t getSize() const { return sizeof(BugPlot); }

private:

    /** Constructor for new BugPlot message.

        \param producer name of the entity that created the message

        \param raw data block containing raw VME data.
    */
    BugPlot(const std::string& producer, ACE_Message_Block* raw);

    BugPlot(const std::string& producer);

    BugPlot(const std::string& producer, double when, double range,
            double azimuth, double elevation, const std::string& tag);

    /** Constructor for BugPlot messages that will be filled in with data from a CDR stream.
     */
    BugPlot();

    void loadXML(XmlStreamReader& xsr);

    double when_;
    double range_;
    double azimuth_;
    double elevation_;
    std::string tag_;
    Attributes attributes_;

    static Header::Ref CDRLoader(ACE_InputCDR& cdr);

    static Header::Ref XMLLoader(const std::string& producer,
                                 XmlStreamReader& xsr);

    static MetaTypeInfo metaTypeInfo_;
};


} // end namespace Messages
} // end namespace SideCar

#endif
