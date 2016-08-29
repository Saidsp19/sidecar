#include "QtCore/QDir"
#include "QtCore/QFileInfo"
#include "QtCore/QVariant"
#include "QtGui/QCheckBox"
#include "QtGui/QComboBox"
#include "QtGui/QFileDialog"
#include "QtGui/QFont"
#include "QtGui/QFrame"
#include "QtGui/QHBoxLayout"
#include "QtGui/QLabel"
#include "QtGui/QLineEdit"
#include "QtGui/QMessageBox"
#include "QtGui/QPushButton"
#include "QtGui/QSpinBox"
#include "QtGui/QStatusBar"
#include "QtGui/QVBoxLayout"

#include "GUI/LogUtils.h"

#include "ControllerItem.h"
#include "MainWindow.h"
#include "ParamEditor.h"
#include "StreamItem.h"
#include "TreeViewItem.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

ParamBase::ParamBase(QVBoxLayout* parentLayout,
                     const XmlRpc::XmlRpcValue& entry)
    : frame_(0), layout_(0), name_(entry["name"]),
      label_(entry["label"])
{
    frame_ = new QFrame(parentLayout->parentWidget());
    frame_->setFrameStyle(QFrame::Box | QFrame::Sunken);
    parentLayout->addWidget(frame_);
    layout_ = new QHBoxLayout(frame_);
    layout_->setMargin(0);
    layout_->setMargin(6);
    layout_->addStretch();
}

QLabel*
ParamBase::makeLabel(QString text)
{
    if (text.isEmpty()) text = GetString(label_);
    QLabel* label = new QLabel(text, getParent());
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return label;
}

ParamBase&
ParamBase::add(QWidget* widget)
{
    layout_->addWidget(widget);
    widget->setToolTip(QString(GetString(name_)));
    return *this;
}

void
ParamBase::done()
{
#if 0
    layout_->addStretch();
    QLabel* label = new QLabel(GetString(name_), getParent());
    QFont font = label->font();
    font.setPointSize(font.pointSize() - 1);
    label->setFont(font);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    label->setToolTip("Configuration parameter name");
    layout_->addWidget(label);
#endif	
}

void
ParamBase::setVisible(bool state)
{
    frame_->setVisible(state);
}

void
ParamBase::addIfChanged(XmlRpc::XmlRpcValue& values, QStringList& changes)
{
    if (hasChanged()) {
	values.push_back(name_);
	acceptChange();
	changes.append(QString("  %1 = %2\n")
                       .arg(QString::fromStdString(name_))
                       .arg(fillInChange(values)));
    }
}

Logger::Log&
IntParam::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("SideCar.GUI.ParamEditor.IntParam");
    return log_;
}

IntParam::IntParam(QVBoxLayout* parentLayout,
                   const XmlRpc::XmlRpcValue& entry)
    : ParamBase(parentLayout, entry),
      value_(entry["value"]), original_(entry["original"]),
      widget_(new QSpinBox(getParent()))
{
    if (entry.hasMember("min"))
	widget_->setMinimum(entry["min"]);
    else
	widget_->setMinimum(-std::numeric_limits<int>::max());
    if (entry.hasMember("max"))
	widget_->setMaximum(entry["max"]);
    else
	widget_->setMaximum(std::numeric_limits<int>::max());

    widget_->setValue(value_);
    connect(widget_, SIGNAL(valueChanged(int)), SIGNAL(valueChanged()));
    add(makeLabel()).add(widget_).done();
}

void
IntParam::setValue(const XmlRpc::XmlRpcValue& value)
{
    value_ = value;
    undoChange();
}

bool
IntParam::hasChanged() const
{
    return widget_->value() != value_;
}

void
IntParam::acceptChange()
{
    value_ = widget_->value();
}

void
IntParam::undoChange()
{
    widget_->setValue(value_);
}

void
IntParam::revertToOriginal()
{
    widget_->setValue(original_);
}

QString
IntParam::fillInChange(XmlRpc::XmlRpcValue& values)
{
    values.push_back(value_);
    return QString::number(value_);
}

