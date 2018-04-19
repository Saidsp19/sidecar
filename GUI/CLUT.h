#ifndef SIDECAR_GUI_CLUT_H // -*- C++ -*-
#define SIDECAR_GUI_CLUT_H

#include <algorithm>
#include <cstddef>

#include "GUI/Color.h"

class QComboBox;
class QImage;
class QString;

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

/** General color lookup table class. Instances provide via getColor(double) a mapping from intensity values in
    the range [0.0-1.0] into RGB values stored in a Color object. There are kNumTypes predefined tables with
    different color ramps. The setType() method selects which table a CLUT object uses.
*/
class CLUT {
public:
    enum Type {
        kBlueSaturated,
        kBone,
        kCool,
        kCopper,
        kGray,
        kGreenSaturated,
        kHot,
        kHSV,
        kJet,
        kPink,
        kRedSaturated,
        kRGB655,
        kRGB565,
        kRGB556,
        kNumTypes
    };

    /** Obtain the log device for CLUT objects

        \return Log device
    */
    static Logger::Log& Log();

    /** Obtain the display name for a given CLUT::Type value.

        \param type predefined CLUT to work with

        \return type name
    */
    static const QString& GetTypeName(Type type);

    /** Obtain the number of entries in each predefined table.

        \param type predefined CLUT to work with

        \return CLUT size
    */
    static size_t GetSize(Type type);

    /** Obtain the display name for a given CLUT::Type value.

        \param type value to convert

        \return type name
    */
    static bool HasSaturation(Type type);

    static void AddTypeNames(QComboBox* widget);

    /** Constructor.

        \param type initial lookup table to use

        \param alpha initial alpha value to use
    */
    CLUT(Type type, float alpha = 1.0);

    /** Change the lookup table use by the CLUT object

        \param type new table index
    */
    void setType(Type type);

    /** Obtain the curent table index used by the CLUT objevt.

        \return active table index
    */
    Type getType() const { return type_; }

    size_t getSize() const { return GetSize(type_); }

    /** Translate an intensity value in the range [0.0-1.0] into a Color objecct containing RGB + alpha values.

        \param normalizedValue the value to translate

        \return new Color object
    */
    Color getColor(double normalizedValue) const;

    /** Obtain the raw RGB color data from the table used by the CLUT objevt.

        \param index table entry to fetch

        \return read-only RGBColor reference
    */
    const RGBColor& getRGBColor(size_t index) const { return colors_[std::min(index, getSize())]; }

    /** Obtain the raw RGB color data from the table used by the CLUT objevt.

        \param index table entry to fetch

        \return read-only RGBColor reference
    */
    const RGBColor& getRGBColor(double normalizedIndex) const
    {
        return colors_[int(normalizedIndex * (getSize() - 1))];
    }

    bool hasSaturation() const;

    void makeColorMapImage(QImage& image, bool showTicks = true);

    void makeGradiantImage(QImage& image, const RGBColor& on, bool showTicks = true);

private:
    const RGBColor* colors_;
    Type type_;
    float alpha_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
