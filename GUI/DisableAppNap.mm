#import <Foundation/Foundation.h>
#import <Foundation/NSProcessInfo.h>

#include "DisableAppNap.h"

struct DisableAppNap::Impl {
    id<NSObject> activityId;
};

DisableAppNap::DisableAppNap()
: impl_(new Impl)
{
    ;
}

DisableAppNap::~DisableAppNap()
{
    delete impl_;
}

void
DisableAppNap::begin()
{
    impl_->activityId = [[NSProcessInfo processInfo] beginActivityWithOptions: NSActivityUserInitiated
                                                                       reason:@"Mine!"];
}

void
DisableAppNap::end()
{
    [[NSProcessInfo processInfo] endActivity: impl_->activityId];
}