DoubleParam::DoubleParam(QVBoxLayout* parentLayout,
                         const XmlRpc::XmlRpcValue& entry)
    : ParamBase(parentLayout, entry), value_(entry["value"]),
      original_(entry["original"]),
      widget_(new QLineEdit(QString::number(value_), getParent()))
{
    QDoubleValidator* validator = new QDoubleValidator(widget_);
    if (entry.hasMember("min"))
	validator->setBottom(entry["min"]);
    if (entry.hasMember("max"))
	validator->setTop(entry["max"]);
    widget_->setValidator(validator);
    widget_->setAlignment(Qt::AlignRight);

    connect(widget_, SIGNAL(textEdited(const QString&)),
            SIGNAL(valueChanged()));

    add(makeLabel()).add(widget_).done();
}

void
DoubleParam::setValue(const XmlRpc::XmlRpcValue& value)
{
    value_ = value;
    undoChange();
}

bool
DoubleParam::hasChanged() const
{
    return widget_->text().toDouble() != value_;
}

void
DoubleParam::acceptChange()
{
    value_ = widget_->text().toDouble();
}

void
DoubleParam::undoChange()
{
    widget_->setText(QString::number(value_));
}

void
DoubleParam::revertToOriginal()
{
    widget_->setText(QString::number(original_));
}

QString
DoubleParam::fillInChange(XmlRpc::XmlRpcValue& values)
{
    values.push_back(value_);
    return QString::number(value_);
}

StringParam::StringParam(QVBoxLayout* parentLayout,
                         const XmlRpc::XmlRpcValue& entry)
    : ParamBase(parentLayout, entry),
      value_(GetString(std::string(entry["value"]))),
      original_(GetString(std::string(entry["original"]))),
      widget_(new QLineEdit(value_, getParent()))
{
    connect(widget_, SIGNAL(textEdited(const QString&)),
            SIGNAL(valueChanged()));
    add(makeLabel()).add(widget_).done();
}

void
StringParam::setValue(const XmlRpc::XmlRpcValue& value)
{
    value_ = GetString(value);
    undoChange();
}

bool
StringParam::hasChanged() const
{
    return widget_->text() != value_;
}

void
StringParam::acceptChange()
{
    value_ = widget_->text();
}

void
StringParam::undoChange()
{
    widget_->setText(value_);
}

void
StringParam::revertToOriginal()
{
    widget_->setText(original_);
}

QString
StringParam::fillInChange(XmlRpc::XmlRpcValue& values)
{
    values.push_back(value_.toStdString());
    return GetQuotedString(value_);
}

PathParam::PathParam(QVBoxLayout* parentLayout,
                     const XmlRpc::XmlRpcValue& entry, PathType pathType)
    : ParamBase(parentLayout, entry),
      value_(GetString(entry["value"])),
      original_(GetString(entry["original"])), fileInfo_(value_),
      filename_(new QLineEdit(fileInfo_.fileName(), getParent())),
      widget_(new QPushButton("Set...", getParent())), pathType_(pathType)
{
    filename_->setReadOnly(true);
    updateWidgets();
    connect(widget_, SIGNAL(clicked()), SLOT(editPath()));
    add(makeLabel()).add(filename_).add(widget_).done();
}

void
PathParam::setValue(const XmlRpc::XmlRpcValue& value)
{
    value_ = GetString(value);
    undoChange();
}

void
PathParam::updateWidgets()
{
    filename_->setText(fileInfo_.fileName());
    filename_->setToolTip(fileInfo_.absolutePath());
}

bool
PathParam::hasChanged() const
{
    return value_ != fileInfo_.filePath();
}

void
PathParam::acceptChange()
{
    value_ = fileInfo_.filePath();
}

void
PathParam::undoChange()
{
    fileInfo_ = value_;
    updateWidgets();
}

void
PathParam::revertToOriginal()
{
    fileInfo_ = original_;
    updateWidgets();
}

QString
PathParam::fillInChange(XmlRpc::XmlRpcValue& values)
{
    values.push_back(value_.toStdString());
    return GetQuotedString(value_);
}

