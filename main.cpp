#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "DecodeFeed.hpp"
#include "MeterBridge.hpp"
#include "SpotFeed.hpp"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Decodium"));
    app.setApplicationName(QStringLiteral("Decometer"));

    // Tre sorgenti indipendenti, tre oggetti che non si conoscono fra loro:
    // il quadrante non deve fermarsi perche' il cluster tace, e viceversa.
    MeterBridge bridge;
    DecodeFeed decodeFeed;
    SpotFeed spotFeed;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("bridge"), &bridge);
    engine.rootContext()->setContextProperty(QStringLiteral("decodeFeed"), &decodeFeed);
    engine.rootContext()->setContextProperty(QStringLiteral("spotFeed"), &spotFeed);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("Decometer"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
