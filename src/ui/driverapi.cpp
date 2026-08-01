#include "driverapi.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>

static QString detectEscalator() {
    if (!QStandardPaths::findExecutable(QStringLiteral("doas")).isEmpty())
        return QStringLiteral("doas");
    if (!QStandardPaths::findExecutable(QStringLiteral("sudo")).isEmpty())
        return QStringLiteral("sudo");
    return {};
}

DriverAPI::DriverAPI(QObject *parent) : QObject(parent) {
    m_escalator = detectEscalator();
    m_driverBin = QDir(QCoreApplication::applicationDirPath())
                      .absoluteFilePath(QStringLiteral("driver_bin/k1ng_driver"));
    if (m_escalator.isEmpty())
        qWarning("k1ng-ui: neither doas nor sudo found in PATH");
    else
        qInfo("k1ng-ui: using %s, driver: %s", qPrintable(m_escalator), qPrintable(m_driverBin));
}

bool DriverAPI::busy() const { return m_busy; }
QString DriverAPI::lastError() const { return m_lastError; }

void DriverAPI::setDPI(int dpi) {
    run({ QString::number(dpi) });
}

void DriverAPI::loadPreset(const QString &name) {
    run({ "-p", name });
}

void DriverAPI::run(const QStringList &args) {
    if (m_busy) return;

    if (m_escalator.isEmpty()) {
        m_lastError = QStringLiteral("neither doas nor sudo found");
        emit lastErrorChanged();
        emit error(m_lastError);
        return;
    }

    m_busy = true;
    emit busyChanged();

    auto *proc = new QProcess(this);
    proc->setProgram(m_escalator);
    proc->setArguments(QStringList() << m_driverBin << args);
    proc->setProcessChannelMode(QProcess::ForwardedChannels);

    connect(proc, &QProcess::finished, this, [this, proc](int exitCode, QProcess::ExitStatus) {
        m_busy = false;
        emit busyChanged();

        if (exitCode == 0) {
            emit success(QString());
        } else {
            m_lastError = QStringLiteral("driver exited with code %1").arg(exitCode);
            emit lastErrorChanged();
            emit error(m_lastError);
        }
        proc->deleteLater();
    });

    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError) {
        m_busy = false;
        emit busyChanged();
        m_lastError = proc->errorString();
        emit lastErrorChanged();
        emit error(m_lastError);
        proc->deleteLater();
    });

    proc->start();
}
