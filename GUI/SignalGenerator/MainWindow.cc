#include <sys/time.h>

#include <cmath>

#include "QtCore/QSettings"
#include "QtCore/QString"
#include "QtCore/QTimer"
#include "QtGui/QCloseEvent"
#include "QtGui/QMessageBox"
#include "QtGui/QRegExpValidator"
#include "QtGui/QStatusBar"

#include "GUI/LogUtils.h"
#include "GUI/modeltest.h"
#include "GUI/Writers.h"
#include "Utils/Utils.h"

#include "Emitter.h"
#include "GeneratorConfiguration.h"
#include "GeneratorConfigurationsModel.h"
#include "MainWindow.h"

#include "ui_MainWindow.h"

using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::SignalGenerator;

struct MainWindow::Private
{
    Ui::MainWindow* gui_;
    Emitter* emitter_;
    GeneratorConfigurationsModel* model_;
    double priRate_;
    QString lastName_;
    QString lastAddress_;
    GeneratorConfiguration* active_;
    bool initialized_;

    /** Elements for creating messages.
     */
    size_t bufferSize_;
    Messages::VMEDataMessage vme_;
    double alpha_;
    double beta_;
    double shaftMovePerMessage_;
    bool doComplex_;
    Time::TimeStamp clock_;
    GeneratorConfiguration::AmplitudeVector accumulator_;
    QTimer* timer_;
};

Logger::Log&
MainWindow::Log()
{
    Logger::Log& log_ = Logger::Log::Find("SignalGenerator.MainWindow");
    return log_;
}

MainWindow::MainWindow()
    : MainWindowBase(), p_(new Private)
{
    p_->gui_ = new Ui::MainWindow;
    p_->emitter_ = new Emitter;
    p_->model_ = new GeneratorConfigurationsModel(this);
#ifdef __DEBUG__
    new ModelTest(p_->model_, this);
#endif
    
    p_->active_ = 0;
    p_->initialized_ = false;
    p_->gui_->setupUi(this);
    p_->timer_ = new QTimer(this);
    p_->timer_->setInterval(0);
    connect(p_->timer_, SIGNAL(timeout()), SLOT(generateOneMessage()));
#ifdef __DEBUG__
    setWindowTitle("Signal Generator (DEBUG)");
#else
    setWindowTitle("Signal Generator");
#endif
    getApp()->setVisibleWindowMenuNew(false);

    QListView* generators = p_->gui_->generators_;
    generators->setModel(p_->model_);
    generators->setFlow(QListView::LeftToRight);
    generators->setUniformItemSizes(true);
    generators->setEditTriggers(QAbstractItemView::NoEditTriggers);
    generators->setSelectionBehavior(QAbstractItemView::SelectRows);
    generators->setSelectionMode(QAbstractItemView::SingleSelection);
    generators->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    connect(generators->selectionModel(),
            SIGNAL(currentRowChanged(const QModelIndex&,
                                     const QModelIndex&)),
            SLOT(generatorSelectionCurrentChanged(const QModelIndex&,
                                                  const QModelIndex&)));

    p_->gui_->removeGenerator_->setEnabled(false);
    p_->gui_->startStop_->setEnabled(false);

    // The following should match valid multicast IP addresses and host names.
    //
    QRegExp pattern("(\\b(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"
                    "\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"
                    "\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"
                    "\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b)|"
                    "(\\b([A-Za-z][-A-Za-z0-9]*\\.?){1,}\\b)");

    QRegExpValidator* validator = new QRegExpValidator(pattern, this);
    p_->gui_->address_->setValidator(validator);
}

void
MainWindow::restoreFromSettings(QSettings& settings)
{
    Super::restoreFromSettings(settings);
    p_->gui_->sampleFrequency_->setValue(
	settings.value("SampleFrequency", 1).toInt());
    p_->gui_->sampleFrequencyScale_->setCurrentIndex(
	settings.value("SampleFrequencyScale", 2).toInt());
    p_->gui_->sampleCount_->setValue(
	settings.value("SampleCount", 1000).toInt());
    p_->gui_->radialCount_->setValue(
	settings.value("RadialCount", 100).toInt());
    p_->gui_->doComplex_->setChecked(
	settings.value("DoComplex", true).toBool());

    p_->gui_->name_->setText(
	settings.value("Name", "Negative Video").toString());
    p_->gui_->connectionType_->setCurrentIndex(
	settings.value("ConnectionType", kMulticast).toInt());
    p_->gui_->address_->setText(
	settings.value("Address", "237.1.2.100").toString());
    p_->lastAddress_ = p_->gui_->address_->text();
    p_->gui_->address_->setEnabled(
	p_->gui_->connectionType_->currentIndex() == kMulticast);

    p_->gui_->messageCount_->setValue(
	settings.value("MessageCount", 50).toInt());
    p_->gui_->emitterFrequency_->setValue(
	settings.value("EmitterFrequency", 360).toInt());
    p_->gui_->emitterFrequencyScale_->setCurrentIndex(
	settings.value("EmitterFrequencyScale", 0).toInt());

    int count = settings.beginReadArray("Generators");
    for (int index = 0; index < count; ++index) {
	settings.setArrayIndex(index);
	GeneratorConfiguration* obj =
	    new GeneratorConfiguration(getSampleFrequency());
	addGenerator(obj);
	obj->restoreFromSettings(settings);
    }
    settings.endArray();
}

