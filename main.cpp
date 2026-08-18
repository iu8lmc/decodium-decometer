#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "MeterBridge.hpp"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Decodium"));
    app.setApplicationName(QStringLiteral("Decometer"));

    MeterBridge bridge;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("bridge"), &bridge);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("Decometer"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
