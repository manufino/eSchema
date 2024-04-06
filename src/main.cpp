#include "MainWindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QSplashScreen>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QPixmap pixmap(":/res/resources/splash.jpeg");

    int newWidth = 500; // Larghezza desiderata
    int newHeight = 0;  // Altezza sarà calcolata automaticamente per mantenere le proporzioni

    // Calcola la nuova altezza mantenendo le proporzioni originali
    newHeight = pixmap.height() * newWidth / pixmap.width();

    // Ridimensiona la QPixmap mantenendo le proporzioni originali
    QPixmap resizedPixmap = pixmap.scaled(newWidth, newHeight, Qt::KeepAspectRatio);

    QSplashScreen splash(resizedPixmap);
    splash.show();
    splash.showMessage("Caricamento ...");

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "eSchema_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    MainWindow w;
    w.show();
    splash.finish(&w);

    return a.exec();
}