void
MainWindow::showAndRaise()
{
    Super::showAndRaise();

    if (p_->gui_->name_->text().isEmpty())
	p_->gui_->name_->setText("SignalGenerator");
    if (p_->gui_->address_->text().isEmpty())
	p_->gui_->address_->setText("237.1.2.100");
    if (p_->model_->rowCount() == 0)
	addGenerator(new GeneratorConfiguration(getSampleFrequency()));

    p_->initialized_ = true;
    generateMessages();
    publish();
}

void
MainWindow::saveToSettings(QSettings& settings)
{
    settings.setValue("SampleFrequency", p_->gui_->sampleFrequency_->value());
    settings.setValue("SampleFrequencyScale",
                      p_->gui_->sampleFrequencyScale_->currentIndex());
    settings.setValue("SampleCount", p_->gui_->sampleCount_->value());
    settings.setValue("RadialCount", p_->gui_->radialCount_->value());
    settings.setValue("DoComplex", p_->gui_->doComplex_->isChecked());
    settings.setValue("Name", p_->gui_->name_->text());
    settings.setValue("ConnectionType",
                      p_->gui_->connectionType_->currentIndex());
    settings.setValue("Address", p_->gui_->address_->text());
    settings.setValue("MessageCount", p_->gui_->messageCount_->value());
    settings.setValue("EmitterFrequency",
                      p_->gui_->emitterFrequency_->value());
    settings.setValue("EmitterFrequencyScale",
                      p_->gui_->emitterFrequencyScale_->currentIndex());

    settings.beginWriteArray("Generators", p_->model_->rowCount());
    for (int index = 0; index < p_->model_->rowCount(); ++index) {
	settings.setArrayIndex(index);
	GeneratorConfiguration* obj = p_->model_->getConfiguration(index);
	obj->saveToSettings(settings);
    }
    settings.endArray();
}

void
MainWindow::on_addGenerator__clicked()
{
    GeneratorConfiguration* obj;
    if (! p_->active_) {
	obj = new GeneratorConfiguration(getSampleFrequency());
    }
    else {
	obj = new GeneratorConfiguration(p_->active_);
    }

    addGenerator(obj);
    generateMessages();
}

void
MainWindow::addGenerator(GeneratorConfiguration* obj)
{
    connect(this, SIGNAL(sampleFrequencyChanged(double)), obj,
            SLOT(setSampleFrequency(double)));
    connect(this, SIGNAL(reset()), obj, SLOT(reset()));
    connect(obj, SIGNAL(activeConfiguration(GeneratorConfiguration*)),
            SLOT(activeConfigurationChanged(GeneratorConfiguration*)));
    connect(obj, SIGNAL(configurationChanged()),
            SLOT(generateMessages()));

    QModelIndex index = p_->model_->add(obj);
    p_->gui_->generators_->setIndexWidget(index, obj);
    p_->gui_->generators_->setCurrentIndex(index);
    p_->gui_->startStop_->setEnabled(true);
}

void
MainWindow::on_removeGenerator__clicked()
{
    if (p_->active_) {
	p_->gui_->removeGenerator_->setEnabled(false);
	GeneratorConfiguration* obj = p_->active_;
	p_->active_ = 0;
	p_->model_->remove(obj);
	p_->gui_->startStop_->setEnabled(p_->model_->rowCount());
	generateMessages();
    }
}

void
MainWindow::generatorSelectionCurrentChanged(const QModelIndex& now,
                                             const QModelIndex& old)
{
    Logger::ProcLog log("generatorSelectionCurrentChanged", Log());
    LOGINFO << "old: " << old.row() << " now: " << now.row() << std::endl;
    if (old.isValid()) {
	p_->model_->GetObject(old)->setSelected(false);
	p_->active_ = 0;
    }

    if (now.isValid()) {
	p_->active_ = p_->model_->GetObject(now);
	p_->active_->setSelected(true);
    }

    p_->gui_->removeGenerator_->setEnabled(now.isValid());
}

void
MainWindow::activeConfigurationChanged(GeneratorConfiguration* active)
{
    int row = p_->model_->getRowFor(active);
    if (row != -1) {
	p_->gui_->generators_->setCurrentIndex(p_->model_->index(row));
    }
}

