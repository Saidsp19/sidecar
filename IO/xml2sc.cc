#include <cmath>
#include <iostream>
#include <string>

#include "IO/FileWriterTask.h"
#include "IO/MessageManager.h"
#include "Logger/Log.h"
#include "Utils/CmdLineArgs.h"
#include "Utils/FilePath.h"
#include "Utils/Utils.h"

#include "Messages/XmlStreamReader.h"

#include "QtCore/QFile"

using namespace SideCar;

const std::string about = "Convert XML messages into SideCar binary messages. "
    "Time slicing option values T0 and T1 may be specified in either relative "
    "or absolute values (GMT). Relative values start with a '+' followed by "
    "seconds or minutes:seconds. Absolute times start with a digit and must "
    "be hours:minutes:seconds.";

const Utils::CmdLineArgs::OptionDef opts[] = {
    { 'D', "debug", "enable root debug level", 0 },
    { 0, "t0", "begin processing at time T0", "T0" },
    { 0, "t1", "end processing at time T1", "T1" },
    { 'c', "count", "only process COUNT records", "COUNT" },
};

const Utils::CmdLineArgs::ArgumentDef args[] = {
    { "IN", "path to input file" },
    { "OUT", "path to output file" },
};

int
main(int argc, char** argv)
{
    Utils::CmdLineArgs cla(argc, argv, about, opts, sizeof(opts), args, sizeof(args));
    if (cla.hasOpt("debug"))
	Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    Utils::FilePath inputPath(cla.arg(0));
    if (! inputPath.exists()) {
	std::cerr << "*** file '" << inputPath << "' does not exists\n";
	return 1;
    }

    // Open the XML file
    //
    QFile file(inputPath.c_str());
    if (! file.open(QFile::ReadOnly | QFile::Text)) {
	std::cerr << "*** failed to open file '" << inputPath << "'\n";
	return 1;
    }

    // Create a new FileWriterTask object.
    //
    std::string outputPath = cla.arg(1);
    IO::FileWriterTask::Ref writer(IO::FileWriterTask::Make());
    if (! writer->openAndInit("Video", outputPath)) {
	std::cerr << "*** failed to open output file '" << cla.arg(1) << "'" << std::endl;
	return 1;
    }

    // Load the XML data.
    //
    Messages::XmlStreamReader xsr(&file);
    Messages::MetaTypeInfo::XMLLoader loader = 0;
    std::string producer;
    size_t counter = 0;
    
    while (! xsr.atEnd()) {

	QString elementName;
	switch (xsr.readNext()) {

	case QXmlStreamReader::StartElement:

	    elementName = xsr.name().toString();
	    if (elementName == "group") {

		producer = xsr.getAttribute("producer").toStdString();
		std::string type = xsr.getAttribute("type").toStdString();

		const Messages::MetaTypeInfo* metaTypeInfo = Messages::MetaTypeInfo::Find(type);
		if (! metaTypeInfo) {
		    std::cerr << "*** unknown message type '" << type << "'\n";
		    return 1;
		}

		std::clog << "... processing '" << type << "' messages from '" << producer << "'\n";
		loader = metaTypeInfo->getXMLLoader();
	    }
	    else if (elementName == "msg") {
		if (! loader) {
		    std::cerr << "*** invalid XML file" << std::endl;
		    return 1;
		}

		Messages::Header::Ref native = loader(producer, xsr);
		if (! native) {
		    std::cerr << "*** failed to create SideCar message from XML\n";
		    return 1;
		}

		++counter;
		IO::MessageManager mgr(native);
		if (writer->put(mgr.getMessage()) == -1) {
		    std::cerr << "*** failed to put message to writer task\n";
		    return 1;
		}
	    }
	    else if (elementName != "file") {
		std::clog << "... ignoring element '" << elementName.toStdString() << '\n';
	    }
	    break;

	case QXmlStreamReader::EndElement:
	case QXmlStreamReader::Characters:
	case QXmlStreamReader::Comment:
	case QXmlStreamReader::DTD:
	case QXmlStreamReader::EntityReference:
        default:
	    break;
	}
    }

    writer->close(1);
    std::clog << "... created " << counter << " messages\n";

    return 0;
}
