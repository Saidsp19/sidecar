// -*- C++ -*-

namespace SideCar {
namespace GUI {

class DisableAppNap
{
    struct Impl;
public:
    DisableAppNap();

    ~DisableAppNap();

    void begin();
    void end();

private:
    Impl* impl_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */
