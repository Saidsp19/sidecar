#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "ace/FILE_Connector.h"

#include "QtCore/QByteArray"
#include "QtCore/QDateTime"
#include "QtCore/QFileInfo"
#include "QtCore/QSettings"
#include "QtCore/QString"
#include "QtCore/QTextStream"
#include "QtCore/QTimer"
#include "QtGui/QFileDialog"
#include "QtGui/QInputDialog"
#include "QtGui/QMessageBox"
#include "QtGui/QProgressDialog"
#include "QtGui/QRegExpValidator"
#include "QtGui/QStatusBar"

#include "Configuration/Loader.h"
#include "GUI/AppBase.h"
#include "GUI/LogUtils.h"
#include "GUI/Writers.h"
#include "IO/ByteOrder.h"
#include "IO/Decoder.h"
#include "IO/MessageManager.h"
#include "IO/Writers.h"
#include "Messages/RadarConfig.h"
#include "Messages/Video.h"
#include "Time/TimeStamp.h"

#include "MainWindow.h"
#include "Utils.h"

using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::PRIEmitter;

inline void
degreesToRadians(double& v)
{
    v *= M_PI_4 / 45.0;
}
inline void
radiansToDegrees(double& v)
{
    v *= 45.0 / M_PI_4;
}

Logger::Log&
MainWindow::Log()
{
    Logger::Log& log_ = Logger::Log::Find("priemitter.MainWindow");
    return log_;
}

MainWindow::MainWindow() :
    MainWindowBase(), Ui::MainWindow(), file_(0), reader_(), writer_(0), timer_(new QTimer(this)), lastFile_(""),
    emitting_(false)
{
    setupUi(this);
    setFixedSize();
#ifdef __DEBUG__
    setWindowTitle("PRIEmitter (DEBUG)");
#else
    setWindowTitle("PRIEmitter");
#endif

    getApp()->setVisibleWindowMenuNew(false);
    getApp()->setVisibleWindowMenuMaximize(false);
    getApp()->setVisibleWindowMenuFullScreen(false);

    QSettings settings;
    name_->setText(settings.value("Name", "NegativeVideo").toString());
    lastName_ = name_->text();

    connectionType_->setCurrentIndex(settings.value("ConnectionType", kTCP).toInt());

    // The following should match valid IP addresses and host names.
    //
    QRegExp pattern("(\\b(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"
                    "\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"
                    "\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"
                    "\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b)|"
                    "(\\b([A-Za-z][-A-Za-z0-9]*\\.?){1,}\\b)");

    if (pattern.isValid()) {
        QRegExpValidator* validator = new QRegExpValidator(pattern, this);
        address_->setValidator(validator);
    } else {
        QMessageBox::information(this, "Invalid Host Name Pattern", pattern.errorString(), QMessageBox::Ok);
    }

    address_->setText(settings.value("Address", "237.1.2.100").toString());
    lastAddress_ = address_->text();
    address_->setEnabled(connectionType_->currentIndex() == kMulticast);
    frequency_->setValue(settings.value("Frequency", 500).toInt());

    loopAtEnd_->setCheckState(settings.value("LoopAtEnd", true).toBool() ? Qt::Checked : Qt::Unchecked);
    simAz_->setCheckState(settings.value("SimulatedAzimuth", false).toBool() ? Qt::Checked : Qt::Unchecked);

    startStop_->setEnabled(false);
    resetRate_->setEnabled(false);

    actionEmitterStart_->setEnabled(false);
    rewind_->setEnabled(false);
    actionEmitterRewind_->setEnabled(false);
    step_->setCurrentIndex(settings.value("Step", 0).toInt());

    for (int index = 0; index < kMaxRecentFiles; ++index) {
        QAction* action = recentFiles_[index] = new QAction(this);
        action->setShortcut(QKeySequence(QString("CTRL+%1").arg(index + 1)));
        connect(action, SIGNAL(triggered()), SLOT(openRecentFile()));
        menuRecentFiles_->addAction(action);
    }

    menuRecentFiles_->addSeparator();
    menuRecentFiles_->addAction(actionRecentFilesClear_);

    updateRecentFileActions();
    updateTimer();

    connect(timer_, SIGNAL(timeout()), SLOT(emitMessage()));
    if (!writer_) makeWriter();
}

