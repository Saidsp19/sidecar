#include <cmath>

#include "Utils/Utils.h"

#include "App.h"
#include "Configuration.h"
#include "RangeMap.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::BScope;

RangeMap::RangeMap() : Super(), last_(), viewSettings_(App::GetApp()->getConfiguration()->getViewSettings())
{
    connect(viewSettings_, SIGNAL(settingChanged()), SLOT(viewSettingsChanged()));
    viewSettingsChanged();
}

void
RangeMap::viewSettingsChanged()
{
    azimuthMin_ = viewSettings_->getAzimuthMin();
    azimuthMax_ = viewSettings_->getAzimuthMax();
    clearMap();
}

void
RangeMap::addVertex(const Vertex& vertex)
{
    Vertex v;

    // Convert x,y into azimuth, range. Normalize azimuth to be between azimuthMin_ and azimuthMax_
    //
    v.y = ::sqrt(vertex.x * vertex.x + vertex.y * vertex.y);
    v.x = viewSettings_->normalizedAzimuth(::atan2(vertex.x, vertex.y));

    if (!empty()) {

        // See if we are drawing a line that wraps around to the other side of the image.
        //
        auto dx = v.x - last_.x;
        if (::fabs(dx) > M_PI) {
            auto dy = v.y - last_.y;
            Vertex tmp = v;
            if (v.x > last_.x) {
                // Since our new azimuth is greater than the previous, we need to slice the line at azimuthMin_.
                // Calculate the range value at azimuthMin_: x = (x1 - x0) / (y1 - y0) * (y - y0) + x0
                //
                dx -= Utils::kCircleRadians;
                auto ty = dy / dx * (azimuthMin_ - last_.x) + last_.y;
                v.y = ty;
                v.x = azimuthMin_;
                Super::addVertex(v);

                // Now draw a line around the side of the image so we can connect to the other side of the
                // image. A bit of a waste, but faster than calling glBegin multiple times.
                //
                v.y = -1;
                Super::addVertex(v);
                v.x = azimuthMax_;
                Super::addVertex(v);
                v.y = ty;
                Super::addVertex(v);
                v = tmp;
            } else {
                // Since our new azimuth is less than the previous, we needd to slice the line at azimuthMax_.
                // Calculate the range value at azimuthMax_ with the same equation as given above.
                //
                dx += Utils::kCircleRadians;
                auto ty = dy / dx * (azimuthMax_ - last_.x) + last_.y;
                v.y = ty;
                v.x = azimuthMax_;
                Super::addVertex(v);

                // Again, wrap around the image to azimuthMin_.
                //
                v.y = -1;
                Super::addVertex(v);
                v.x = azimuthMin_;
                Super::addVertex(v);
                v.y = ty;
                Super::addVertex(v);
                v = tmp;
            }
        }
    }

    Super::addVertex(v);
    last_ = v;
}