void
PathParam::editPath()
{
    QWidget* window = widget_->window();
    QString caption = QString("%1").arg(GetString(getLabel()));
    QString directory = fileInfo_.absolutePath();
    QString filter = QString("*.%1").arg(fileInfo_.suffix());
    QString newPath =
	pathType_ == kReadPath ?
	QFileDialog::getOpenFileName(window,
                                     caption,
                                     directory,
                                     filter,
                                     0,
                                     QFileDialog::DontResolveSymlinks) :
	QFileDialog::getSaveFileName(window,
                                     caption,
                                     directory,
                                     filter,
                                     0,
                                     QFileDialog::DontResolveSymlinks);
    if (! newPath.isEmpty()) {
	fileInfo_.setFile(newPath);
	updateWidgets();
	emit valueChanged();
    }
}

Logger::Log&
EnumParam::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("SideCar.GUI.ParamEditor.EnumParam");
    return log_;
}

EnumParam::EnumParam(QVBoxLayout* parentLayout,
                     const XmlRpc::XmlRpcValue& entry)
    : ParamBase(parentLayout, entry),
      value_(entry["value"]), original_(entry["original"]),
      offset_(entry["min"]), widget_(new QComboBox(getParent()))
{
    Logger::ProcLog log("EnumParam", Log());
    LOGINFO << getName() << " value: " << value_ << " offset: "
	    << offset_ << std::endl;

    const XmlRpc::XmlRpcValue::ValueArray& names(entry["enumNames"]);
    for (size_t index = 0; index < names.size(); ++index) {
	QString entry(QString::fromStdString(
                          std::string(names[index])));
	widget_->addItem(entry);
    }

    undoChange();
    connect(widget_, SIGNAL(currentIndexChanged(int)),
            SIGNAL(valueChanged()));
    add(makeLabel()).add(widget_).done();
}

void
EnumParam::setValue(const XmlRpc::XmlRpcValue& value)
{
    Logger::ProcLog log("setValue", Log());
    value_ = value;
    LOGINFO << getName() << " value: " << value << std::endl;
    undoChange();
}

bool
EnumParam::hasChanged() const
{
    return value_ != -1 && (value_ - offset_) != widget_->currentIndex();
}

void
EnumParam::acceptChange()
{
    Logger::ProcLog log("acceptChange", Log());
    value_ = widget_->currentIndex();
    if (value_ != -1) value_ += offset_;
    LOGINFO << getName() << " value: " << value_ << std::endl;
}

void
EnumParam::undoChange()
{
    Logger::ProcLog log("undoChange", Log());
    int index = value_;
    if (index != -1) index -= offset_;
    LOGINFO << getName() << " value: " << value_ << " index: " << index
	    << std::endl;
    widget_->setCurrentIndex(index);
}

void
EnumParam::revertToOriginal()
{
    Logger::ProcLog log("revertToOriginal", Log());
    int index = original_;
    if (index != -1) index -= offset_;
    LOGINFO << getName() << " original: " << original_ << " index: " << index
	    << std::endl;
    widget_->setCurrentIndex(index);
}

QString
EnumParam::fillInChange(XmlRpc::XmlRpcValue& values)
{
    Logger::ProcLog log("fillInChange", Log());
    LOGINFO << getName() << " value: " << value_ << std::endl;
    values.push_back(value_);
    return GetQuotedString(widget_->itemText(value_));
}

BoolParam::BoolParam(QVBoxLayout* parentLayout,
                     const XmlRpc::XmlRpcValue& entry)
    : ParamBase(parentLayout, entry), value_(entry["value"]),
      original_(entry["original"]),
      widget_(new QCheckBox(GetString(getLabel()), getParent()))
{
    widget_->setCheckState(value_ ? Qt::Checked : Qt::Unchecked);
    connect(widget_, SIGNAL(stateChanged(int)), SIGNAL(valueChanged()));
    add(widget_).done();
}

void
BoolParam::setValue(const XmlRpc:: XmlRpcValue& value)
{
    value_ = value;
    undoChange();
}

bool
BoolParam::hasChanged() const
{
    return value_ != widget_->isChecked();
}

void
BoolParam::acceptChange()
{
    value_ = widget_->isChecked();
}

void
BoolParam::undoChange()
{
    widget_->setCheckState(value_ ? Qt::Checked : Qt::Unchecked);
}

void
BoolParam::revertToOriginal()
{
    widget_->setCheckState(original_ ? Qt::Checked : Qt::Unchecked);
}

