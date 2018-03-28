#include <cmath>

#include "QtCore/QDir"
#include "QtCore/QFile"
#include "QtCore/QFileInfo"
#include "QtGui/QAbstractButton"
#include "QtGui/QAction"
#include "QtGui/QFont"
#include "QtGui/QIcon"
#include "QtGui/QPainter"
#include "QtGui/QPixmap"

#include "Utils/Utils.h"

#include "Utils.h"

static const QStringList kPrefixes(QStringList() << "Show"
                                                 << "Hide");

QAction*
SideCar::GUI::MakeMenuAction(const QString& text, QObject* parent, const char* slot, int shortcut)
{
    QAction* action = new QAction(text, parent);
    action->setMenuRole(QAction::NoRole);
    action->setData(kPrefixes);
    if (shortcut) { action->setShortcut(shortcut); }

    if (parent && slot) { QObject::connect(action, SIGNAL(triggered()), parent, slot); }
    return action;
}

void
SideCar::GUI::UpdateToggleAction(QAction* action, bool newCheckedState)
{
    if (newCheckedState != action->isChecked()) { action->setChecked(newCheckedState); }

    // If there is no tool tip text defined for the action, create one using the action's text. Capitalize the
    // first word, but leave everything else lower-case.
    //
    QString text(action->toolTip());
    if (text.isEmpty()) {
        text = action->text();
        text = text.mid(0, 1).toUpper() + text.mid(1).toLower();
    }

    // Obtain a usable set of prefixes
    //
    QStringList prefixes;
    QVariant data(action->data());
    if (data.isValid()) {
        prefixes = data.toStringList();
    } else {
        prefixes = kPrefixes;
    }

    // Obtain the new prefix to use for the tooltip based on the new checked state of the action.
    //
    QString newPrefix(prefixes[newCheckedState ? 1 : 0]);

    // Obtain the old prefix value if present
    //
    QString oldPrefix("");
    if (text.startsWith(prefixes[0])) {
        oldPrefix = prefixes[0];
    } else if (text.startsWith(prefixes[1])) {
        oldPrefix = prefixes[1];
    }

    // Add or substitute the new prefix. If just adding, add a space to the prefix.
    //
    if (oldPrefix.isEmpty()) {
        newPrefix += ' ';
    } else {
        text.remove(0, oldPrefix.size());
    }

    action->setToolTip(newPrefix + text);
}

QString
SideCar::GUI::DistanceToString(double value, int precision, int fieldWidth)
{
    QString tmp = QString::number(value, 'f', precision);
    return tmp.rightJustified(fieldWidth);
}

QString
SideCar::GUI::RadiansToString(double value, bool decimalFormat, int precision, int fieldWidth)
{
    return DegreesToString(Utils::radiansToDegrees(value), decimalFormat, precision, fieldWidth);
}

QString
SideCar::GUI::DegreesToString(double value, bool decimalFormat, int precision, int fieldWidth)
{
    if (decimalFormat) {
        QString tmp = QString::number(value, 'f', precision);
        return tmp.rightJustified(fieldWidth, '0').append(DegreeSymbol());
    }

    // Fetch the number of degrees.
    //
    int deg = int(value);
    if (deg == 360) { deg = 0; }

    // Obtain integral number of seconds from origianl fractional part.
    //
    value = ::rint((value - deg) * 3600.0);

    // Obtain number of minutes.
    //
    int mm = int(value / 60.0);

    // Obtain remaining number of seconds.
    //
    int ss = int(value - mm * 60.0);

    // The typecasts below are necessary to disambiguate the multiple QString::arg() choices. Not sure if this
    // is a GCC 4.0 thing...
    //
    return QString("%1%2 %3' %4\"")
        .arg(deg, 3, int(10), QLatin1Char('0'))
        .arg(DegreeSymbol())
        .arg(mm, 2, int(10), QLatin1Char('0'))
        .arg(ss, 2, int(10), QLatin1Char('0'));
}

QString
SideCar::GUI::ByteAmountToString(double value, int precision)
{
    QString units;
    if (value >= 1024 * 1024 * 1024) {
        units = "GB";
        value /= 1024 * 1024 * 1024;
    } else if (value >= 1024 * 1024) {
        units = "MB";
        value /= 1024 * 1024;
    } else if (value >= 1024) {
        units = "KB";
        value /= 1024;
    } else {
        units = "bytes";
    }

    return QString("%1 %2").arg(value, 0, 'f', precision).arg(units);
}

bool
SideCar::GUI::RemoveDirectory(const QDir& dir)
{
    bool ok = dir.exists();
    if (ok) {
        QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
        foreach (QFileInfo entryInfo, entries) {
            QString path = entryInfo.absoluteFilePath();
            if (entryInfo.isDir()) {
                if (!RemoveDirectory(QDir(path))) {
                    ok = false;
                    break;
                }
            } else {
                QFile file(path);
                if (!file.remove()) {
                    ok = false;
                    break;
                }
            }
        }
    }

    if (ok && !dir.rmdir(dir.absolutePath())) { ok = false; }

    return ok;
}

