#ifndef SIDECAR_GUI_TARGETPLOT_H // -*- C++ -*-
#define SIDECAR_GUI_TARGETPLOT_H

#include "QtCore/QList"
#include "QtCore/QString"
#include "QtCore/QTime"

namespace SideCar {
namespace Messages {
class BugPlot;
class Extraction;
class TSPI;
} // namespace Messages
namespace GUI {

/** Extension of the Messages::Extraction class that plots for extractions and user picks. Contains a QTime
    attribute that holds the time when the TargetPlot was born. The History class uses this value to determine
    when to delete a TargetPlot object. The PPIWidget class uses a normalized version of this value to determine
    the alpha value used to draw the TargetPlot graphical representation: older TargetPlot objects have a lower
    alpha value, and thus appear fainter than those that are younger.
*/
class TargetPlot {
public:
    /** Constructor for user picks.

        \param range range in kilometers

        \param azimuth azimuth in radians

        \param
    */
    TargetPlot(const Messages::BugPlot& msg);

    /** Constructor for existing Messages::Extraction objects.

        \param extraction
    */
    TargetPlot(const Messages::Extraction& extraction, const QString& tag);

    /** Constructor for existing Messages::Extraction objects.

        \param extraction
    */
    TargetPlot(const Messages::TSPI& msg);

    double getRange() const { return range_; }

    double getAzimuth() const { return azimuth_; }

    double getElevation() const { return elevation_; }

    double getX() const { return x_; }

    double getY() const { return y_; }

    /** Obtain the current age of the plot

        \return duration in milliseconds
    */
    int getAge() const { return age_.elapsed(); }

    /** Obtain the tag of the RangeTruth plot

        \return duration in milliseconds
    */
    const QString& getTag() const { return tag_; }

private:
    double range_;
    double azimuth_;
    double elevation_;
    double x_;
    double y_;
    QString tag_;
    QTime age_;
};

using TargetPlotList = QList<TargetPlot>;
using TargetPlotListList = QList<TargetPlotList>;

} // end namespace GUI
} // end namespace SideCar

#endif
