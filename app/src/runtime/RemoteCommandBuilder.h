#pragma once

#include <QByteArray>
#include <QString>
#include <QVariantMap>

class RemoteCommandBuilder
{
public:
    static QByteArray buildSetManualMode(int robotId, const QString& operatorName = QString());
    static QByteArray buildSetAutoMode(int robotId, const QString& operatorName = QString());
    static QByteArray buildClearError(int robotId, const QString& operatorName = QString());

    static QByteArray buildHome(int robotId, const QString& operatorName = QString());
    static QByteArray buildStart(int robotId, const QString& operatorName = QString());
    static QByteArray buildStop(int robotId, const QString& operatorName = QString());

    static QByteArray buildStartJointJog(int robotId,
                                         int joint,
                                         bool positive,
                                         double speed,
                                         const QString& operatorName = QString());

    static QByteArray buildStopJointJog(int robotId,
                                        const QString& operatorName = QString());

    static QByteArray buildJogHeartbeat(int robotId,
                                        const QString& jogSessionId,
                                        const QString& operatorName = QString());

private:
    static QByteArray buildCommand(int robotId,
                                   const QString& command,
                                   const QVariantMap& params,
                                   const QString& operatorName,
                                   int timeoutMs);

    static QString makeCommandId();
};