QString
BoolParam::fillInChange(XmlRpc::XmlRpcValue& values)
{
    values.push_back(value_);
    return QString(value_ ? "TRUE" : "FALSE");
}

NotificationParam::NotificationParam(QVBoxLayout* parentLayout,
                                     const XmlRpc::XmlRpcValue& entry)
    : ParamBase(parentLayout, entry),
      armed_(false), value_(entry["value"]),
      widget_(new QPushButton("Send", getParent()))
{
    connect(widget_, SIGNAL(clicked()), SLOT(doNotification()));
    add(makeLabel(GetString(getLabel()))).add(widget_).done();
}

void
NotificationParam::setValue(const XmlRpc::XmlRpcValue& value)
{
    value_ = value;
    undoChange();
}

void
NotificationParam::doNotification()
{
    armed_ = true;
    emit sendNotification();
}

bool
NotificationParam::hasChanged() const
{
    return armed_;
}

void
NotificationParam::acceptChange()
{
    ;
}

void
NotificationParam::undoChange()
{
    ;
}

void
NotificationParam::revertToOriginal()
{
    ;
}

QString
NotificationParam::fillInChange(XmlRpc::XmlRpcValue& values)
{
    values.push_back(++value_);
    armed_ = false;
    return QString("N/A");
}

Logger::Log&
ParamEditor::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("SideCar.GUI.ParamEditor");
    return log_;
}

ParamEditor::ParamEditor(MainWindow* window, ControllerItem* controllerItem)
    : QDialog(window), controllerItem_(controllerItem), definition_(),
      widgetContainer_(0), parameters_(), advanced_(0), apply_(0),
      ok_(0), undo_(0), original_(0), cancel_(0),
      statusBar_(window->statusBar())
{
    Q_ASSERT(window && controllerItem);

    // Dispose of ourselves when the ControllerItem that manages our algorithm goes away.
    //
    connect(controllerItem, SIGNAL(destroyed()), SLOT(deleteLater()));

    // The finished() signal is sent when the edit dialog wishes to send updates to the algorithm.
    //
    connect(this, SIGNAL(finished(int)), SLOT(endEdit(int)));

    // Notify the MainWindow of parameter updates.
    //
    connect(this, SIGNAL(valuesChanged(const QStringList&)), window,
            SLOT(parameterValuesChanged(const QStringList&)));

    setModal(false);
    setWindowTitle(QString("%1 Parameters")
                   .arg(controllerItem->getName()));
}

void
ParamEditor::buildDialog()
{
    QVBoxLayout* vbox = new QVBoxLayout(this);
    vbox->setSizeConstraint(QLayout::SetFixedSize);
    vbox->setMargin(9);
    vbox->setSpacing(6);
    widgetContainer_ = new QFrame(this);
    widgetContainer_->setFrameStyle(QFrame::NoFrame);
    vbox->addWidget(widgetContainer_);
    vbox->addStretch(1);

    QHBoxLayout* buttonLayout(new QHBoxLayout);
    vbox->addLayout(buttonLayout);

    advanced_ = new QCheckBox("Advanced", this);
    advanced_->setChecked(false);
    advancedClicked(false);
    connect(advanced_, SIGNAL(clicked(bool)),
            SLOT(advancedClicked(bool)));

    buttonLayout->addWidget(advanced_);
    buttonLayout->addStretch();

    // Create action buttons at the bottom of the dialog window.
    //
    ok_ = new QPushButton("OK", this);
    ok_->setEnabled(true);
    ok_->setToolTip("Select to apply changes and close the editor");
    buttonLayout->addWidget(ok_);
    connect(ok_, SIGNAL(clicked()), this, SLOT(maybeAccept()));

    apply_ = new QPushButton("Apply", this);
    apply_->setEnabled(false);
    apply_->setToolTip("Select to apply changes without closing\n"
                       "the editor");
    apply_->setDefault(true);
    buttonLayout->addWidget(apply_);
    connect(apply_, SIGNAL(clicked()), this, SLOT(apply()));

    undo_ = new QPushButton("Undo", this);
    undo_->setEnabled(false);
    undo_->setToolTip("Select to use undo value changes, using\n"
                      "values present when the editor opened.");
    buttonLayout->addWidget(undo_);
    connect(undo_, SIGNAL(clicked()), this, SLOT(undo()));

    buttonLayout = new QHBoxLayout;
    vbox->addLayout(buttonLayout);
    buttonLayout->addStretch();

    cancel_ = new QPushButton("Cancel", this);
    cancel_->setToolTip("Cancel any pending value changes and\n"
                        "close the editor window.");
    buttonLayout->addWidget(cancel_);
    connect(cancel_, SIGNAL(clicked()), this, SLOT(maybeReject()));

    original_ = new QPushButton("Use Configured", this);
    original_->setEnabled(false);
    original_->setToolTip("Press to revert to parameter value from\n"
                          "configuration file. Editor remains open.");
    buttonLayout->addWidget(original_);
    connect(original_, SIGNAL(clicked()), this, SLOT(applyOriginal()));
}

