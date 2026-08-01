#pragma once
#include <QObject>
#include <QString>
#include <QVariantList>
#include <qqml.h>

class PresetManager : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList presets READ presets NOTIFY presetsChanged)

public:
    explicit PresetManager(QObject *parent = nullptr);

    QVariantList presets() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool savePreset(const QString &name, int dpi);
    Q_INVOKABLE bool deletePreset(const QString &name);
    Q_INVOKABLE int dpiForPreset(const QString &name) const;

signals:
    void presetsChanged();

private:
    void parse();

    static QString tomlPath();

    QVariantList m_presets;
};
