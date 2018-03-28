#ifndef SIDECAR_GUI_PARAMEDITOR_H // -*- C++ -*-
#define SIDECAR_GUI_PARAMEDITOR_H

#include "QtCore/QFileInfo"
#include "QtCore/QString"
#include "QtCore/QStringList"
#include "QtGui/QDialog"
#include "QtGui/QHBoxLayout"

#include "XMLRPC/XmlRpc.h"

class QCheckBox;
class QComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStatusBar;
class QVBoxLayout;

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Master {

class ControllerItem;
class MainWindow;
class RootItem;

/** Abstract base class for a parameter editor. Contains a label and a value editor, the type of which is
    dependent on the type of the parameter being edited. Defines the interface that all typed editors must
    define.
*/
class ParamBase : public QObject {
    Q_OBJECT
public:
    static QString GetString(const std::string& value) { return QString::fromStdString(value); }

    static QString GetQuotedString(const QString& value) { return QString("'%1'").arg(value); }

    /** Constructor. Creates a layout into which the label and (future) editor widget reside.

        \param parentLayout the layout to add to

        \param entry definition of the parameter being edited.
    */
    ParamBase(QVBoxLayout* parentLayout, const XmlRpc::XmlRpcValue& entry);

    /** Obtain the name of the parameter.

        \return parameter name
    */
    const std::string& getName() const { return name_; }

    const std::string& getLabel() const { return label_; }

    /** Determine if the parameter value has changed. Derived classes must define.

        \return true if so
    */
    virtual bool hasChanged() const = 0;

    virtual bool notOriginal() const = 0;

    /** Commit the value change. Derived classes must define.
     */
    virtual void acceptChange() = 0;

    /** Undo any changes in the editor by updating it with the current parameter value. Derived classes must
        define.
    */
    virtual void undoChange() = 0;

    virtual void setValue(const XmlRpc::XmlRpcValue& value) = 0;

    virtual void revertToOriginal() = 0;

    /** If the editor has a changed value, update the held parameter value, and add XML data that will update
        the parameter with a new value.

        \param values an XML array that holds the changes
    */
    void addIfChanged(XmlRpc::XmlRpcValue& values, QStringList& changes);

    /** Add an editor widget.

        \param widget the editor widget to use
    */
    ParamBase& add(QWidget* widget);

    void done();

signals:

    /** Notification sent out by the editor when it holds a value that is different from the parameter's current
        value.
    */
    void valueChanged();

public slots:

    void setVisible(bool state);

protected:
    /** Add XML data that will update the parameter with a new value. Derived classes must define.

        \param values an XML array that holds the changes
    */
    virtual QString fillInChange(XmlRpc::XmlRpcValue& values) = 0;

    QLabel* makeLabel(QString text = "");

    QWidget* getParent() { return layout_->parentWidget(); }

private:
    QFrame* frame_;
    QHBoxLayout* layout_;
    std::string name_;
    std::string label_;
};

/** Integer parameter editor. Uses a QSpinBox to perform the value editing.
 */
class IntParam : public ParamBase {
public:
    static Logger::Log& Log();

    /** Constructor. Creates a layout into which the label and QSpinBox reside.

        \param parentLayout the layout to add to

        \param entry definition of the parameter being edited.
    */
    IntParam(QVBoxLayout* parentLayout, const XmlRpc::XmlRpcValue& entry);

    void setValue(const XmlRpc::XmlRpcValue& value);

    /** Determine if the parameter value has changed.

        \return true if so
    */
    bool hasChanged() const;

    bool notOriginal() const { return value_ != original_; }

    /** Commit the value change.
     */
    void acceptChange();

    /** Undo any changes in the editor by updating it with the current parameter value. Derived classes must
        define.
    */
    void undoChange();

    /** Revert the current value to the original configuration value.
     */
    void revertToOriginal();

private:
    /** Add XML data that will update the parameter with a new value.

        \param values an XML array that holds the changes
    */
    QString fillInChange(XmlRpc::XmlRpcValue& values);

    int value_;
    int original_;
    QSpinBox* widget_;
};

/** Double parameter editor. Uses a QLineEdit plus a validator to perform the value editing.
 */
class DoubleParam : public ParamBase {
public:
    /** Constructor. Creates a layout into which the label and QLineEdit reside.

        \param parentLayout the layout to add to

        \param entry definition of the parameter being edited.
    */
    DoubleParam(QVBoxLayout* parentLayout, const XmlRpc::XmlRpcValue& entry);

    void setValue(const XmlRpc::XmlRpcValue& value);

    /** Determine if the parameter value has changed.

        \return true if so
    */
    bool hasChanged() const;

    bool notOriginal() const { return value_ != original_; }

    /** Commit the value change.
     */
    void acceptChange();

    /** Undo any changes in the editor by updating it with the current parameter value. Derived classes must
        define.
    */
    void undoChange();

    /** Revert the current value to the original configuration value.
     */
    void revertToOriginal();

private:
    /** Add XML data that will update the parameter with a new value.

        \param values an XML array that holds the changes
    */
    QString fillInChange(XmlRpc::XmlRpcValue& values);

    double value_;
    double original_;
    QLineEdit* widget_;
};

/** String parameter editor. Uses a QLineEdit to perform the value editing.
 */
class StringParam : public ParamBase {
public:
    /** Constructor. Creates a layout into which the label and QLineEdit reside.

        \param parentLayout the layout to add to

        \param entry definition of the parameter being edited.
    */
    StringParam(QVBoxLayout* parentLayout, const XmlRpc::XmlRpcValue& entry);

    void setValue(const XmlRpc::XmlRpcValue& value);

    /** Determine if the parameter value has changed.

        \return true if so
    */
    bool hasChanged() const;

    bool notOriginal() const { return value_ != original_; }

    /** Commit the value change.
     */
    void acceptChange();

    /** Undo any changes in the editor by updating it with the current parameter value. Derived classes must
        define.
    */
    void undoChange();

    /** Revert the current value to the original configuration value.
     */
    void revertToOriginal();

private:
    /** Add XML data that will update the parameter with a new value.

        \param values an XML array that holds the changes
    */
    QString fillInChange(XmlRpc::XmlRpcValue& values);

    QString value_;
    QString original_;
    QLineEdit* widget_;
};

/** File path parameter editor. Shows a QPushButton, which when clicked, shows a QFileDialog to select a new
    file path.
*/
class PathParam : public ParamBase {
    Q_OBJECT
public:
    enum PathType { kReadPath = 0, kWritePath };

    /** Constructor. Creates a layout into which the label and QPushButton reside.

        \param parentLayout the layout to add to

        \param entry definition of the parameter being edited.
    */
    PathParam(QVBoxLayout* parentLayout, const XmlRpc::XmlRpcValue& entry, PathType pathType);

    void setValue(const XmlRpc::XmlRpcValue& value);

    /** Determine if the parameter value has changed.

        \return true if so
    */
    bool hasChanged() const;

    bool notOriginal() const { return value_ != original_; }

    /** Commit the value change.
     */
    void acceptChange();

    /** Undo any changes in the editor by updating it with the current parameter value. Derived classes must
        define.
    */
    void undoChange();

    /** Revert the current value to the original configuration value.
     */
    void revertToOriginal();

private slots:

    /** Handler invoked when edit button is pressed. Shows a QFileDialog to obtain a new file path.
     */
    void editPath();

private:
    /** Add XML data that will update the parameter with a new value.

        \param values an XML array that holds the changes
    */
    QString fillInChange(XmlRpc::XmlRpcValue& values);

    void updateWidgets();

    QString value_;
    QString original_;
    QFileInfo fileInfo_;
    QLineEdit* filename_;
    QPushButton* widget_;
    PathType pathType_;
};

/** Enumeration parameter editor. Uses a QComboBox to perform the value editing.
 */
class EnumParam : public ParamBase {
public:
    static Logger::Log& Log();

    /** Constructor. Creates a layout into which the label and QComboBox reside.

        \param parentLayout the layout to add to

        \param entry definition of the parameter being edited.
    */
    EnumParam(QVBoxLayout* parentLayout, const XmlRpc::XmlRpcValue& entry);

    void setValue(const XmlRpc::XmlRpcValue& value);

    /** Determine if the parameter value has changed.

        \return true if so
    */
    bool hasChanged() const;

    bool notOriginal() const { return value_ != original_; }

    /** Commit the value change.
     */
    void acceptChange();

    /** Undo any changes in the editor by updating it with the current parameter value. Derived classes must
        define.
    */
    void undoChange();

    /** Revert the current value to the original configuration value.
     */
    void revertToOriginal();

private:
    /** Add XML data that will update the parameter with a new value.

        \param values an XML array that holds the changes
    */
    QString fillInChange(XmlRpc::XmlRpcValue& values);

    int value_;
    int original_;
    int offset_;
    QComboBox* widget_;
};

/** Boolean value parameter editor. Uses a QCheckBox to perform the value editing.
 */
class BoolParam : public ParamBase {
public:
    /** Constructor. Creates a layout into which the label and QCheckBox reside.

        \param parentLayout the layout to add to

        \param entry definition of the parameter being edited.
    */
    BoolParam(QVBoxLayout* parentLayout, const XmlRpc::XmlRpcValue& entry);

    void setValue(const XmlRpc::XmlRpcValue& value);

    /** Determine if the parameter value has changed.

        \return true if so
    */
    bool hasChanged() const;

    bool notOriginal() const { return value_ != original_; }

    /** Commit the value change.
     */
    void acceptChange();

    /** Undo any changes in the editor by updating it with the current parameter value. Derived classes must
        define.
    */
    void undoChange();

    /** Revert the current value to the original configuration value.
     */
    void revertToOriginal();

private:
    /** Add XML data that will update the parameter with a new value.

        \param values an XML array that holds the changes
    */
    QString fillInChange(XmlRpc::XmlRpcValue& values);

    bool value_;
    bool original_;
    QCheckBox* widget_;
};

/** Notification parameter 'editor'. Shows a QPushButton, which when clicked, emits a parameter change request.
    Used for push buttons such as "Reset" or "Rewind".
*/
class NotificationParam : public ParamBase {
    Q_OBJECT
public:
    /** Constructor. Creates a layout into which the label and QPushButton reside.

        \param parentLayout the layout to add to

        \param entry definition of the parameter being edited.
    */
    NotificationParam(QVBoxLayout* parentLayout, const XmlRpc::XmlRpcValue& entry);

    void setValue(const XmlRpc::XmlRpcValue& value);

    /** Determine if the parameter value has changed.

        \return true if so
    */
    bool hasChanged() const;

    bool notOriginal() const { return false; }

    /** Commit the value change.
     */
    void acceptChange();

    /** Undo any changes in the editor by updating it with the current parameter value. Derived classes must
        define.
    */
    void undoChange();

    /** Revert the current value to the original configuration value.
     */
    void revertToOriginal();

signals:
    void sendNotification();

private slots:
    void doNotification();

private:
    /** Add XML data that will update the parameter with a new value.

        \param values an XML array that holds the changes
    */
    QString fillInChange(XmlRpc::XmlRpcValue& values);

    bool armed_;
    int value_;
    QPushButton* widget_;
};

/** A modal dialog that shows a collection of editor widgets with runtime parameter values for an algorithm. In
    addition to the parameter editor widgets, the dialog contains four action buttons: - OK -- accept parameter
    changes, and close the editor - Apply -- accept parameter changes, but keep the editor open - Undo -- undo
    any changes to the parameter editors, and keep the editor open. - Original -- revert any changes to the
    parameter editors to their original values and keep the editor open. - Close -- close the editor, ignoring
    any changes.
*/
class ParamEditor : public QDialog {
    Q_OBJECT
public:
    static Logger::Log& Log();

    enum DialogCode { Rejected = QDialog::Rejected, Accepted = QDialog::Accepted, Applied, AppliedOriginal, Undone };

    ParamEditor(MainWindow* window, ControllerItem* controllerItem);

signals:

    void valuesChanged(const QStringList& changes);

public slots:

    void beginEdit();

    void endEdit(int status);

private slots:

    void advancedClicked(bool state);

    void maybeAccept();
    void maybeReject();
    void apply();
    void updateButtons();
    void undo();
    void applyOriginal();
    void done(int r);

private:
    void buildDialog();
    bool buildParameters();
    bool updateParameters();

    ControllerItem* controllerItem_;
    XmlRpc::XmlRpcValue definition_;
    QFrame* widgetContainer_;
    QList<ParamBase*> parameters_;
    QCheckBox* advanced_;
    QPushButton* apply_;
    QPushButton* ok_;
    QPushButton* undo_;
    QPushButton* original_;
    QPushButton* cancel_;
    QStatusBar* statusBar_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
