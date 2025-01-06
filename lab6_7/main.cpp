#include "mainwindow.h"
#include "settings.h"
#include "authform.h"

#include <QApplication>
#include <QScreen>
#include <csignal>
#include <memory>
#include <QShortcut>
#include <QKeySequence>
#include <QSettings>
#include <QMessageBox>

std::shared_ptr<Kerfur> globalKerfur = nullptr;

void signalHandler(int signal) {
    Q_UNUSED(signal);
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("WoaW");
    QCoreApplication::setApplicationName("WeatherApp");

    auto k = std::make_shared<Kerfur>();
    globalKerfur = k;
    k->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

    Settings set;
    AuthForm auth;
    set.setWindowFlag(Qt::WindowStaysOnTopHint);

    QSettings settings;
    QString storedPassword = settings.value("appPassword", "2025").toString();

    QObject::connect(&set, &Settings::passwordChanged, [&](const QString &newPass){
        storedPassword = newPass;
    });

    QObject::connect(&auth, &AuthForm::passwordEntered, [&](const QString &enteredPass){
        if (enteredPass == storedPassword) {
            auth.hide();
            set.show();
            set.raise();
            set.activateWindow();
        } else {
            QMessageBox::warning(&auth, "Ошибка", "Неверный пароль!");
        }
    });

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = 0;
    int y = screenGeometry.height() - k->height();
    k->move(x, y);

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    MainWindow w(k.get());

    QSettings stg;
    bool unkillable = stg.value("unkillable", false).toBool();
    bool fullscreen = stg.value("fullscreen", false).toBool();
    w.setUnkillable(unkillable);
    k->setUnkillable(unkillable);

    QObject::connect(&set, &Settings::fullscreenChanged, [&](bool enabled){
        if (enabled) {
            w.showFullScreen();
        } else {
            w.showNormal();
        }
    });

    QObject::connect(&set, &Settings::unkillableChanged, [&](bool enabled){
        w.setUnkillable(enabled);
        k->setUnkillable(enabled);
    });

    QShortcut *shortcutSettings = new QShortcut(QKeySequence("Ctrl+Alt+4"), k.get());
    QObject::connect(shortcutSettings, &QShortcut::activated, [&](){
        auth.show();
        auth.raise();
        auth.activateWindow();
    });

    w.show();
    k->show();

    return a.exec();
}
