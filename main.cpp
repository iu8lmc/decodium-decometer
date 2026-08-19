#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "DecodeFeed.hpp"
#include "MeterBridge.hpp"
#include "SpotFeed.hpp"

int main(int argc, char* argv[])
{
    // Windows, prova desktop: senza dichiarare la consapevolezza del DPI il
    // sistema virtualizza la finestra — a scala 175% i 480x900 punti chiesti
    // finiscono in 274x514 pixel veri, e il disegno esce dai bordi. Sembra un
    // difetto del layout e non lo e': le misure interne sono giuste, e' la
    // superficie a essere di un'altra dimensione. Su Android e iOS non si
    // presenta, perche' li' il DPI lo governa il sistema.
    //
    // Si rispetta comunque una scelta fatta da fuori: chi avvia con la
    // variabile gia' impostata sa quel che vuole.
#if defined(Q_OS_WIN)
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "windows:dpiawareness=2");
#endif

    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Decodium"));
    app.setApplicationName(QStringLiteral("Decometer"));
    // Da qui la legge il QML come Qt.application.version: una versione scritta
    // due volte e' una versione che prima o poi ne dice una sbagliata.
    app.setApplicationVersion(QStringLiteral(DECOMETER_VERSION));

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
