#ifndef SIDECAR_GUI_KEYSEQ_H	// -*- C++ -*-
#define SIDECAR_GUI_KEYSEQ_H

#include "QtCore/QMetaType"
#include "QtGui/QKeySequence"

class QKeyEvent;

namespace SideCar {
namespace GUI {

/** A wrapper around QKeySequence that better handles conversions to a QString than that found in QKeySequence
    when run on OS X.
*/
class KeySeq
{
public:

    /** Default constructor. No defined key sequence.
     */
    KeySeq();

    /** Construct a key sequence from a string.

        \param key textual representation of the key
    */
    KeySeq(const QString& key);

    /** Construct a key sequence from a defined QKeySequence.

        \param key key sequence to copy
    */
    KeySeq(const QKeySequence& key);

    /** Construct a key sequence from a key code found in a key event.

        \param event event with the key code
    */
    KeySeq(const QKeyEvent* event);

    /** Obtain the wrapped QKeySequence object

        \return QKeySequence reference
    */
    const QKeySequence& getKeySequence() const { return key_; }

    /** Determine if the given key sequence from an event is the same as the key sequence we represent.

        \param event key sequence from an event

        \return true if matched
    */
    bool matches(const KeySeq& event) const;

    /** Obtain a textual representation of the key sequence.

        \return QString object
    */
    QString asString() const;

private:
    QKeySequence key_;		///< Wrapped QKeySequence object

    static int kMetaTypeId_;
};

}
}

Q_DECLARE_METATYPE(SideCar::GUI::KeySeq)

/** \file
 */

#endif