void
MainWindow::on_connectionType__currentIndexChanged(int index)
{
    writerSubscriberCountChanged(0);
    address_->setEnabled(index == kMulticast);
    makeWriter();
    QSettings settings;
    settings.setValue("ConnectionType", index);
}

void
MainWindow::on_name__editingFinished()
{
    QString name = name_->text();
    if (name.isEmpty()) {
        name_->setText(lastName_);
        return;
    }

    if (lastName_ != name) {
        lastName_ = name;
        makeWriter();
        QSettings settings;
        settings.setValue("Name", name);
    }
}

void
MainWindow::on_address__editingFinished()
{
    QString address = address_->text();
    if (address.isEmpty()) {
        address_->setText(lastAddress_);
        return;
    }

    if (lastAddress_ != address) {
        lastAddress_ = address;
        makeWriter();
        QSettings settings;
        settings.setValue("Address", address);
    }
}

void
MainWindow::makeWriter()
{
    Logger::ProcLog log("makeWriter", Log());
    LOGDEBUG << std::endl;

    if (writer_) { removeWriter(); }

    writerSubscriberCountChanged(0);

    if (connectionType_->currentIndex() == kTCP) {
        writer_ = TCPMessageWriter::Make(name_->text(), Messages::Video::GetMetaTypeInfo().getName());

        if (!writer_) {
            QMessageBox::information(this, "Name Conflict",
                                     QString("The name requested for the service is already "
                                             "in use by another service."),
                                     QMessageBox::Ok);
            return;
        }
    } else {
        writer_ = UDPMessageWriter::Make(name_->text(), Messages::Video::GetMetaTypeInfo().getName(), address_->text());
    }

    connect(writer_, SIGNAL(published(const QString&, const QString&, uint16_t)),
            SLOT(writerPublished(const QString&, const QString&, uint16_t)));
    connect(writer_, SIGNAL(failure()), SLOT(writerFailure()));
    connect(writer_, SIGNAL(subscriberCountChanged(size_t)), SLOT(writerSubscriberCountChanged(size_t)));
}

void
MainWindow::removeWriter()
{
    if (writer_) {
        delete writer_;
        writer_ = 0;
    }
}

void
MainWindow::writerSubscriberCountChanged(size_t size)
{
    Logger::ProcLog log("writerSubscriberCountChanged", Log());
    LOGINFO << "new count:" << size << std::endl;
    connections_->setNum(int(size));
}

void
MainWindow::on_frequency__valueChanged(int value)
{
    Logger::ProcLog log("on_frequency__valueChanged", Log());
    LOGINFO << value << std::endl;
    QSettings settings;
    settings.setValue("Frequency", value);
    if (file_) { settings.setValue(file_->fileName() + "Frequency", value); }
    updateTimer();
}

void
MainWindow::updateTimer()
{
    if (emitting_) timer_->stop();
    timer_->setInterval(int(1000.0 / frequency_->value() + 0.5));
    if (emitting_) timer_->start();
}

void
MainWindow::on_resetRate__clicked()
{
    frequency_->setValue(fileFrequency_->text().toInt());
}

void
MainWindow::on_startStop__clicked()
{
    Logger::ProcLog log("on_startStop__clicked", Log());
    LOGINFO << emitting_ << std::endl;
    if (emitting_) {
        timer_->stop();
        emitting_ = false;
        statusBar()->showMessage("Stopped.", 5000);
        startStop_->setText("Start");
        actionEmitterStart_->setText("Start");
    } else {
        statusBar()->showMessage("Started.", 5000);
        startStop_->setText("Stop");
        actionEmitterStart_->setText("Stop");
        timer_->start();
        emitting_ = true;
    }
}

