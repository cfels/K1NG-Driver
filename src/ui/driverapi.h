#pragma once
#include <QObject>
#include <QString>
#include <QProcess>
#include <qqml.h>

class DriverAPI : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit DriverAPI(QObject *parent = nullptr);

    bool busy() const;
    QString lastError() const;

    Q_INVOKABLE void setDPI(int dpi);
    Q_INVOKABLE void loadPreset(const QString &name);
    Q_INVOKABLE void openUrl(const QString &url);

signals:
    void busyChanged();
    void lastErrorChanged();
    void success(const QString &msg);
    void error(const QString &msg);

private:
    void run(const QStringList &args);

    bool m_busy = false;
    QString m_lastError;
    QString m_escalator;
    QString m_driverBin;
};
