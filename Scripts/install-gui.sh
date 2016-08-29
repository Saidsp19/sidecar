#!/bin/bash

set -x
SRC="/opt/sidecar/builds/latest/data/gui"

#
# Install SideCar *.desktop files
#
echo "Installing application desktop files"
cd /usr/share/applications || exit 1
rm -f sidecar-* || exit 1
for EACH in ${SRC}/*.desktop; do
    ln -s ${EACH} || exit 1
done

#
# Install SideCar *.directory files
#
echo "Installing menu directory files"
cd /usr/share/desktop-directories || exit 1
rm -f sidecar-* || exit 1
for EACH in ${SRC}/*.directory; do
    ln -s ${EACH} || exit 1
done

#
# Install sidecar.menu file that adds a SideCar menu to the root desktop menu
#
echo "Installing menu installation file"
mkdir -p /etc/xdg/menus/applications-merged
cd /etc/xdg/menus/applications-merged || exit 1
rm -f sidecar.menu || exit 1
ln -s ${SRC}/sidecar.menu || exit 1

exit 0
