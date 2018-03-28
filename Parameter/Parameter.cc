#include "Logger/Log.h"
#include "Utils/IO.h"

#include "Parameter.h"

using namespace SideCar::Parameter;

InvalidValue::InvalidValue(const std::string& routine, const std::string& name) : Utils::Exception(routine)
{
    *this << ": invalid value for parameter '" << name << "' - ";
}

Logger::Log&
Defs::XMLMixin::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Parameter.XMLMixin");
    return log_;
}

void
Defs::XMLMixin::Init(XmlRpc::XmlRpcValue& xml, const std::string& name, const std::string& label,
                     const std::string& type, bool isAdvanced)
{
    Logger::ProcLog log("Init", Log());
    LOGDEBUG << name << ' ' << type << std::endl;
    xml.clear();
    xml["name"] = name;
    xml["label"] = label;
    xml["type"] = type;
    xml["advanced"] = isAdvanced;
}

void
Defs::XMLMixin::AddEnumNames(XmlRpc::XmlRpcValue& xml, const char* const* names, size_t size)
{
    Logger::ProcLog log("AddEnumNames", Log());
    LOGINFO << "size: " << size << std::endl;
    XmlRpc::XmlRpcValue valueNames;
    valueNames.setSize(size);
    for (auto index = 0; index < size; ++index) {
        LOGDEBUG << names[index] << std::endl;
        valueNames[index] = names[index];
    }
    xml["enumNames"] = valueNames;
}

void
Defs::XMLMixin::AddEnumNames(XmlRpc::XmlRpcValue& xml, const std::vector<std::string>& names)
{
    Logger::ProcLog log("AddEnumNames", Log());
    LOGINFO << "size: " << names.size() << std::endl;
    XmlRpc::XmlRpcValue valueNames;
    valueNames.setSize(names.size());
    for (auto index = 0; index < names.size(); ++index) {
        LOGDEBUG << names[index] << std::endl;
        valueNames[index] = names[index];
    }
    xml["enumNames"] = valueNames;
}

bool
Defs::StringBase::Load(std::istream& is, std::string& value)
{
    if (!(is >> value)) return false;
    if (value.size() && (value[0] == '"' || value[0] == '\'')) {
        auto delimiter = value[0];
        value.erase(0, 1);
        char c;
        while ((is.get(c)) && c != delimiter) { value += c; }
    }

    return true;
}

bool
Defs::StringBase::Load(ACE_InputCDR& cdr, std::string& value)
{
    cdr >> value;
    return true;
}

std::ostream&
Defs::StringBase::Save(std::ostream& os, const std::string& name, const std::string& value)
{
    return os << name << " '" << value << '\'';
}
