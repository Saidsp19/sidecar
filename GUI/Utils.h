#ifndef SIDECAR_GUI_UTILS_H // -*- C++ -*-
#define SIDECAR_GUI_UTILS_H

#include "QtCore/QChar"
#include "QtCore/QObject"
#include "QtCore/QString"

class QAbstractButton;
class QAction;
class QDir;
class QFont;

namespace SideCar {
namespace GUI {

/** Obtain the Unicode character that represents a degree symbol.

    \return QChar value
*/
inline QChar
DegreeSymbol()
{
    return 0x00B0;
}

/** Obtain the Unicode character that represents a bullet symbol

    \return QChar value
*/
inline QChar
BulletSymbol()
{
    return 0x00B7;
}

/** Obtain the Unicode character that represents an ellipses symbol (...)

    \return QChar value
*/
inline QChar
HorizontalEllipses()
{
    return 0x2026;
}

/** Return a text representation for a distance value.

    \param value the value to format

    \param precision how many digits to the right of the decimal point to show

    \param fieldWidth how many characters to show

    \return formatted result
*/
extern QString DistanceToString(double value, int precision, int fieldWidth);

/** Format an angular radian value into one of two formats:

    - ddd.frac: whole degrees followed by fractional degrees after a decimal point.
    - ddd mm ss: formatted as DDD MM' SS" format

    \param value radian value to format

    \param decimalFormat if true, format in ddd.frac format

    \return formatted value
*/
extern QString RadiansToString(double value, bool decimalFormat = false, int precision = 2, int valueWidth = 6);

/** Format an angular degree value into one of two formats:

    - ddd.frac: whole degrees followed by fractional degrees after a decimal point.
    - ddd mm ss: formatted as DDD MM' SS" format

    \param value degree value to format

    \param decimalFormat if true, format in ddd.frac format

    \return formatted value
*/
extern QString DegreesToString(double value, bool decimalFormat = false, int precision = 1, int fieldWidth = 5);

/** Create a new QAction object with a given text. If the parent and slot arguments are not NULL, connect the
    QAction::triggered signal to the slot of the parent. Finally, if the given shortcut value is not zero, pass
    it to QAction::setShortcut().

    \param text label to use for the action

    \param parent hierarchical parent if not NULL. Also the receiver to use if the slot parameter is not NULL.

    \param slot the optional slot to connect to if not NULL. Note that for a connection to take place \a parent
    must not be NULL.

    \param shortcut optional key sequence to use for the action shortcut.

    \return
*/
extern QAction* MakeMenuAction(const QString& text, QObject* parent = 0, const char* slot = 0, int shortcut = 0);

/** Update a show/hide QAction object so that its text reflects the given show/hide state. Implements a SideCar
    GUI application standard for QAction objects that control the visibility of a window. Adjusts the action
    text so that it has "Show" or "Hide" at the beginning to reflect what will happen if the action is
    triggered: if \a state is true, append "Hide"; otherwise, append "Show".

    \param action the QAction object to adjust

    \param state true if reflecting a shown state, false for a hidden state

    \return
*/
extern void UpdateToggleAction(QAction* action, bool newCheckedState);

/** Format a byte count into a human-readable string. Example: 3223323648 -> "3.0 GB"

    \param value the value to format

    \param precision the number of digits to show to the right of a decimal point.

    \return formatted value
*/
extern QString ByteAmountToString(double value, int precision = 1);

/** Recursively remove a given directory and all of its contents. Should really be called 'DeleteDirectory'

    \param directory the directory to remove

    \return true if successful, false otherwise.
*/
extern bool RemoveDirectory(const QDir& directory);

/** Project a 3D model point into a 3D view point.

    \param objx X value of model point

    \param objy Y value of model point

    \param objz Z value of model point

    \param modelMatrix[16] definition of model space

    \param projMatrix[16] transformation matrix into view space

    \param viewport[4] description of view showing projected points

    \param winx resulting X value

    \param winy resulting Y value

    \param winz resulting Z value

    \return true if successful, false otherwise
*/
extern bool ProjectPoint(double objx, double objy, double objz, const double modelMatrix[16],
                         const double projMatrix[16], const int viewport[4], double* winx, double* winy, double* winz);

/** Inverse of ProjectPoint. Project a view point into model space.

    \param winx X value of view point

    \param winy Y value of view point

    \param winz Z value of view point

    \param modelMatrix[16] definition of model space

    \param projMatrix[16] transformation matrix into view space

    \param viewport[4] description of view showing projected points

    \param objx resulting X value

    \param objy resulting Y value

    \param objz resultin Z value

    \return true if successful, false otherwise
*/
extern bool UnProjectPoint(double winx, double winy, double winz, const double modelMatrix[16],
                           const double projMatrix[16], const int viewport[4], double* objx, double* objy,
                           double* objz);

} // end namespace GUI
} // end namespace SideCar

/** \file Utils.h Sundry utilities used by the Qt GUI applications.
 */

#endif