void
ParamEditor::advancedClicked(bool state)
{
    if (state)
	advanced_->setToolTip("Click to hide advanced configuration "
                              "parameters");
    else 
	advanced_->setToolTip("Click to show advanced configuration "
                              "parameters");
}

bool
ParamEditor::buildParameters()
{
    if (! controllerItem_->getParameters(definition_)) {
	QMessageBox::information(
	    qApp->activeWindow(), "Edit Failed",
	    QString("<p>Unable to request the configuration for the service '"
                    "%1'. The service may no longer exists or it may not "
                    "be responding to XML/RPC requests.</p>")
	    .arg(controllerItem_->getName()),
	    QMessageBox::Ok);
	return false;
    }

    if (definition_.size() == 0) {
	QMessageBox::information(qApp->activeWindow(),
                                 "No Parameters",
                                 "<p>The service you attempted to modify has "
                                 "no editable parameters.</p>",
                                 QMessageBox::Ok);
	return false;
    }

    QVBoxLayout* vbox = new QVBoxLayout(widgetContainer_);
    vbox->setMargin(9);
    vbox->setSpacing(6);

    // Create label/widget pairs for each of the parameters. Remember the edit widget and original value of the
    // parameter.
    //
    for (int index = 0; index < definition_.size(); ++index) {
	XmlRpc::XmlRpcValue& vs(definition_[index]);
	std::string type(vs["type"]);
	bool isAdvanced(vs["advanced"]);
	ParamBase* item = 0;

	if (type == "int") {
	    item = new IntParam(vbox, vs);
	}
	else if (type == "double") {
	    item = new DoubleParam(vbox, vs);
	}
	else if (type == "string") {
	    item = new StringParam(vbox, vs);
	}
	else if (type == "readPath") {
	    item = new PathParam(vbox, vs, PathParam::kReadPath);
	}
	else if (type == "writePath") {
	    item = new PathParam(vbox, vs, PathParam::kWritePath);
	}
	else if (type == "enum") {
	    item = new EnumParam(vbox, vs);
	}
	else if (type == "bool") {
	    item = new BoolParam(vbox, vs);
	}
	else if (type == "notification") {
	    item = new NotificationParam(vbox, vs);
	    connect(item, SIGNAL(sendNotification()), SLOT(apply()));
	}
	else {
	    QMessageBox::information(
		qApp->activeWindow(), "Edit Failed",
		QString("Unexpected data type from XML/RPC response - %1")
		.arg(type.c_str()), QMessageBox::Ok);
	    return false;
	}

	connect(item, SIGNAL(valueChanged()), SLOT(updateButtons()));
	parameters_.append(item);

	if (isAdvanced) {
	    item->setVisible(advanced_->isChecked());
	    connect(advanced_, SIGNAL(clicked(bool)), item,
                    SLOT(setVisible(bool)));
	}
    }

    adjustSize();
    updateButtons();

    return true;
}


bool
ParamEditor::updateParameters()
{
    if (! controllerItem_->getParameters(definition_)) {
	QMessageBox::information(
	    qApp->activeWindow(), "Edit Failed",
	    QString("<p>Unable to request the configuration for the service '"
                    "%1'. The service may no longer exists or it may not "
                    "be responding to XML/RPC requests.</p>")
	    .arg(controllerItem_->getName()),
	    QMessageBox::Ok);
	return false;
    }

    // Update widgets with current values.
    //
    for (int index = 0; index < definition_.size(); ++index) {
	XmlRpc::XmlRpcValue& vs(definition_[index]);
	parameters_[index]->setValue(vs["value"]);
    }

    return true;
}

