#ifndef SIDECAR_GUI_TARGETPLOTIMAGING_H // -*- C++ -*-
#define SIDECAR_GUI_TARGETPLOTIMAGING_H

#include "QtCore/QPoint"
#include "QtCore/QString"

#include "GUI/ChannelImaging.h"
#include "GUI/IntSetting.h"
#include "GUI/QComboBoxSetting.h"
#include "GUI/TargetPlot.h"
#include "GUI/VertexColorArray.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

class GLFont;
class PlotSymbolWidget;
class TargetPlot;
class VertexColorTagArray;

/** Abstract base class that defines the interface for plot positioners. A plot positioner returns a Vertex
    center for a given TargetPlot object. In normal use, it will apply some transform from target coordinates
    into view coordinates, returned via the getPosition() method which derived classes must define.

    By isolating view transforms from the TargetPlotImaging class, we are able to make TargetPlotImaging common to the
    BScope and PPIDisplay applications.
*/
class PlotPositionFunctor {
public:
    /** Constructor.
     */
    PlotPositionFunctor();

    /** Destructor. Releases any GLFont object in use.
     */
    virtual ~PlotPositionFunctor();

    /** Obtain the center view coordinates (x,y) for a given TargetPlot object.

        \param plot TargetPlot object to work with

        \return view coordinates for the target
    */
    virtual Vertex getPosition(const TargetPlot& plot) const = 0;

    void renderTags(int tagSize, const VertexColorTagArray& tags);

private:
    GLFont* font_;
};

/** Extension of the ChannelImaging class that contains display settings for target plots.
 */
class TargetPlotImaging : public ChannelImaging {
    Q_OBJECT
    using Super = ChannelImaging;

public:
    /** Enumeration of the various symbol types available to depict target plots.
     */
    enum SymbolType {
        kCross,
        kCrossSquare,
        kDiamondFilled,
        kDiamondHollow,
        kPlus,
        kPlusDiamond,
        kSquareFilled,
        kSquareHollow,
        kTriangleFilled,
        kTriangleHollow,
        kDaggerFilled,
        kDaggerHollow,
        kNumSymbolTypes
    };

    struct SymbolInfo {
        QString name;
        int kind;
        const QPoint* points;
        size_t size;
    };

    static Logger::Log& Log();

    static const QString& GetSymbolName(SymbolType symbolType);

    static const SymbolInfo& GetSymbolInfo(SymbolType symbolType);

    TargetPlotImaging(PlotPositionFunctor* plotPositionFunctor, BoolSetting* visible, ColorButtonSetting* color,
                      DoubleSetting* extent, OpacitySetting* opacity, QComboBoxSetting* symbolType,
                      DoubleSetting* lineWidth, IntSetting* lifeTime, BoolSetting* fadeEnabled,
                      BoolSetting* showTrails = 0, IntSetting* tagSize = 0, BoolSetting* showTags = 0);

    void setPlotPositionFunctor(PlotPositionFunctor* plotPositionFunctor)
    {
        plotPositionFunctor_ = plotPositionFunctor;
    }

    /** Obtain the extent size of the symbols. This is roughly equal to the diameter of a circle that
        circumscribes the plot symbol. Rename of the ChannelImaging::getSize() method to better reflect how the
        setting is used in the TargetPlotImaging class.

        \return extent size
    */
    float getExtent() const { return getSize(); }

    /** Obtain half the extent size. This is roughly equal to the radius of a circle that circumscribes the plot
        symbol.

        \return getExtent() / 2
    */
    float getExtent2() const { return getSize() / 2.0; }

    /** Obtain the current symbol type.

        \return symbol type
    */
    SymbolType getSymbolType() const { return SymbolType(symbolType_->getValue()); }

    /** Obtain the current symbol line width. Used by symbol types kCross and kPlus.

        \return line width
    */
    float getLineWidth() const { return lineWidth_->getValue(); }

    /** Obtain the current plot lifetime, the number of milliseconds that a plot will display after its
        creation.

        \return duration in milliseconds
    */
    int getLifeTime() const { return lifeTime_->getValue() * 1000; }

    /** Obtain the time-degraded opacity value for the target. As time passes, the opacity decreases, thus
        fading the plot.

        \param alpha the initiali value to fade

        \param age the plot age

        \return
    */
    void fadeColor(Color& color, int age) const { color.alpha *= 1.0 - double(age) / getLifeTime(); }

    /** Determine if plot fading is enabled. If it is, plot representations will have decreasing alpha values as
        they age.

        \return true if enabled
    */
    bool getFadeEnabled() const { return fadeEnabled_->getValue(); }

    /** Render plot symbols at the given locations.

        \param targets list of plots to render
    */
    void render(QWidget* widget, const TargetPlotList& targets);

    void render(QWidget* widget, const TargetPlotListList& targets);

    void connectPlotSymbolWidget(PlotSymbolWidget* widget);

    void addSymbolIcons(QComboBox* widget) const;

signals:

    /** Notification sent out when the lifetime setting changes.

        \param msecs
    */
    void lifeTimeChanged(int msecs);

public slots:

    void clearSymbolPoints();

private slots:

    /** Notification handler invoked when the size setting changes. Override of ChannelImaging::setSize().

        \param value new setting value
    */
    void sizeChanged(double value);

    /** Notification handler invoked when the lifetime setting changes.

        \param value new setting value
    */
    void postLifeTimeChanged(int value);

private:
    void scaleSymbolPoints();

    PlotPositionFunctor* plotPositionFunctor_;
    QComboBoxSetting* symbolType_;
    DoubleSetting* lineWidth_;
    IntSetting* lifeTime_;
    BoolSetting* fadeEnabled_;
    BoolSetting* showTrails_;
    IntSetting* tagSize_;
    BoolSetting* showTags_;
    VertexColorArray points_;
    VertexVector symbolPoints_;

    static SymbolInfo kSymbolInfo_[kNumSymbolTypes];
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
