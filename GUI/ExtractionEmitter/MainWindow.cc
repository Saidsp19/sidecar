#include <sys/time.h>

#include <cmath>

#include "QtCore/QItemSelectionModel"
#include "QtCore/QSettings"
#include "QtCore/QString"
#include "QtGui/QRegExpValidator"
#include "QtWidgets/QHeaderView"
#include "QtWidgets/QInputDialog"
#include "QtWidgets/QMessageBox"
#include "QtWidgets/QStatusBar"

#include "GUI/LogUtils.h"
#include "GUI/Writers.h"
#include "IO/MessageManager.h"
#include "IO/Readers.h"
#include "IO/Writers.h"
#include "Messages/Extraction.h"

#include "GUI/Utils.h"

#include "MainWindow.h"
#include "ViewModel.h"

using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::ExtractionEmitter;

Logger::Log&
MainWindow::Log()
{
    Logger::Log& log_ = Logger::Log::Find("ExtractionEmitter.MainWindow");
    return log_;
}

MainWindow::MainWindow() :
    MainWindowBase(), Ui::MainWindow(), writer_(0), model_(new ViewModel(this)),
    entryChanger_(new ViewModelChanger(this))
{
    setupUi(this);
    setFixedSize();
    setWindowTitle("Extractions Emitter");
    getApp()->setVisibleWindowMenuNew(false);
    getApp()->setVisibleWindowMenuMaximize(false);
    getApp()->setVisibleWindowMenuFullScreen(false);

    send_->setEnabled(false);
    pending_->setModel(model_);
    pending_->setSelectionBehavior(QAbstractItemView::SelectRows);
    pending_->setSelectionMode(QAbstractItemView::SingleSelection);
    pending_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    entryChanger_->initialize(model_, range_, azimuth_);

    connect(pending_->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)), this,
            SLOT(pendingCurrentChanged(const QModelIndex&, const QModelIndex&)));

    // The following should match valid IP addresses and host names.
    //
    QRegExp pattern("(\\b(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"
                    "\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"
                    "\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"
                    "\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b)|"
                    "(\\b([A-Za-z][-A-Za-z0-9]*\\.?){1,}\\b)");

    if (pattern.isValid()) {
        QRegExpValidator* validator = new QRegExpValidator(pattern, this);
        hostName_->setValidator(validator);
    } else {
        QMessageBox::information(this, "Invalid Host Name Pattern", pattern.errorString(), QMessageBox::Ok);
    }

    QSettings settings;
    name_->setText(settings.value("Name", "Extractions").toString());

    connectionType_->setCurrentIndex(settings.value("ConnectionType", kTCP).toInt());

    hostName_->setText(settings.value("HostName", "237.1.2.100").toString());
    lastHostName_ = hostName_->text();

    if (!writer_) makeWriter();
}

void
MainWindow::on_connectionType__currentIndexChanged(int index)
{
    if (index == kMulticast) {
        connections_->setText("Connections: N/A");
    } else {
        connections_->setText("Connections: 0");
    }

    hostName_->setEnabled(index == kMulticast);
    makeWriter();

    QSettings settings;
    settings.setValue("ConnectionType", index);
}

void
MainWindow::on_hostName__editingFinished()
{
    QString newName = hostName_->text();
    if (newName.isEmpty()) {
        hostName_->setText(lastHostName_);
        return;
    }

    if (lastHostName_ != newName) {
        lastHostName_ = newName;
        makeWriter();
    }

    QSettings settings;
    settings.setValue("HostName", hostName_->text());
}

void
MainWindow::makeWriter()
{
    if (writer_) removeWriter();

    if (connectionType_->currentIndex() == kTCP) {
        writer_ = TCPMessageWriter::Make(name_->text(), Messages::Extractions::GetMetaTypeInfo().getName());
    } else {
        writer_ = UDPMessageWriter::Make(name_->text(), Messages::Extractions::GetMetaTypeInfo().getName(),
                                         hostName_->text());
    }

    connect(writer_, SIGNAL(published(const QString&, const QString&, uint16_t)),
            SLOT(writerPublished(const QString&, const QString&, uint16_t)));
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
    connections_->setText(QString("Connections: %1").arg(size));
}

void
MainWindow::pendingCurrentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    static Logger::ProcLog log("pendingCurrentChanged", Log());
    LOGDEBUG << current.row() << '/' << current.column() << ' ' << previous.row() << '/' << previous.column()
             << std::endl;

    if (previous.isValid() && previous.row() < model_->rowCount()) { entryChanger_->setRow(QModelIndex().row()); }

    if (current.isValid() && current.row() < model_->rowCount()) { entryChanger_->setRow(current.row()); }
}

void
MainWindow::on_editName__clicked()
{
    Logger::ProcLog log("on_editName__clicked", Log());
    QString oldName(name_->text());
    bool ok;
    QString newName(
        QInputDialog::getText(this, "Channel Name", "Enter new channel name:", QLineEdit::Normal, oldName, &ok));
    if (ok && !newName.isEmpty()) {
        LOGDEBUG << newName << std::endl;
        QSettings settings;
        settings.setValue("Name", newName);
        name_->setText(newName);
        writer_->setServiceName(newName);
    }
}

void
MainWindow::on_range__valueChanged(int value)
{
    QString tip(QString("%1 km").arg(value));
    range_->setToolTip(tip);
    tip.prepend("Range: ");
    range_->setStatusTip(tip);
    statusBar()->showMessage(tip, 5000);
}

void
MainWindow::on_azimuth__valueChanged(int value)
{
    QString tip(DegreesToString(value));
    azimuth_->setToolTip(tip);
    tip.prepend("Azimuth: ");
    azimuth_->setStatusTip(tip);
    statusBar()->showMessage(tip, 5000);
}

void
MainWindow::on_add__clicked()
{
    model_->add(range_->value(), azimuth_->value());
    pending_->selectRow(model_->rowCount() - 1);
    send_->setEnabled(true);
}

void
MainWindow::on_clear__clicked()
{
    model_->clear();
    send_->setEnabled(false);
}

void
MainWindow::on_send__clicked()
{
    static Logger::ProcLog log("on_send__clicked", Log());

    Messages::Extractions::Ref msg;
    if (model_->rowCount() == 0) {
        msg = Messages::Extractions::Make("ExractionEmitter", Messages::Header::Ref());
        msg->push_back(
            Messages::Extraction(Time::TimeStamp::Now(), range_->value(), azimuth_->value() * M_PI / 180.0, 0.0));
    } else {
        msg = model_->getEntries();
    }

    IO::MessageManager mgr(msg);
    writer_->writeMessage(mgr);
}

void
MainWindow::writerPublished(const QString& serviceName, const QString& host, uint16_t port)
{
    static Logger::ProcLog log("writerPublished", Log());
    LOGDEBUG << serviceName << std::endl;
    statusBar()->showMessage(QString("Server started on %1/%2").arg(host).arg(port));
    if (serviceName != name_->text()) {
        name_->setText(serviceName);
        QMessageBox::information(this, "Name Conflict",
                                 QString("The name requested for the service is already in use by "
                                         "another service. The new name is '%1'")
                                     .arg(serviceName),
                                 QMessageBox::Ok);
    }
}
