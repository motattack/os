#include "form.h"
#include "ui_form.h"

#include <QPainter>
#include <QTimer>

Form::Form(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Form)
{
    ui->setupUi(this);

    timer = new QTimer;
    connect(timer, SIGNAL(timeout()), this, SLOT(refresh()));
    timer->start(25);
}

Form::~Form()
{
    delete ui;
}

void Form::displayNumber(int number)
{
    // Отображаем переданное число
    ui->label->setText(QString("Вы ввели число: %1").arg(number));
}

void Form::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QPen pen;
    pen.setColor(Qt::red);

    painter.setPen(pen);

    //painter.drawRect(x, y, 200, 200);
    painter.drawText(x, y, "шизойд");
}

void Form::refresh()
{
    if ((x + 50) >= ui->screen->geometry().width()) {
        dir_x = -1;
    }
    if (x < ui->screen->geometry().x()) {
        dir_x = 1;

    }

    if ((y + 50)>= ui->screen->geometry().height()) {
        dir_y = -1;
    }
    if (y < ui->screen->geometry().y()){
        dir_y = 1;
    }

    x = x + dir_x;
    y = y + dir_y;
    //qDebug() << "test";

    update();
}
