#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QImage>
#include <QPixmap>
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>

MainWindow::MainWindow(Kerfur *kerfur, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , kerfur(kerfur)
    , kerfur_reset(false)
    , isErrorShown(false)
    , m_isUnkillable(false)
{
    ui->setupUi(this);
    networkManager = new QNetworkAccessManager(this);

    ui->statisticsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->statisticsTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    updateNavigationButtons(0);
    onLoadCurrentTemperature();

    connect(ui->loadStatsButton, &QPushButton::clicked, this, &MainWindow::onLoadAll);

    connect(ui->btn_prev, &QPushButton::clicked, this, &MainWindow::onBtnPrevClicked);
    connect(ui->btn_next, &QPushButton::clicked, this, &MainWindow::onBtnNextClicked);
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onLoadCurrentTemperature()
{
    QUrl url("http://local.woaw/current-temperature");
    QNetworkRequest request(url);

    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Network error:" << reply->errorString();
            ui->currentTemperatureLabel->setText("Ошибка сети");
            kerfur->setAnimation(Kerfur::KerfurState::SpiralEyes);
            kerfur_reset = true;
            if (!isErrorShown) {
                isErrorShown = true;
                QMessageBox::critical(this, "Ошибка",
                                      "Не удалось загрузить текущую температуру: "
                                          + reply->errorString());
            }
            reply->deleteLater();
            return;
        } else {
            if (kerfur_reset) {
                kerfur->setAnimation(Kerfur::KerfurState::Neutral);
                kerfur_reset = false;
            }
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (!doc.isArray()) {
                ui->currentTemperatureLabel->setText("Некорректный формат данных");
                if (!isErrorShown) {
                    isErrorShown = true;
                    QMessageBox::critical(this, "Ошибка",
                                          "Ответ сервера содержит некорректные данные.");
                }
                reply->deleteLater();
                return;
            }

            QJsonArray data = doc.array();
            if (data.isEmpty() || !data[0].isObject()) {
                ui->currentTemperatureLabel->setText("Нет данных");
                if (!isErrorShown) {
                    isErrorShown = true;
                    QMessageBox::critical(this, "Ошибка",
                                          "Сервер вернул пустые данные.");
                }
                reply->deleteLater();
                return;
            } else {
                // Если данные корректны:
                double temperature = data[0].toObject()
                                         .value("temperature")
                                         .toString()
                                         .toDouble();
                ui->currentTemperatureLabel->setText(
                    QString::number(temperature, 'f', 1) + " °C"
                    );
                isErrorShown = false;
            }
        }
        reply->deleteLater();
    });
}


void MainWindow::onLoadStatistics()
{
    QString startDate = ui->startDateEdit->date().toString("yyyy-MM-dd");
    QString startTime = ui->startTimeEdit->time().toString("hh:mm:ss");
    QString endDate = ui->endDateEdit->date().toString("yyyy-MM-dd");
    QString endTime = ui->endTimeEdit->time().toString("hh:mm:ss");

    QString logType;
    if (ui->rb_type_all->isChecked())
        logType = "all";
    else if (ui->rb_type_hourly->isChecked())
        logType = "hourly";
    else if (ui->rb_type_daily->isChecked())
        logType = "daily";
    else
        logType = "all";

    QString requestUrl = QString("http://local.woaw/stats?logType=%1&startDate=%2+%3&endDate=%4+%5&offset=%6&limit=%7")
                             .arg(logType)
                             .arg(startDate)
                             .arg(startTime)
                             .arg(endDate)
                             .arg(endTime)
                             .arg(currentOffset)
                             .arg(limit);

    QUrl url(requestUrl);
    QNetworkRequest request(url);

    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->onStatisticsReceived(reply);
    });
}


void MainWindow::onStatisticsReceived(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Network error:" << reply->errorString();
        if (!isErrorShown) {
            isErrorShown = true;
            QMessageBox::critical(this, "Ошибка", "Не удалось загрузить статистику: " + reply->errorString());
        }
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isArray()) {
        qDebug() << "Invalid JSON format";
        if (!isErrorShown) {
            isErrorShown = true;
            QMessageBox::critical(this, "Ошибка", "Ответ сервера содержит некорректные данные.");
        }
        reply->deleteLater();
        return;
    }

    QJsonArray data = doc.array();
    ui->statisticsTable->setRowCount(0);

    for (const QJsonValue &value : data) {
        QJsonObject obj = value.toObject();
        if (!obj.contains("timestamp") || !obj.contains("temperature")) {
            qDebug() << "Missing required fields in data";
            if (!isErrorShown) {
                isErrorShown = true;
                QMessageBox::critical(this, "Ошибка", "Отсутствуют необходимые данные в ответе.");
            }
            continue;
        }

        QString timestampStr = obj.value("timestamp").toString();
        QDateTime dateTime = QDateTime::fromString(timestampStr, "yyyy-MM-dd HH:mm:ss");
        if (!dateTime.isValid()) {
            qDebug() << "Invalid timestamp format:" << timestampStr;
            if (!isErrorShown) {
                isErrorShown = true;
                QMessageBox::critical(this, "Ошибка", "Некорректный формат временной метки.");
            }
            continue;
        }

        QString formattedDate = dateTime.toString("dd.MM.yyyy HH:mm:ss");

        double temperature = obj.value("temperature").toString().toDouble();

        int row = ui->statisticsTable->rowCount();
        ui->statisticsTable->insertRow(row);
        ui->statisticsTable->setItem(row, 0, new QTableWidgetItem(formattedDate));
        ui->statisticsTable->setItem(row, 1, new QTableWidgetItem(QString::number(temperature, 'f', 1) + " °C"));
    }

    updateNavigationButtons(data.size());

    isErrorShown = false;

    reply->deleteLater();
}

void MainWindow::updateNavigationButtons(int dataSize)
{
    ui->btn_prev->setEnabled(currentOffset > 0 and dataSize != 0);

    ui->btn_next->setEnabled(dataSize == limit);
}

void MainWindow::onBtnPrevClicked()
{
    if (currentOffset >= limit) {
        currentOffset -= limit;
        onLoadStatistics();
    }
}

void MainWindow::onBtnNextClicked()
{
    currentOffset += limit;
    onLoadStatistics();
}

void MainWindow::onLoadAll()
{
    onLoadCurrentTemperature();
    onLoadStatistics();
}
