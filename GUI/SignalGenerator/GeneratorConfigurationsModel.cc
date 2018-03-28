#include "GeneratorConfigurationsModel.h"
#include "GeneratorConfiguration.h"

using namespace SideCar::GUI::SignalGenerator;

GeneratorConfigurationsModel::GeneratorConfigurationsModel(QObject* parent) : Super(parent), signalConfigurations_()
{
    ;
}

QModelIndex
GeneratorConfigurationsModel::add(GeneratorConfiguration* signalConfiguration)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    signalConfigurations_.append(signalConfiguration);
    endInsertRows();
    return index(rowCount() - 1);
}

void
GeneratorConfigurationsModel::remove(GeneratorConfiguration* signalConfiguration)
{
    int row = signalConfigurations_.indexOf(signalConfiguration);
    if (row != -1) {
        beginRemoveRows(QModelIndex(), row, row);
        delete signalConfigurations_.takeAt(row);
        endRemoveRows();
    }
}

QModelIndex
GeneratorConfigurationsModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || row >= signalConfigurations_.count()) return QModelIndex();
    return createIndex(row, column, signalConfigurations_[row]);
}

QVariant
GeneratorConfigurationsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return QVariant();
    GeneratorConfiguration* obj = signalConfigurations_[index.row()];
    switch (role) {
    case Qt::SizeHintRole: return obj->minimumSizeHint();
    case Qt::DisplayRole: return QString::number(obj->getSignalFrequency());
    default: return QVariant();
    };
}
