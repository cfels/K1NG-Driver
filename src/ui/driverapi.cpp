#include "driverapi.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>

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

void DriverAPI::openUrl(const QString &url) {
    static const QStringList browsers = {
        QStringLiteral("librewolf"),
        QStringLiteral("firefox"),
        QStringLiteral("chromium"),
        QStringLiteral("google-chrome"),
        QStringLiteral("kde-open5"),
        QStringLiteral("kde-open"),
    };

    QString realUser = qEnvironmentVariable("SUDO_USER");
    if (realUser.isEmpty())
        realUser = qEnvironmentVariable("DOAS_USER");

    for (const QString &cmd : browsers) {
        QString bin = QStandardPaths::findExecutable(cmd);
        if (bin.isEmpty())
            continue;

        if (!realUser.isEmpty()) {
            QString display    = qEnvironmentVariable("DISPLAY");
            QString xauth      = qEnvironmentVariable("XAUTHORITY");
            QString wayland    = qEnvironmentVariable("WAYLAND_DISPLAY");
            QString script     = QStringLiteral(
                "DISPLAY=%1 XAUTHORITY=%2 WAYLAND_DISPLAY=%3 "
                "XDG_RUNTIME_DIR=/run/user/$(id -u %4) %5 %6")
                .arg(display, xauth, wayland, realUser, bin, url);
            QProcess::startDetached(QStringLiteral("su"),
                { realUser, QStringLiteral("-c"), script });
        } else {
            QProcess::startDetached(bin, { url });
        }
        return;
    }
    QDesktopServices::openUrl(QUrl(url));
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
