#include <stdexcept>
#include <sstream>

#include "Utils/Wrapper.h"

using namespace Utils;

Wrapper::Wrapper(const std::string& data, int width, int cursor, const std::string& prefix)
    : data_(data), prefix_(prefix), width_(width), cursor_(cursor)
{
    if (width < 1) throw std::runtime_error("Wrapper: width");
}

std::ostream&
Wrapper::print(std::ostream& os) const
{
    // Split the string to wrap into words using istringstream
    //
    std::istringstream is(data_);
    std::string word;
    auto cursor = cursor_;

    // Read words until string has been processed
    //
    while (is >> word) {

	// We will write out a word + a space. Can we?
	//
	auto size = word.size() + 1;
	if (cursor && size + cursor >= width_) {

	    // Write out newline + continuation prefix
	    //
	    os << '\n' << prefix_;
	    cursor = prefix_.size();
	}

	// Write out word + space
	//
	os << word << ' ';
	cursor += size;
    }

    return os;
}
