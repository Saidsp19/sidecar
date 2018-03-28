#ifndef SIDECAR_GUI_SIGNALGENERATOR_GENERATORCONFIGURATIONSMODEL_H // -*- C++ -*-
#define SIDECAR_GUI_SIGNALGENERATOR_GENERATORCONFIGURATIONSMODEL_H

#include "QtCore/QAbstractListModel"
#include "QtCore/QList"

namespace SideCar {
namespace GUI {
namespace SignalGenerator {

class GeneratorConfiguration;

class GeneratorConfigurationsModel : public QAbstractListModel {
    Q_OBJECT
    using Super = QAbstractListModel;

public:
    GeneratorConfigurationsModel(QObject* parent = 0);

    static GeneratorConfiguration* GetObject(const QModelIndex& index)
    {
        return static_cast<GeneratorConfiguration*>(index.internalPointer());
    }

    GeneratorConfiguration* getConfiguration(int index) const { return signalConfigurations_[index]; }

    QModelIndex add(GeneratorConfiguration* signalConfiguration);

    void remove(GeneratorConfiguration* signalConfiguration);

    int getRowFor(GeneratorConfiguration* signalConfiguration) const
    {
        return signalConfigurations_.indexOf(signalConfiguration);
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const { return signalConfigurations_.count(); }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const;

    size_t getLeastMultipleGateCount(size_t init);

private:
    QList<GeneratorConfiguration*> signalConfigurations_;
};

} // end namespace SignalGenerator
} // end namespace GUI
} // end namespace SideCar

#endif
