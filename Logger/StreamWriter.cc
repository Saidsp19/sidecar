#include <iostream>
#include "Writers.h"

using namespace Logger;
using namespace Logger::Writers;

void
Stream::flush()
{
    os_.flush();
}

void
Stream::write(const Msg& msg)
{
    format(os_, msg);
    if (getFlushAfterUse()) flush();
}
