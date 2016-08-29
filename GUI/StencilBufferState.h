#ifndef SIDECAR_GUI_STENCILBUFFERSTATE_H // -*- C++ -*-
#define SIDECAR_GUI_STENCILBUFFERSTATE_H

namespace SideCar {
namespace GUI {

/** Utility class that turns on Open/GL stencil buffer tesing only if it is used, and disables it when
    destructed.
*/
class StencilBufferState
{
public:

    /** Constructor. Assumes that GL_STENCIL_TEST is not enabled.
     */
    StencilBufferState() : using_(false) {}

    /** Destructor. Disables GL_STENCIL_TEST if it had been enabled by an earlier use() call.
     */
    ~StencilBufferState();

    /** Enable the stencil test if necessary, and clear the stencil buffer.
     */
    void use();

private:
    bool using_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
