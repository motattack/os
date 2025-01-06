#ifndef AUTHFORM_H
#define AUTHFORM_H

#include <QWidget>

namespace Ui {
class AuthForm;
}

class AuthForm : public QWidget
{
    Q_OBJECT

public:
    explicit AuthForm(QWidget *parent = nullptr);
    ~AuthForm();

signals:
    void passwordEntered(const QString &password);

private slots:
    void on_btn_login_clicked();

private:
    Ui::AuthForm *ui;
};

#endif // AUTHFORM_H
