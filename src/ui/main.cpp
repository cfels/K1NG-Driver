#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QStyleHints>
#include <QUrl>
#include <QStringLiteral>
#include <QIcon>

int main(int argc, char *argv[]) {
    QGuiApplication::setDesktopSettingsAware(false);

    QGuiApplication app(argc, argv);
    app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    app.setWindowIcon(QIcon(QStringLiteral(":/qt/qml/src/ui/qml/assets/logo.png")));

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));
    engine.addImportPath(QStringLiteral("qrc:/"));

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/src/ui/qml/main_window.qml")));

    return app.exec();
}
