#include "authform.h"
#include "ui_authform.h"

AuthForm::AuthForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AuthForm)
{
    ui->setupUi(this);

    ui->password->setEchoMode(QLineEdit::Password);
}

AuthForm::~AuthForm()
{
    delete ui;
}

void AuthForm::on_btn_login_clicked()
{
    QString enteredPassword = ui->password->text();
    emit passwordEntered(enteredPassword);
}
