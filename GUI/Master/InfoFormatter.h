#ifndef SIDECAR_GUI_INFOFORMATTER_H // -*- C++ -*-
#define SIDECAR_GUI_INFOFORMATTER_H

#include "QtCore/QHash"
#include "QtCore/QString"
#include "QtCore/QVariant"

namespace Logger { class Log; }

namespace SideCar {
namespace IO { class StatusBase; }
namespace GUI {
namespace Master {

/** The InfoFormatter class represents a formatter for the 'info' status field of Algorithm/Controller objects.
    One cannot create InfoFormatter objects directly, rather one must call the Find() class method in order to
    obtain an InfoFormatter object for a given name. If the object does not exist, the Find() method will create
    a new one and register it with the class formatters_ attribute, Otherwise, Find() returns the
    already-registered object.
*/
class InfoFormatter
{
public:

    /** Obtain log device for InfoFormatter objects

        \return log device reference
    */
    static Logger::Log& Log();

    /** Locate or create an InfoFormatter object with the given name. Uses the QLibrary class to load a DLL with
        the name 'lib' + name. It then tries to resolve the symbol 'FormatInfo' in the loaded DLL.

        \param name the name of the formatter to locate

        \return reference to InfoFormatter object
    */
    static InfoFormatter* Find(const QString& name);

    /** Obtain the name for the InfoFormatter object

        \return formatter name
    */
    const QString& getName() const { return name_; }

    /** Obtain formatting information for status information in a IO::StatusBase object.

        \param status 

        \param role 

        \return 
    */
    QVariant format(const IO::StatusBase& status, int role);

    void release();

private:

    using Formatter = char* (*)(const IO::StatusBase&, int);

    InfoFormatter(const QString& name, Formatter formatter);

    QString name_;
    Formatter formatter_;
    size_t useCount_;

    using InfoFormatterHash = QHash<QString,InfoFormatter*>;
    static InfoFormatterHash formatters_; 
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
