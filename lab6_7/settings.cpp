#include "settings.h"
#include "ui_settings.h"

#include <QMessageBox>
#include <QSettings>
#include <QProcess>

Settings::Settings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Settings)
{
    ui->setupUi(this);

    ui->password->setEchoMode(QLineEdit::Password);

    QSettings stg;
    bool unkillable = stg.value("unkillable", false).toBool();
    bool fullscreen = stg.value("fullscreen", false).toBool();

    ui->cb_unkillable->setChecked(unkillable);
    ui->cb_fullscreen->setChecked(fullscreen);

    connect(ui->cb_fullscreen, &QCheckBox::toggled, this, [this](bool checked){
        QSettings stg;
        stg.setValue("fullscreen", checked);
        stg.sync();

        emit fullscreenChanged(checked);
    });

    connect(ui->cb_unkillable, &QCheckBox::toggled, this, [this](bool checked){
        QSettings stg;
        stg.setValue("unkillable", checked);
        stg.sync();

        emit unkillableChanged(checked);
    });
}

Settings::~Settings()
{
    delete ui;
}

void Settings::on_btn_change_password_clicked()
{
    QString newPass = ui->password->text().trimmed();
    if(newPass.isEmpty()){
        QMessageBox::warning(this, tr("Ошибка"), tr("Пароль не может быть пустым"));
        return;
    }

    QSettings s;
    s.setValue("appPassword", newPass);
    s.sync();

    emit passwordChanged(newPass);

    QMessageBox::information(this, tr("Успех"), tr("Пароль успешно изменён"));
    ui->password->clear();
}

void Settings::on_bb_settings_accepted()
{
    QSettings s;
    s.setValue("fullscreen", ui->cb_fullscreen->isChecked());
    s.setValue("unkillable", ui->cb_unkillable->isChecked());
    s.sync();

    this->close();
}

void Settings::on_bb_settings_rejected()
{
    this->close();
}


void Settings::on_btn_explorer_clicked()
{
    QProcess::startDetached("explorer.exe");
}