void
MainWindow::on_sampleCount__valueUpdated(int value)
{
    generateMessages();
}

void
MainWindow::on_radialCount__valueUpdated(int value)
{
    generateMessages();
}

void
MainWindow::on_doComplex__clicked(bool state)
{
    generateMessages();
}

void
MainWindow::on_name__editingFinished()
{
    Logger::ProcLog log("on_name__editingFinished", Log());
    QString name = p_->gui_->name_->text();
    LOGINFO << "name: " << name << std::endl;

    if (name.isEmpty()) {
	p_->gui_->name_->setText(p_->lastName_);
	return;
    }

    if (p_->lastName_ != name) {
	p_->lastName_ = name;
	publish();
    }
}

void
MainWindow::on_connectionType__currentIndexChanged(int index)
{
    p_->gui_->address_->setEnabled(index == kMulticast);
    publish();
}

void
MainWindow::on_address__editingFinished()
{
    Logger::ProcLog log("on_address__editingFinished", Log());
    QString address = p_->gui_->address_->text();
    LOGINFO << "address: " << address << std::endl;

    if (address.isEmpty()) {
	p_->gui_->address_->setText(p_->lastAddress_);
	return;
    }

    if (p_->lastAddress_ != address) {
	p_->lastAddress_ = address;
	publish();
    }
}

void
MainWindow::on_emitterFrequency__valueUpdated(int value)
{
    p_->emitter_->setFrequency(getEmitterFrequency());
}

void
MainWindow::on_emitterFrequencyScale__currentIndexChanged(int index)
{
    p_->emitter_->setFrequency(getEmitterFrequency());
}

void
MainWindow::on_sampleFrequency__valueUpdated(double value)
{
    emit sampleFrequencyChanged(getSampleFrequency());
    generateMessages();
}

void
MainWindow::on_sampleFrequencyScale__currentIndexChanged(int index)
{
    emit sampleFrequencyChanged(getSampleFrequency());
    generateMessages();
}

void
MainWindow::on_startStop__clicked()
{
    if (p_->emitter_->isRunning()) {
	stop();
    }
    else {
	start();
    }
}

void
MainWindow::start()
{
    if (p_->emitter_->start())
	p_->gui_->startStop_->setText("Stop");
}

void
MainWindow::rewind()
{
    p_->emitter_->rewind();
}

void
MainWindow::stop()
{
    p_->emitter_->stop();
    p_->gui_->startStop_->setText("Start");
}

void
MainWindow::on_messageCount__valueUpdated(int value)
{
    generateMessages();
}

void
MainWindow::generateMessages()
{
    Logger::ProcLog log("generateMessages", Log());

    if (! p_->initialized_) return;

    p_->vme_.header.msgDesc = (Messages::VMEHeader::kIRIGValidMask |
                               Messages::VMEHeader::kAzimuthValidMask |
                               Messages::VMEHeader::kPRIValidMask);

    p_->doComplex_ = p_->gui_->doComplex_->isChecked();
    if (p_->doComplex_) {
	p_->vme_.header.msgDesc |= (Messages::VMEHeader::kPackedIQ << 16);
    }
    else {
	p_->vme_.header.msgDesc |= (Messages::VMEHeader::kPackedReal << 16);
    }

    p_->priRate_ = 1.0 / getEmitterFrequency();

    p_->vme_.header.pri = 0;
    p_->vme_.header.timeStamp = 0;
    p_->vme_.header.irigTime = 0;
    p_->vme_.header.azimuth = 0;
    p_->vme_.rangeMin = p_->gui_->rangeMin_->value();
    p_->vme_.rangeFactor = (p_->gui_->rangeMax_->value() -
                            p_->gui_->rangeMin_->value()) /
	p_->gui_->sampleCount_->value();

    p_->bufferSize_ = p_->gui_->sampleCount_->value();
    if (p_->doComplex_) p_->bufferSize_ *= 2;

    p_->accumulator_.resize(p_->bufferSize_, 0.0);
    p_->clock_ = Time::TimeStamp(0.0);

    int valueMin = p_->gui_->valueMin_->value();
    int valueMax = p_->gui_->valueMax_->value();

    int activeCount = 0;
    for (int index = 0; index < p_->model_->rowCount(); ++index) {
	GeneratorConfiguration* cfg = p_->model_->getConfiguration(index);
	if (cfg->isEnabled()) {
	    ++activeCount;
	    cfg->reset();
	}
    }

    // Calculate the coefficients in the linear transform from amplitudes to sample values. Amplitudes values
    // span [-1 * rowCount, 1 * rowCount], while sample values span [valueMin, valueMax]: alpha = (Vmax -
    // Vmin) / (Amax - Amin) = (Vmax - Vmin) / 2 V = (A / rowCount - Amin) * alpha + Vmin Simplifying
    // gets us: beta = -Amin * alpha + Vmin = alpha + Vmin V = A * alpha / rowCount + beta
    //
    p_->alpha_ = (valueMax - valueMin) / 2.0;
    p_->beta_ = valueMin + p_->alpha_;
    p_->alpha_ /= activeCount;
    p_->shaftMovePerMessage_ = 65536.0 / p_->gui_->radialCount_->value();

    LOGDEBUG << "alpha: " << p_->alpha_ << " beta: " << p_->beta_ << std::endl;

    p_->emitter_->clear();
    if (! p_->timer_->isActive()) {
	p_->timer_->start();
    }

    generateOneMessage();
}

