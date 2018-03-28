#include "QtCore/QFile"

#include "XmlStreamReader.h"

using namespace SideCar::Messages;

XmlStreamReader::XmlStreamReader(QIODevice* device) : QXmlStreamReader(device)
{
    ;
}

QString
XmlStreamReader::getAttribute(const QString& name) const
{
    return attributes().value(name).toString();
}

bool
XmlStreamReader::readNextEntityAndValidate(const QString& expected)
{
    while (!atEnd() && readNext() != QXmlStreamReader::StartElement)
        ;

    return !atEnd() && name().toString() == expected;
}

QStringList
XmlStreamReader::getSamples()
{
    return readElementText().split(' ', QString::SkipEmptyParts);
}
