#ifndef SIDECAR_MESSAGES_XMLSTREAMREADER_H // -*- C++ -*-
#define SIDECAR_MESSAGES_XMLSTREAMREADER_H
// (C) Copyright 2009 Massachusetts Institute of Technology. All rights reserved.
//

#include "QtCore/QString"
#include "QtCore/QStringList"
#include "QtCore/QXmlStreamAttributes"
#include "QtCore/QXmlStreamReader"

namespace SideCar {
namespace Messages {

/** Variant of Qt QXmlStreamReader class that encapsulates some common SideCar uses. Also solves a linking
    problem on MacOS X due to some crufty symbol renaming going on by Trolltech/Nokia (I don't think they'll be
    doing THAT again, given all of the complaints this caused...)
*/
class XmlStreamReader : public QXmlStreamReader
{
public:

    /** Constructor. Initializes instance, taking data from the given QIODevice object.

        \param file the source of the data
    */
    XmlStreamReader(QIODevice* device);

    /** Obtain the attribute value for the current entity with a given name.

        \param name the name of the attribute to look for

        \return value found. NOTE: QString::isNull() will return true if the
        attribute did not exist, while QString::isEmpty() will return true if
        the attribute was set with an empty value ("").
    */
    QString getAttribute(const QString& name) const;

    /** Read the stream until the next entity start.

        \param expected the name of the entity to expect

        \return true if the expected entity was found; false otherwise
    */
    bool readNextEntityAndValidate(const QString& expected);

    /** Obtain a list of sample values from the current entity.

        \return QStringList
    */
    QStringList getSamples();
};

} // end namespace Messages
} // end namespace SideCar

#endif