void
MainWindow::on_rewind__clicked()
{
    Logger::ProcLog log("on_rewind__clicked", Log());
    LOGINFO << std::endl;
    rewind();
}

void
MainWindow::on_actionFileOpen__triggered()
{
    static Logger::ProcLog log("on_actionFileOpen__triggered", Log());
    QString path = QFileDialog::getOpenFileName(this, "Choose a data file", lastFile_, "Data (*.hdr *.pri)");

    if (path.isEmpty()) {
        statusBar()->showMessage("Open canceled", 5000);
        return;
    }

    LOGINFO << "path: " << path << std::endl;
    openFile(path);
}

void
MainWindow::loadDone()
{
    delete progressDialog_;
    progressDialog_ = 0;
    QApplication::restoreOverrideCursor();
    startStop_->setEnabled(writer_ != 0);
    resetRate_->setEnabled(writer_ != 0);
}

void
MainWindow::openFile(const QString& path)
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QFileInfo fileInfo(path);

    if (emitting_) on_startStop__clicked();

    progressDialog_ = new QProgressDialog("Loading...", "Cancel", 0, 100, this);
    progressDialog_->setWindowModality(Qt::ApplicationModal);

    if (fileInfo.completeSuffix() == "pri") {
        if (!openPRIFile(fileInfo)) {
            loadDone();
            return;
        }
    } else if (fileInfo.completeSuffix() == "hdr") {
        if (!openLogFile(fileInfo)) {
            loadDone();
            return;
        }
    } else {
        statusBar()->showMessage("Unkown file type");
        loadDone();
        return;
    }

    if (progressDialog_->wasCanceled()) {
        statusBar()->showMessage("Canceled.", 5000);
    } else {
        statusBar()->showMessage("Finished.", 5000);
    }
    loadDone();

    actionEmitterStart_->setEnabled(true);
    rewind_->setEnabled(true);
    actionEmitterRewind_->setEnabled(true);
    setCurrentFile(path);

    shaftDelta_ = Messages::RadarConfig::GetShaftEncodingMax() / (M_PI * 2.0) * beamWidthValue_;
    simulatedShaft_ = 0.0;
}

void
MainWindow::on_actionEmitterStart__triggered()
{
    on_startStop__clicked();
}

void
MainWindow::on_actionEmitterRewind__triggered()
{
    on_rewind__clicked();
}

void
MainWindow::openConfigFile(const QFileInfo& fileInfo)
{
    // See if there is an XML configuration file that belongs with the data
    // file. It has the same path and file name except for an 'xml' suffix.
    //
    QDir folder(fileInfo.dir());
    QString fileName(fileInfo.baseName());
    fileName.append(".xml");
    QString configPath;
    if (folder.exists(fileName)) {
        configPath = folder.filePath(fileName);
    } else {
        configPath = ::getenv("SIDECAR_CONFIG");
    }

    Configuration::Loader loader;
    loader.loadRadarConfig(configPath);
}