void
ParamEditor::beginEdit()
{
    if (! widgetContainer_)
	buildDialog();
    if (parameters_.empty()) {
	if (! buildParameters())
	    return;
    }
    else {
	if (! updateParameters())
	    return;
    }

    show();
    raise();
    activateWindow();
}

void
ParamEditor::endEdit(int status)
{
    // If the user rejected the changes (via the Cancel button) then just ignore the entire edit.
    //
    if (status == Rejected)
	return;

    // Build the XML-RCP request with new parameter values.
    //
    XmlRpc::XmlRpcValue args;
    args.setSize(0);

    // Iterate over all of the parameter defintions and add XML-RCP data that describes the parameter and its
    // new value.
    //
    QStringList changed;
    changed.append(controllerItem_->getParameterChangedHeading());

    for (int index = 0; index < parameters_.size(); ++index)
	parameters_[index]->addIfChanged(args, changed);

    if (args.size()) {
	if (! 	controllerItem_->setParameters(args)) {
	    QMessageBox::information(
		qApp->activeWindow(), "Edit Failed",
		QString("Unable to update the value for the service '"
                        "%1'. The service may no longer exists or it may "
                        "not be responding to XML/RPC requests.")
		.arg(controllerItem_->getName()),
		QMessageBox::Ok);

	    // Since we failed to apply the changes, lets close the dialog if it was not already done so by
	    // QDialog.
	    //
	    if (status == ParamEditor::Applied ||
                status == ParamEditor::AppliedOriginal)
		close();
	}

	emit valuesChanged(changed);
    }
}

void
ParamEditor::maybeReject()
{
    if (! apply_->isEnabled() ||
        QMessageBox::question(qApp->activeWindow(),
                              "Changes Pending",
                              "<p>Changes have not been applied to the "
                              "algorithm. Do you wish to continue and forget "
                              "the changes?.</p>",
                              QMessageBox::No | QMessageBox::Yes,
                              QMessageBox::No) == QMessageBox::Yes) {
	// Invoke QDialog::reject() to set the dialog's status value to QDialog::Reject. This will emit
	// QDialog::finished(), which will invoke our endEdit() routine.
	//
	reject();
    }
}

void
ParamEditor::maybeAccept()
{
    if (apply_->isEnabled()) {

	// Invoke QDialog::accept() to set the dialog's status value to QDialog::Accept. This will emit
	// QDialog::finished(), which will invoke our endEdit() routine.
	//
	accept();
    }
    else {
	close();
    }
}

void
ParamEditor::updateButtons()
{
    Logger::ProcLog log("updateButtons", Log());
    LOGINFO << std::endl;

    bool enabled = false;
    bool originalEnabled = false;
    for (int index = 0; index < parameters_.size(); ++index) {
	if (parameters_[index]->hasChanged())
	    enabled = true;

	XmlRpc::XmlRpcValue& vs(definition_[index]);
	bool isAdvanced(vs["advanced"]);
	if (! isAdvanced && parameters_[index]->notOriginal()) {
	    LOGDEBUG << "index: " << index << std::endl;
	    originalEnabled = true;
	}
    }

    apply_->setEnabled(enabled);
    undo_->setEnabled(enabled);
    original_->setEnabled(originalEnabled);
}

void
ParamEditor::apply()
{
    setResult(Applied);
    emit finished(Applied);
    updateButtons();
}

void
ParamEditor::applyOriginal()
{
    for (int index = 0; index < parameters_.size(); ++index)
	parameters_[index]->revertToOriginal();
    setResult(AppliedOriginal);
    emit finished(AppliedOriginal);
    updateButtons();
}

void
ParamEditor::undo()
{
    for (int index = 0; index < parameters_.size(); ++index)
	parameters_[index]->undoChange();
    updateButtons();
}

void
ParamEditor::done(int r)
{
    QDialog::done(r);
    updateButtons();
}
