#ifndef SETTINGS_H
#define SETTINGS_H

#include <QWidget>

namespace Ui {
class Settings;
}

class Settings : public QWidget
{
    Q_OBJECT

public:
    explicit Settings(QWidget *parent = nullptr);
    ~Settings();

    void setCurrentPassword(const QString &password) { m_currentPassword = password; }

signals:
    void passwordChanged(const QString &newPassword);
    void fullscreenChanged(bool enabled);
    void unkillableChanged(bool enabled);

private slots:
    void on_btn_change_password_clicked();
    void on_bb_settings_accepted();
    void on_bb_settings_rejected();

    void on_btn_explorer_clicked();

private:
    Ui::Settings *ui;
    QString m_currentPassword;
};

#endif // SETTINGS_H