bool
MainWindow::openLogFile(const QFileInfo& fileInfo)
{
    QFile headerFile(fileInfo.filePath());
    if (!headerFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        statusBar()->showMessage(QString("Failed to open file %1").arg(fileInfo.fileName()));
        return false;
    }

    openConfigFile(fileInfo);

    QTextStream in(&headerFile);
    float rangeMin;
    QString token;
    if ((in >> token >> token >> rangeMin).status()) {
        statusBar()->showMessage("Failed to read minimum range value", 5000);
        return false;
    }

    rangeMin_->setText(QString::number(rangeMin));

    float rangeMax;
    if ((in >> token >> token >> rangeMax).status()) {
        statusBar()->showMessage("Failed to read max range value", 5000);
        return false;
    }

    rangeMax_->setText(QString::number(rangeMax));

    int samplesPerPulse;
    if ((in >> token >> samplesPerPulse).status()) {
        statusBar()->showMessage("Failed to read samples/pulse value", 5000);
        return false;
    }

    if (samplesPerPulse == 0) {
        statusBar()->showMessage("Samples/pulse value is zero", 5000);
        return false;
    }

    pulseSize_->setText(QString::number(samplesPerPulse));

    Time::TimeStamp deltaSum;
    Time::TimeStamp lastTime;

    double when;
    double azimuthStart;
    double azimuthEnd;
    double beamWidth = 0.0;

    using VideoVector = std::vector<Messages::Video::Ref>;
    VideoVector messages;

    Messages::VMEDataMessage vme;
    vme.header.msgDesc = ((Messages::VMEHeader::kPackedReal << 16) | Messages::VMEHeader::kIRIGValidMask |
                          Messages::VMEHeader::kAzimuthValidMask | Messages::VMEHeader::kPRIValidMask);
    vme.header.pri = 0;
    vme.header.timeStamp = 0;

    while (!(in >> when >> azimuthStart >> azimuthEnd >> token).status()) {
        degreesToRadians(azimuthStart);
        degreesToRadians(azimuthEnd);

        vme.header.irigTime = when;
        ++vme.header.pri;

        if (!beamWidth) {
            beamWidth = azimuthEnd - azimuthStart;
            if (beamWidth < 0.0) beamWidth += M_PI * 2.0;
        }

        vme.header.azimuth =
            uint32_t(::rint(azimuthStart / (M_PI * 2.0) * Messages::RadarConfig::GetShaftEncodingMax()));

        Messages::Video::Ref msg(Messages::Video::Make("PRIEmitter", vme, samplesPerPulse));
        msg->setCreatedTimeStamp(Time::TimeStamp(when));

        if (messages.empty()) {
            lastTime = msg->getCreatedTimeStamp();
        } else {
            Time::TimeStamp delta(msg->getCreatedTimeStamp());
            delta -= lastTime;
            deltaSum += delta;
            lastTime = msg->getCreatedTimeStamp();
        }

        messages.push_back(msg);
    }

    QFileInfo dataFileInfo(fileInfo.dir(), fileInfo.baseName() + ".log");

    QFile dataFile(dataFileInfo.filePath());
    if (!dataFile.open(QIODevice::ReadOnly)) {
        statusBar()->showMessage(QString("Failed to open data file %1").arg(dataFileInfo.fileName()));
        return false;
    }

    QByteArray data = dataFile.readAll();
    QFileInfo priFileInfo(dataFileInfo.dir(), dataFileInfo.baseName() + ".pri");

    ::unlink(priFileInfo.filePath().toStdString().c_str());
    ACE_FILE_Addr outputAddr(priFileInfo.filePath().toStdString().c_str());
    ACE_FILE_Connector fileConnector;
    IO::FileWriter::Ref writer(new IO::FileWriter);
    if (fileConnector.connect(writer->getDevice(), outputAddr) == -1) {
        statusBar()->showMessage(QString("Failed to open PRI output file %1").arg(priFileInfo.fileName()));
        return false;
    }

    ByteOrder byteOrder;
    // byteOrder.setDecodingByteOrder(ByteOrder::kLittleEndian);
    short* ptr = reinterpret_cast<short*>(data.data());

    short valueMin = 32767;
    short valueMax = -valueMin;

    VideoVector::iterator nextMessage = messages.begin();
    VideoVector::iterator end = messages.end();
    while (nextMessage != end) {
        Messages::Video::Ref msg = *nextMessage++;
        for (int count = samplesPerPulse; count; --count) {
            byteOrder.decode(ptr, sizeof(short));
            if (*ptr < valueMin) valueMin = *ptr;
            if (*ptr > valueMax) valueMax = *ptr;
            msg->push_back(*ptr++);
        }

        IO::MessageManager mgr(msg);
        if (!writer->write(mgr.getMessage())) { statusBar()->showMessage(QString("Failed to encode message"), 5000); }
    }

    writer->getDevice().close();

    return openPRIFile(priFileInfo);
}