/** Multiply 4x4 matrix and 4-element vector, returning result in 4-element vector

    \param mat[16] the 4x4 matrix

    \param in[4] the input vector

    \param out[4] the resulting vector
*/
static void
MultMatrixVec(const double mat[16], const double in[4], double out[4])
{
    out[0] = in[0] * mat[0 * 4 + 0] + in[1] * mat[1 * 4 + 0] + in[2] * mat[2 * 4 + 0] + in[3] * mat[3 * 4 + 0];
    out[1] = in[1] * mat[0 * 4 + 1] + in[1] * mat[1 * 4 + 1] + in[2] * mat[2 * 4 + 1] + in[3] * mat[3 * 4 + 1];
    out[2] = in[2] * mat[0 * 4 + 2] + in[1] * mat[1 * 4 + 2] + in[2] * mat[2 * 4 + 2] + in[3] * mat[3 * 4 + 2];
    out[3] = in[3] * mat[0 * 4 + 3] + in[1] * mat[1 * 4 + 3] + in[2] * mat[2 * 4 + 3] + in[3] * mat[3 * 4 + 3];
}

/** Multiply two 4x4 matrices together, returning new 4x4 matrix.

    \param a[16] first 4x4 matrix

    \param b[16] second 4x4 matrix

    \param r[16] result 4x4 matrix
*/
static void
MultMatrices(const double a[16], const double b[16], double r[16])
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            r[i * 4 + j] = a[i * 4 + 0] * b[0 * 4 + j] + a[i * 4 + 1] * b[1 * 4 + j] + a[i * 4 + 2] * b[2 * 4 + j] +
                           a[i * 4 + 3] * b[3 * 4 + j];
        }
    }
}

bool
SideCar::GUI::ProjectPoint(double objx, double objy, double objz, const double modelMatrix[16],
                           const double projMatrix[16], const int viewport[4], double* winx, double* winy, double* winz)
{
    double in[] = {objx, objy, objz, 1.0};
    double out[4];

    MultMatrixVec(modelMatrix, in, out);
    MultMatrixVec(projMatrix, out, in);

    if (in[3] == 0.0) return false;

    in[0] /= in[3];
    in[1] /= in[3];
    in[2] /= in[3];

    /* Map x, y and z to range 0-1 */
    in[0] = in[0] * 0.5 + 0.5;
    in[1] = in[1] * 0.5 + 0.5;
    in[2] = in[2] * 0.5 + 0.5;

    /* Map x,y to viewport */
    in[0] = in[0] * viewport[2] + viewport[0];
    in[1] = in[1] * viewport[3] + viewport[1];

    *winx = in[0];
    *winy = in[1];
    *winz = in[2];

    return true;
}

/** Attempt to invert a 4x4 matrix.

    \param m[16] input 4x4 matrix

    \param invOut[16] output 4x4 matrix

    \return true if successful, false otherwise
*/
static bool
InvertMatrix(const double m[16], double invOut[16])
{
    double inv[16], det;
    int i;

    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] +
             m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] -
             m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] +
             m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] -
              m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] -
             m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] +
             m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] -
             m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] +
              m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] +
             m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] -
             m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] +
              m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] -
              m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] -
             m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] -
              m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] -
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (det == 0) return false;

    det = 1.0 / det;

    for (i = 0; i < 16; i++) invOut[i] = inv[i] * det;

    return true;
}

bool
SideCar::GUI::UnProjectPoint(double winx, double winy, double winz, const double modelMatrix[16],
                             const double projMatrix[16], const int viewport[4], double* objx, double* objy,
                             double* objz)
{
    double finalMatrix[16];
    MultMatrices(modelMatrix, projMatrix, finalMatrix);
    if (!InvertMatrix(finalMatrix, finalMatrix)) return false;

    double in[4];
    double out[4];

    in[0] = winx;
    in[1] = winy;
    in[2] = winz;
    in[3] = 1.0;

    /* Map x and y from window coordinates */
    in[0] = (in[0] - viewport[0]) / viewport[2];
    in[1] = (in[1] - viewport[1]) / viewport[3];

    /* Map to range -1 to 1 */
    in[0] = in[0] * 2 - 1;
    in[1] = in[1] * 2 - 1;
    in[2] = in[2] * 2 - 1;

    MultMatrixVec(finalMatrix, in, out);
    if (out[3] == 0.0) return false;

    out[0] /= out[3];
    out[1] /= out[3];
    out[2] /= out[3];

    *objx = out[0];
    *objy = out[1];
    *objz = out[2];

    return true;
}
