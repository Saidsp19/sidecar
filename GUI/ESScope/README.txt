VME System State
----------------

Ideally, the VME would emit the following pieces of information:

  - scanDimensions ( numAlphaScans, numBetaScans )
  - bounds ( minAlpha, maxAlpha, minBeta, maxBeta )
  - alphaIndex
  - alpha value
  - betaIndex
  - beta value
  - rangeMin
  - rangeFactor
  - face tilt from horizontal
  - rotation of pedastal from north
  - pitch, yaw, roll of base from level
  - IRIG time

Alternatively, we need a separate and smaller status record that gets sent out
on a TCP channel to communicate changes to the system's state. Unfortunately,
that requires some handshaking with clients so that they can get the current
system state when first subscribing to data channels.


Tracking Requirements
---------------------

We need a way to command the VME to change its alpha/beta bounds and possibly
scan dimensions. When tracking a target, the client application will collect
alpha/beta information about a target from the user, and craft a control
message to direct the VME system to adjust its alpha/beta bounds so that it
forms a small box around the target of interest. When the client application
receives VME data with the new bounds, it will adjust its views to match that
of the VME system state defined above.

[How to handle client panning and zooming?]


Bug Reports
-----------

User bug reports consist of a 3-tuple: latitude, longitude, height. This is
derived from the alpha, beta, range information obtained from the display and
the radar's location and orientation.