bool
MainWindow::openPRIFile(const QFileInfo& fileInfo)
{
    if (file_) {
        if (reader_) { reader_->reset(); }
        file_->close();
        delete file_;
        file_ = 0;
    }

    file_ = new QFile(fileInfo.filePath());
    if (!file_->open(QFile::ReadOnly)) {
        statusBar()->showMessage(QString("Failed to open file %1").arg(fileInfo.fileName()));
        delete file_;
        file_ = 0;
        return false;
    }

    reader_ = IO::FileReader::Make();
    reader_->getDevice().set_handle(file_->handle());

    openConfigFile(fileInfo);

    Time::TimeStamp deltaSum;
    Time::TimeStamp lastTime;

    double fileSize = file_->size();
    size_t messageCount = 0;
    size_t numGates = 0;
    short valueMin = 32767;
    short valueMax = -valueMin;
    while (reader_->fetchInput()) {
        if (reader_->isMessageAvailable()) {
            IO::Decoder decoder(reader_->getMessage());
            Messages::Video::Ref msg(decoder.decode<Messages::Video>());
            if (messageCount == 0) {
                lastTime = msg->getCreatedTimeStamp();
                numGates = msg->size();
                std::clog << lastTime << std::endl;
            } else {
                Time::TimeStamp delta(msg->getCreatedTimeStamp());
                delta -= lastTime;
                deltaSum += delta;
                lastTime = msg->getCreatedTimeStamp();
                if (msg->size() > numGates) { numGates = msg->size(); }
            }

            ++messageCount;
            short min = *(std::min_element(msg->begin(), msg->end()));
            if (min < valueMin) valueMin = min;
            short max = *(std::max_element(msg->begin(), msg->end()));
            if (max > valueMax) valueMax = max;
        }

        double percentComplete = reader_->getDevice().tell() / fileSize;
        progressDialog_->setValue(int(percentComplete * 100.0));
        qApp->processEvents();
        if (progressDialog_->wasCanceled()) { break; }
    }

    rewind();

    if (messageCount) {
        if (messageCount > 1) {
            double delta = deltaSum.asDouble();
            delta /= (messageCount - 1);
            if (delta != 0.0) {
                int freq = int(1.0 / delta);
                fileFrequency_->setText(QString::number(freq));
                QSettings settings;
                frequency_->setValue(settings.value(file_->fileName() + "Frequency", freq).toInt());
            }
        }

        rangeMin_->setText(QString::number(Messages::RadarConfig::GetRangeMin_deprecated()));
        rangeMax_->setText(QString::number(Messages::RadarConfig::GetRangeMax()));

        pulseSize_->setText(QString::number(numGates));
        beamWidthValue_ = Messages::RadarConfig::GetBeamWidth();
        beamWidth_->setText(QString::number(beamWidthValue_));
    } else {
        rangeMin_->setText("---");
        rangeMax_->setText("---");
        pulseSize_->setText("---");
        beamWidth_->setText("---");
        fileFrequency_->setText("---");
    }

    valueMin_->setText(QString::number(valueMin));
    valueMax_->setText(QString::number(valueMax));
    pulseCount_->setText(QString::number(messageCount));

    return true;
}

