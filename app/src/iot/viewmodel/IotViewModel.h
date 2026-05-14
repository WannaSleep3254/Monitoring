#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <memory>

class IotDatabase;
class IRobotGateway;

class IotViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList robotModels READ robotModels NOTIFY robotModelsChanged)
    Q_PROPERTY(QVariantList robotThresholds READ robotThresholds NOTIFY robotThresholdsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit IotViewModel(QObject* parent = nullptr);

    Q_INVOKABLE bool initialize();

    void setRobotGateway(IRobotGateway* gateway);

    QVariantList robotModels() const;
    QVariantList robotThresholds() const;
    QString lastError() const;

    Q_INVOKABLE bool saveThreshold(int robotIndex, const QVariantMap& thresholdData);

signals:
    void robotModelsChanged();
    void robotThresholdsChanged();
    void lastErrorChanged();

private slots:
    void onSnapshotUpdated(int robotId, QVariantMap snapshot);

private:
    QVariantMap defaultThreshold() const;
    QVariantMap normalizeThreshold(const QVariantMap& thresholdData) const;
    void initializeDefaultModels();
    void updateRobotModelFromSnapshot(int robotId, const QVariantMap& snapshot);

private:
    IRobotGateway* m_gateway = nullptr;

    QVariantList m_robotModels;
    QVariantList m_robotThresholds;
    QString m_lastError;

    std::unique_ptr<IotDatabase> m_database;
};
