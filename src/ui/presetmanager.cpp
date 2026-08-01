#include "presetmanager.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QVariantMap>
#include <QProcess>
#include <QStandardPaths>

PresetManager::PresetManager(QObject *parent) : QObject(parent) {
    parse();
}

QString PresetManager::tomlPath() {
    return QStringLiteral("/root/mouse_presets.toml");
}

static QString findEscalator() {
    if (!QStandardPaths::findExecutable(QStringLiteral("doas")).isEmpty())
        return QStringLiteral("doas");
    if (!QStandardPaths::findExecutable(QStringLiteral("sudo")).isEmpty())
        return QStringLiteral("sudo");
    return {};
}

QVariantList PresetManager::presets() const {
    return m_presets;
}

void PresetManager::reload() {
    parse();
}

void PresetManager::parse() {
    m_presets.clear();

    const QString esc = findEscalator();
    QByteArray content;

    if (!esc.isEmpty()) {
        QProcess proc;
        proc.start(esc, { QStringLiteral("cat"), tomlPath() });
        proc.waitForFinished(3000);
        content = proc.readAllStandardOutput();
    } else {
        QFile f(tomlPath());
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            emit presetsChanged();
            return;
        }
        content = f.readAll();
    }

    if (content.isEmpty()) {
        emit presetsChanged();
        return;
    }

    QTextStream in(content);
    QString currentSection;
    QRegularExpression sectionRe(QStringLiteral("^\\[(.+)\\]$"));
    QRegularExpression dpiRe(QStringLiteral("^dpi\\s*=\\s*(\\d+)$"));

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        auto secMatch = sectionRe.match(line);
        if (secMatch.hasMatch()) {
            currentSection = secMatch.captured(1);
            continue;
        }
        auto dpiMatch = dpiRe.match(line);
        if (dpiMatch.hasMatch() && !currentSection.isEmpty()) {
            QVariantMap entry;
            entry[QStringLiteral("name")] = currentSection;
            entry[QStringLiteral("dpi")]  = dpiMatch.captured(1).toInt();
            m_presets.append(entry);
        }
    }

    emit presetsChanged();
}

bool PresetManager::savePreset(const QString &name, int dpi) {
    if (name.trimmed().isEmpty() || dpi < 50 || dpi > 26000 || dpi % 50 != 0)
        return false;

    deletePreset(name);

    const QString esc = findEscalator();
    if (esc.isEmpty()) return false;

    const QString line = QStringLiteral("\n[%1]\ndpi = %2\n").arg(name).arg(dpi);

    QProcess proc;
    proc.setProcessChannelMode(QProcess::ForwardedChannels);
    proc.start(esc, { QStringLiteral("tee"), QStringLiteral("-a"), tomlPath() });
    proc.write(line.toLocal8Bit());
    proc.closeWriteChannel();
    proc.waitForFinished(5000);

    parse();
    return proc.exitCode() == 0;
}

bool PresetManager::deletePreset(const QString &name) {
    QFile f(tomlPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QString content = f.readAll();
    f.close();

    QRegularExpression blockRe(
        QStringLiteral("\\[%1\\]\\s*\\ndpi\\s*=\\s*\\d+\\s*\\n?").arg(
            QRegularExpression::escape(name)));
    content.remove(blockRe);

    const QString esc = findEscalator();
    if (esc.isEmpty()) return false;

    QProcess proc;
    proc.setProcessChannelMode(QProcess::ForwardedChannels);
    proc.start(esc, { QStringLiteral("tee"), tomlPath() });
    proc.write(content.toLocal8Bit());
    proc.closeWriteChannel();
    proc.waitForFinished(5000);

    parse();
    return proc.exitCode() == 0;
}

int PresetManager::dpiForPreset(const QString &name) const {
    for (const auto &v : m_presets) {
        auto m = v.toMap();
        if (m.value(QStringLiteral("name")).toString() == name)
            return m.value(QStringLiteral("dpi")).toInt();
    }
    return -1;
}
