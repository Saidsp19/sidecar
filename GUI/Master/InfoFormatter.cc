#include "QtCore/QLibrary"

#include "GUI/LogUtils.h"

#include "InfoFormatter.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

InfoFormatter::InfoFormatterHash InfoFormatter::formatters_;

Logger::Log&
InfoFormatter::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.InfoFormatter");
    return log_;
}

InfoFormatter*
InfoFormatter::Find(const QString& name)
{
    static Logger::ProcLog log("Find", Log());
    LOGINFO << name << std::endl;

    // See if there is already a formatter registered, one that is currently in use. If we have the only copy,
    // then reload it just in case a developer has compiled and installed a newer version.
    //
    InfoFormatterHash::const_iterator pos = formatters_.find(name);
    if (pos != formatters_.end()) {
        InfoFormatter* obj = pos.value();
        obj->useCount_ += 1;
        return obj;
    }

    // Load in the library and get the routine called FormatInfo.
    //
    QString libName = QString("lib%1").arg(name);
    QLibrary library(libName);
    Formatter formatter = Formatter(library.resolve("FormatInfo"));
    InfoFormatter* obj = new InfoFormatter(name, formatter);
    formatters_[name] = obj;

    return obj;
}

InfoFormatter::InfoFormatter(const QString& name, Formatter formatter) :
    name_(name), formatter_(formatter), useCount_(1)
{
    Logger::ProcLog log("InfoFormatter", Log());
    LOGINFO << name << std::endl;
}

QVariant
InfoFormatter::format(const IO::StatusBase& status, int role)
{
    if (!formatter_) return QVariant();
    char* value = (*formatter_)(status, role);
    if (!value) return QVariant();
    QString tmp(value);
    delete[] value;
    return tmp;
}

void
InfoFormatter::release()
{
    if (--useCount_ == 0) {
        formatters_.remove(name_);
        delete this;
    }
}
