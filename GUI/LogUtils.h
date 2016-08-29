#ifndef SIDECAR_GUI_LOGUTILS_H // -*- C++ -*-
#define SIDECAR_GUI_LOGUTILS_H

#include "QtCore/QByteArray"
#include "QtCore/QList"
#include "QtCore/QPoint"
#include "QtCore/QPointF"
#include "QtCore/QRect"
#include "QtCore/QRectF"
#include "QtCore/QSize"
#include "QtCore/QSizeF"
#include "QtCore/QString"

#include "Logger/Log.h"

/** Define an std::ostream insertion operator for QString objects. Converts the string to a C++ std::string
    value first. NOTE: this is not the correct thing to do for non-Latin texts.

    \param os C++ output stream to write to

    \param s QString object to write

    \return C++ output stream written to
*/
inline std::ostream&
operator<<(std::ostream& os, const QString& s) { return os << s.toStdString(); }

inline std::ostream&
operator<<(std::ostream& os, const QByteArray& ba) { return os << ba.constData(); }

inline std::ostream&
operator<<(std::ostream& os, const QList<QByteArray>& a)
{ os << '['; foreach(QByteArray b, a) os << b.constData() << ','; return os << ']'; }

inline std::ostream&
operator<<(std::ostream& os, const QList<QString>& a)
{ os << '['; foreach(QString b, a) os << b.toStdString() << ','; return os << ']'; }

inline std::ostream&
operator<<(std::ostream& os, const QSize& s) { return os << s.width() << 'x' << s.height() ; }

inline std::ostream&
operator<<(std::ostream& os, const QSizeF& s) { return os << s.width() << 'x' << s.height() ; }

inline std::ostream&
operator<<(std::ostream& os, const QPoint& s) { return os << s.x() << ',' << s.y() ; }

inline std::ostream&
operator<<(std::ostream& os, const QPointF& s) { return os << s.x() << ',' << s.y() ; }

inline std::ostream&
operator<<(std::ostream&os, const QRect& r) { return os << '[' << r.topLeft() << ' ' << r.size() << ']'; }

inline std::ostream&
operator<<(std::ostream&os, const QRectF& r) { return os << '[' << r.topLeft() << ' ' << r.size() << ']'; }

namespace SideCar {
namespace GUI {

std::string GetOpenGLErrorString(int glErr);

}
}

/** Macro that executes an OpenGL routine, and prints out to Logger::Log any OpenGL error code that results. Any
    routine that uses it <b>must</b> define a local 'log' variable of type Logger::Log or Logger::ProcLog.
    Example:

    \code
    void foo()
    {
    static Logger::ProcLog log("foo", Log());
    LOGINFO << "start" << std::endl;
    GLEC(glEnable(GL_BLEND));
    GLEC(glDisable(GL_MULTISAMPLE));
    }
    \endcode

    \param A OpenGL function call. The macro expands to execute the given call as-is, followed by a check of
    glGetError().
*/
#define GLEC(A) A;							\
    do {						    		\
	GLenum err = glGetError();			    		\
	if (err != GL_NO_ERROR) 		    			\
	    LOGERROR << "GL error after " #A  << " - "			\
		     << GetOpenGLErrorString(err) << " (" << err << ")"	\
		     << std::endl;					\
    } while(0)

#endif