void
MainWindow::generateOneMessage()
{
    static Logger::ProcLog log("generateOneMessage", Log());

    if (p_->doComplex_) {
	for (int index = 0; index < p_->model_->rowCount(); ++index) {
	    GeneratorConfiguration* cfg = p_->model_->getConfiguration(index);
	    if (cfg->isEnabled())
		cfg->complexAddTo(p_->accumulator_);
	}
    }
    else {
	for (int index = 0; index < p_->model_->rowCount(); ++index) {
	    GeneratorConfiguration* cfg = p_->model_->getConfiguration(index);
	    if (cfg->isEnabled())
		cfg->normalAddTo(p_->accumulator_);
	}
    }

    double azimuth = p_->shaftMovePerMessage_ * p_->vme_.header.pri;
    p_->vme_.header.azimuth = uint32_t(::rint(azimuth)) % 65536;

    ++p_->vme_.header.pri;
    Messages::Video::Ref msg = Messages::Video::Make("SignalGenerator",
                                                     p_->vme_,
                                                     p_->bufferSize_);
    msg->setCreatedTimeStamp(p_->clock_);
    p_->clock_ += p_->priRate_;
    p_->vme_.header.irigTime += p_->priRate_;

    for (size_t index = 0; index < p_->bufferSize_; ++index) {
	double value = p_->accumulator_[index] * p_->alpha_ + p_->beta_;
	msg->push_back(int(::rint(value)));
	p_->accumulator_[index] = 0.0;
    }

    p_->emitter_->addMessage(msg);

    if ((p_->vme_.header.pri % 50) == 0)
	statusBar()->showMessage(
	    QString("Generated message %1").arg(p_->vme_.header.pri));

    if (p_->vme_.header.pri == uint32_t(p_->gui_->messageCount_->value())) {
	statusBar()->showMessage("Done.", 5000);
	p_->timer_->stop();
    }
}

void
MainWindow::on_rewind__clicked()
{
    rewind();
}

void
MainWindow::writerPublished(const QString& serviceName, const QString& host,
                            uint16_t port)
{
    static Logger::ProcLog log("writerPublished", Log());
    LOGDEBUG << serviceName << std::endl;
    statusBar()->showMessage(QString("Server started on %1/%2")
                             .arg(host).arg(port));
    if (serviceName != p_->gui_->name_->text()) {
	p_->gui_->name_->setText(serviceName);
	QMessageBox::information(
	    this,
	    "Name Conflict",
	    QString(
		"The name requested for the service is already in use by "
		"another service. The new name is '%1'").arg(serviceName),
	    QMessageBox::Ok);
    }
}

void
MainWindow::writerSubscriberCountChanged(size_t size)
{
    p_->gui_->connections_->setNum(int(size));
}

void
MainWindow::publish()
{
    if (! p_->initialized_) return;

    MessageWriter* writer = p_->emitter_->setPublisherInfo(
	p_->gui_->name_->text(),
	ConnectionType(p_->gui_->connectionType_->currentIndex()),
	p_->gui_->address_->text());

    connect(writer,
            SIGNAL(published(const QString&, const QString&, uint16_t)),
            SLOT(writerPublished(const QString&, const QString&,
                                 uint16_t)));
}

size_t
MainWindow::getEmitterFrequency() const
{
    return size_t(
	::pow(10.0, p_->gui_->emitterFrequencyScale_->currentIndex() * 3.0) *
	p_->gui_->emitterFrequency_->value());
}

double
MainWindow::getSampleFrequency() const
{
    return ::pow(10.0, p_->gui_->sampleFrequencyScale_->currentIndex() * 3.0)
	* p_->gui_->sampleFrequency_->value();
}

void
MainWindow::closeEvent(QCloseEvent* event)
{
    App* app = getApp();
    
    // Since we are the only window that should be alive, tell the application to quit. Only do this if the
    // application is not already in the process of shutting down.
    //
    if (! app->isQuitting()) {
	event->ignore();
	QTimer::singleShot(0, app, SLOT(applicationQuit()));
	return;
    }

    Super::closeEvent(event);
}
