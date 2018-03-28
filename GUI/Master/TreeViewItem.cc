#include "QtGui/QColor"
#include "QtGui/QFont"

#include "GUI/Utils.h"

#include "App.h"
#include "ServicesModel.h"
#include "TreeViewItem.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

QColor
TreeViewItem::GetTextColor()
{
    static const QColor color(0, 0, 0);
    return color;
}

QColor
TreeViewItem::GetRecordingColor()
{
    static const QColor color(0xFF, 0x33, 0x33);
    return color;
}

QColor
TreeViewItem::GetOKColor()
{
    static const QColor color(0x33, 0x99, 0x33);
    return color;
}

QColor
TreeViewItem::GetWarningColor()
{
    static const QColor color(0xCC, 0x99, 0x33);
    return color;
}

QColor
TreeViewItem::GetFailureColor()
{
    static const QColor color(0xFF, 0x33, 0x33);
    return color;
}

QFont
TreeViewItem::GetTextFont()
{
    static const QFont font("Arial", 10);
    return font;
}

TreeViewItem::TreeViewItem() :
    status_(), parent_(0), index_(-1), children_(), name_("~ROOT~"), fullName_(""), ok_(true), processing_(false),
    activeState_(kIdle), expanded_(true)
{
    ;
}

TreeViewItem::TreeViewItem(const IO::StatusBase& status, TreeViewItem* parent) :
    status_(status), parent_(parent), index_(-1), children_(), name_(QString::fromStdString(status_.getName())),
    fullName_(QString("%1:%2").arg(parent->getFullName()).arg(name_)), ok_(true), processing_(false),
    activeState_(kIdle), expanded_(true)
{
    ;
}

TreeViewItem::~TreeViewItem()
{
    qDeleteAll(children_);
}

bool
TreeViewItem::update(const IO::StatusBase& status)
{
    beforeUpdate();
    bool changed = status_ != status;
    status_ = status;
    afterUpdate();
    return changed;
}

QVariant
TreeViewItem::getData(int column, int role) const
{
    QVariant value;
    switch (column) {
    case ServicesModel::kName: value = getNameDataValue(role); break;
    case ServicesModel::kHost: value = getHostDataValue(role); break;
    case ServicesModel::kState: value = getStateDataValue(role); break;
    case ServicesModel::kRecording: value = getRecordingDataValue(role); break;
    case ServicesModel::kPending: value = getPendingCountValue(role); break;
    case ServicesModel::kRate: value = getRateDataValue(role); break;
    case ServicesModel::kError: value = getErrorDataValue(role); break;
    case ServicesModel::kInfo: value = getInfoDataValue(role); break;
    default: break;
    }

    // If we don't have a value yet, try again for some specific roles.
    //
    if (!value.isValid()) {
        switch (role) {
        case Qt::TextAlignmentRole:
            value = getAlignment(column);
            break;
            // case Qt::FontRole: value = getFont(column); break;
        case Qt::ForegroundRole: value = getForegroundColor(column); break;
        default: break;
        }
    }

    return value;
}

QVariant
TreeViewItem::getNameDataValue(int role) const
{
    if (role != Qt::DisplayRole) return QVariant();
    return name_;
}

QVariant
TreeViewItem::getStateDataValue(int role) const
{
    return QVariant();
}

QVariant
TreeViewItem::getRecordingDataValue(int role) const
{
    return QVariant();
}

QVariant
TreeViewItem::getPendingCountValue(int role) const
{
    return QVariant();
}

QVariant
TreeViewItem::getRateDataValue(int role) const
{
    return QVariant();
}

QVariant
TreeViewItem::getErrorDataValue(int role) const
{
    return QVariant();
}

QVariant
TreeViewItem::getInfoDataValue(int role) const
{
    return QVariant();
}

QColor
TreeViewItem::getForegroundColor(int column) const
{
    switch (column) {
    case ServicesModel::kName:
    case ServicesModel::kError:
        if (!ok_) return GetFailureColor();
        break;

    case ServicesModel::kState: return ok_ && processing_ ? GetOKColor() : GetFailureColor();

    case ServicesModel::kRate:
        if (ok_ && processing_) {
            if (activeState_ == kActive) return GetOKColor();
            return GetWarningColor();
        }
        return GetFailureColor();
        break;

    default: break;
    }

    return GetTextColor();
}

QFont
TreeViewItem::getFont(int column) const
{
    if (column == ServicesModel::kInfo) {
        QFont font = GetTextFont();
        font.setPointSize(font.pointSize() - 1);
        return font;
    }

    return GetTextFont();
}

int
TreeViewItem::getAlignment(int column) const
{
    int flags = Qt::AlignVCenter;
    switch (column) {
    case ServicesModel::kState:
    case ServicesModel::kRecording: flags |= Qt::AlignHCenter; break;

    case ServicesModel::kPending:
    case ServicesModel::kRate: flags |= Qt::AlignRight; break;

    case ServicesModel::kHost:
    case ServicesModel::kInfo:
    default: flags |= Qt::AlignLeft;
    }

    return flags;
}

void
TreeViewItem::insertChild(int index, TreeViewItem* child)
{
    child->initialize();
    children_.insert(index, child);
    for (; index < children_.size(); ++index) children_[index]->index_ = index;
    childAdded(child);
}

void
TreeViewItem::appendChild(TreeViewItem* child)
{
    child->initialize();
    child->index_ = children_.size();
    children_.append(child);
    childAdded(child);
}

void
TreeViewItem::removeChild(int index)
{
    TreeViewItem* child = children_.takeAt(index);
    for (; index < children_.size(); ++index) children_[index]->index_ = index;
    childRemoved(child);
    delete child;
}

QVariant
TreeViewItem::GetRecordingDataValue(bool canRecord, int role)
{
    bool isRecording = App::GetApp()->isRecording();

    switch (role) {
    case Qt::DisplayRole:
        if (canRecord) return isRecording ? "REC" : "Y";
        return " ";
        break;

    case Qt::ForegroundRole: return GetRecordingColor(); break;

    default: break;
    }

    return QVariant();
}

void
TreeViewItem::setProcessingState(IO::ProcessingState::Value state)
{
    switch (state) {
    case IO::ProcessingState::kAutoDiagnostic:
    case IO::ProcessingState::kCalibrate:
    case IO::ProcessingState::kRun: processing_ = true; break;

    case IO::ProcessingState::kFailure:
        ok_ = false;
        // !!! Falling thru by design !!!
        //

    default: processing_ = false; break;
    }
}

void
TreeViewItem::setName(const QString& name)
{
    name_ = name;
    fullName_ = QString("%1:%2").arg(getParent()->getFullName()).arg(name);
}

int
TreeViewItem::getInsertionPoint(const QString& name) const
{
    QString lowerName = name.toLower();
    int index = 0;
    for (; index < getNumChildren(); ++index) {
        int rc = QString::compare(lowerName, getChild(index)->getName().toLower());
        if (rc < 0) break;
    }

    return index;
}
