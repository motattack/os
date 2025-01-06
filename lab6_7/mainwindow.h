#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include "kerfur.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Kerfur *kerfur, QWidget *parent = nullptr);
    ~MainWindow();

    void setUnkillable(bool val) {
        m_isUnkillable = val;
    }

private slots:
    void updateNavigationButtons(int dataSize);
    void onLoadCurrentTemperature();
    void onLoadStatistics();
    void onStatisticsReceived(QNetworkReply *reply);
    void onLoadAll();
    void onBtnPrevClicked();
    void onBtnNextClicked();

protected:
    void closeEvent(QCloseEvent *event) override {
        if (m_isUnkillable) {
            if (kerfur) {
                kerfur->setAnimation(Kerfur::KerfurState::Evil);
            }
            event->ignore();
        } else {
            QMainWindow::closeEvent(event);
        }
    }

private:
    Ui::MainWindow *ui;
    Kerfur *kerfur;
    bool kerfur_reset;
    bool isErrorShown;
    bool m_isUnkillable;
    QNetworkAccessManager *networkManager;

    int currentOffset = 0;
    const int limit = 50;
};

#endif // MAINWINDOW_H
