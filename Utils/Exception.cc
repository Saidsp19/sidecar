#include "Utils/Exception.h"

using namespace Utils;

Exception::Exception(const char* err) :
    std::exception(), os_(nullptr), err_(err ? err : "")
{
    if (!err) { throw std::invalid_argument("NULL pointer for exception text"); }
}

Exception::Exception(const Exception& copy) : std::exception(), os_(nullptr), err_(copy.err_)
{
    // If the given instance has an active ostringstream, take its contents as our err text.
    //
    if (copy.os_.get()) err_ = copy.os_->str();
}

Exception::~Exception()
{
    ;
}

Exception&
Exception::operator=(const Utils::Exception& rhs)
{
    if (&rhs == this) return *this;

    // Dispose of any existing converter stream
    //
    os_.reset(nullptr);

    // If other object has a converter stream, take a copy of its contents. Otherwise, just take the other
    // object's current error text.
    //
    if (rhs.os_.get()) {
        err_ = rhs.os_->str();
    } else {
        err_ = rhs.err_;
    }

    return *this;
}

const std::string&
Exception::err() const
{
    // If we have a converter stream, save a copy of its contents and then dispose of it.
    //
    if (os_.get()) {
        err_ = os_->str();
        os_.reset(nullptr);
    }

    return err_;
}

const char*
Exception::what() const noexcept
{
    return err().c_str();
}

std::ostream&
Exception::os()
{
    if (!os_.get()) {
        os_.reset(new std::ostringstream(err_, std::ios_base::out | std::ios_base::app | std::ios_base::ate));
        err_ = "";
    }
    return *os_;
}
