#pragma once

#include "IRobotGateway.h"
#include "RobotRuntimeTypes.h"

#include <QString>
#include <QTimer>

class MultiChannelRobotGateway : public IRobotGateway
{
    Q_OBJECT

public:
    explicit MultiChannelRobotGateway(QObject* parent = nullptr);

    bool start() override;
    void stop() override;

    bool isConnected(int robotId) const override;

    void setManualMode(int robotId) override;
    void setAutoMode(int robotId) override;
    void clearError(int robotId) override;

    void startJointJog(int robotId, int joint, bool positive) override;
    void stopJointJog(int robotId) override;

    QString sourceModeName() const;
    bool setSourceModeName(const QString& modeName);

private:
    enum class GatewaySourceMode {
        Dummy,
        Remote
    };

private slots:
    void pollSnapshots();

private:
    void publishDummySnapshot();
    void pollRemoteSnapshots();

    void sendRemoteCommandDryRun(const QByteArray& requestPayload,
                                 const QString& successMessage);

    QTimer m_pollTimer;
    GatewaySourceMode m_sourceMode = GatewaySourceMode::Dummy;

    quint64 m_sequence = 0;
};
