#ifndef SIDECAR_GUI_COLOR_H // -*- C++ -*-
#define SIDECAR_GUI_COLOR_H

namespace SideCar {
namespace GUI {

/** Primitive color class that holds OpenGL RGB values for a specific color. NOTE: do not add any constructors,
    to this class; the CLUT class expects to initialize arrays of RGBColor objects, and at least GCC 4.0 is not
    able to do so if there is a constructor defined.
*/
struct RGBColor {
    float red, green, blue;
};

/** Utility OpenGL color class that expands on the primitive RGBColor class to hold an opacity (alpha) value.
 */
struct Color : public RGBColor {

    /** Default constructor.
     */
    Color() {}

    /** Construction from an RGBColor and opacity value

        \param rgb RGB values

        \param ta alpha value
    */
    Color(const RGBColor& rgb, float ta) : RGBColor(rgb), alpha(ta) {}

    /** Construction from individual RGB values

        \param tr red value

        \param tg green value

        \param tb blue value
    */
    Color(float tr, float tg, float tb) : alpha(1.0)
    {
        red = tr;
        green = tg;
        blue = tb;
    }

    /** Construction from individual RGB opacity values.

        \param tr red value

        \param tg green value

        \param tb blue value

        \param ta alpha value
    */
    Color(float tr, float tg, float tb, float ta) : alpha(ta)
    {
        red = tr;
        green = tg;
        blue = tb;
    }

    /** Scale existing RGB values by a given facctor.

        \param value scale factor to use

        \return reference to self
    */
    Color& operator*=(float value)
    {
        red *= value;
        green *= value;
        blue *= value;
        return *this;
    }

    /** Apply the held RGB and alpha values to the current OpenGL context.
     */
    // void use() const { glColor4f(red, green, blue, alpha); }

    float alpha;
};

} // end namespace GUI
} // end namespace SideCar

#endif