void
MainWindow::emitMessage()
{
    static Logger::ProcLog log("emitMessages", Log());

    while (1) {
        if (!reader_->fetchInput()) {
            rewind();
            if (!loopAtEnd_->isChecked()) {
                on_startStop__clicked();
                return;
            }

            LOGINFO << "looping" << std::endl;
            QString when("Looping");
            statusBar()->showMessage("Looping", 5000);
            continue;
        }

        if (reader_->isMessageAvailable()) break;
    }

    IO::Decoder decoder(reader_->getMessage());
    Messages::Video::Ref msg(decoder.decode<Messages::Video>());
    msg->setCreatedTimeStamp(Time::TimeStamp::Now());

    if (simAz_->isChecked()) {
        msg->getRIUInfo().shaftEncoding = uint32_t(::rint(simulatedShaft_));
        simulatedShaft_ += shaftDelta_;
        if (simulatedShaft_ > Messages::RadarConfig::GetShaftEncodingMax())
            simulatedShaft_ -= Messages::RadarConfig::GetShaftEncodingMax();
    }

    IO::MessageManager mgr(msg);
    if (!writer_->writeMessage(mgr)) { makeWriter(); }
}

void
MainWindow::openRecentFile()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        QString path(action->data().toString());
        openFile(path);
    }
}

void
MainWindow::setCurrentFile(const QString& path)
{
    lastFile_ = path;
    setWindowTitle(QString("PRIEmitter - %1").arg(strippedName(path)));
    QSettings settings;
    QStringList files = settings.value("RecentFileList").toStringList();
    files.removeAll(path);
    files.prepend(path);
    while (files.size() > kMaxRecentFiles) files.removeLast();
    settings.setValue("RecentFileList", files);
    updateRecentFileActions();
}

void
MainWindow::updateRecentFileActions()
{
    QSettings settings;
    QStringList files = settings.value("RecentFileList").toStringList();
    for (int index = 0; index < files.size(); ++index) {
        QString text(QString("&%1 %2").arg(index + 1).arg(strippedName(files[index])));
        recentFiles_[index]->setText(text);
        recentFiles_[index]->setData(files[index]);
        recentFiles_[index]->setVisible(true);
    }

    for (int index = files.size(); index < kMaxRecentFiles; ++index) recentFiles_[index]->setVisible(false);

    menuRecentFiles_->setEnabled(files.size() > 0);
}

QString
MainWindow::strippedName(const QString& path)
{
    return QFileInfo(path).fileName();
}

void
MainWindow::on_actionRecentFilesClear__triggered()
{
    QSettings settings;
    settings.setValue("RecentFileList", QStringList());
    updateRecentFileActions();
}

void
MainWindow::on_step__currentIndexChanged(int index)
{
    QSettings settings;
    settings.setValue("Step", index);
}

void
MainWindow::on_loopAtEnd__toggled(bool state)
{
    QSettings settings;
    settings.setValue("LoopAtEnd", state);
}

void
MainWindow::writerPublished(const QString& serviceName, const QString& serverAddress, uint16_t port)
{
    static Logger::ProcLog log("writerPublished", Log());
    LOGDEBUG << serviceName << std::endl;

    QString address = QString("%1:%2").arg(serverAddress).arg(port);
    address_->setToolTip(address);
    connectionType_->setToolTip(address);
    statusBar()->showMessage(QString("Server address: %1").arg(address));

    if (serviceName != name_->text()) {
        name_->setText(serviceName);
        lastName_ = serviceName;
        QMessageBox::information(this, "Name Conflict",
                                 QString("The name requested for the service is already in use "
                                         "by another service. The new name is '%1'")
                                     .arg(serviceName),
                                 QMessageBox::Ok);
    }
}

void
MainWindow::writerFailure()
{
    QMessageBox::information(this, "Name Conflict",
                             QString("The name requested for the service is already in use by "
                                     "another service."),
                             QMessageBox::Ok);
}

void
MainWindow::on_simAz__toggled(bool checked)
{
    QSettings settings;
    settings.setValue("SimulatedAzimuth", checked);
}

void
MainWindow::rewind()
{
    // Reset the stream and rewind the device.
    //
    reader_->reset();
    ::lseek(reader_->getDevice().get_handle(), 0, SEEK_SET);
}
