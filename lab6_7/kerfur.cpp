#include "kerfur.h"
#include "ui_kerfur.h"

#include <QTimer>
#include <QtMath>
#include <QDebug>

Kerfur::Kerfur(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Kerfur)
    , timer_cycle(new QTimer(this))
    , meowPlayer(new QMediaPlayer(this))
    , audioOutput(new QAudioOutput(this))
    , currentState(KerfurState::Neutral)
    , currentFrame(0)
    , isDragging(false)
    , isMurmuring(false)
    , m_isUnkillable(false)
{
    ui->setupUi(this);

    loadAnimations();

    connect(timer_cycle, &QTimer::timeout, this, &Kerfur::cycle);
    timer_cycle->start(400);

    meowPlayer->setAudioOutput(audioOutput);
    meowPlayer->setSource(QUrl("qrc:/kerfur/audio/kerfur2meow-02.ogg"));
    audioOutput->setVolume(1);
}

Kerfur::~Kerfur()
{
    delete ui;
}

void Kerfur::loadAnimations()
{
    animations[KerfurState::Neutral] = {
        QImage(":/kerfur/images/kerfur_neutral_center.svg")
    };

    animations[KerfurState::NeutralBlink] = {
        QImage(":/kerfur/images/kerfur_blink_neutral.svg")
    };

    animations[KerfurState::Murmur] = {
        QImage(":/kerfur/images/kerfur_murmur_eyes.svg"),
        QImage(":/kerfur/images/kerfur_murmur_tongue.svg")
    };

    animations[KerfurState::Evil] = {
        QImage(":/kerfur/images/kerfur_evil_mad.svg")
    };

    animations[KerfurState::EvilMurmur] = {
        QImage(":/kerfur/images/kerfur_evil_sly.svg")
    };

    animations[KerfurState::SpiralEyes] = {
        QImage(":/kerfur/images/kerfur_spiral_eyes.svg")
    };
}

void Kerfur::setAnimation(KerfurState state)
{
    if (state == currentState)
        return;

    currentState = state;
    currentFrame = 0;
    update();
}

void Kerfur::cycle()
{
    if (currentState == KerfurState::Neutral) {
        int rnd = QRandomGenerator::global()->bounded(4);
        if (rnd == 0) {
            setAnimation(KerfurState::NeutralBlink);
            QTimer::singleShot(200, this, [this]() {
                setAnimation(KerfurState::Neutral);
            });
        }
    }

    prevFrame = currentFrame;
    if (isMurmuring && currentState != KerfurState::Murmur && currentState != KerfurState::SpiralEyes) {
        if (currentState == KerfurState::Evil || currentState == KerfurState::EvilMurmur) {
            setAnimation(KerfurState::EvilMurmur);
        } else {
            int murmurIndex = QRandomGenerator::global()->bounded(animations[KerfurState::Murmur].size());
            setAnimation(KerfurState::Murmur);
            currentFrame = murmurIndex;
        }
    }

    if (!isMurmuring && !animations[currentState].isEmpty()) {
        currentFrame = (currentFrame + 1) % animations[currentState].size();
    }

    if (prevFrame != currentFrame) {
        update();
    }
}

void Kerfur::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (!animations[currentState].isEmpty()) {
        const QImage &currentImage = animations[currentState][currentFrame];
        painter.drawImage(
            QRectF(0, 0, width(), height()),
            currentImage,
            currentImage.rect(),
            Qt::AutoColor
        );
    }
}

void Kerfur::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();

        isMurmuring = true;
        meowPlayer->stop();
        meowPlayer->play();
        event->accept();
    }
}

void Kerfur::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging && (event->buttons() & Qt::LeftButton)) {
        moveWithinScreen(event->globalPosition().toPoint() - dragPosition);
        event->accept();
    }
}

void Kerfur::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
        isMurmuring = false;

        if (currentState == KerfurState::EvilMurmur) {
            setAnimation(KerfurState::Evil);
        } else if (currentState == KerfurState::Murmur) {
            setAnimation(KerfurState::Neutral);
        }
        event->accept();
    }
}

void Kerfur::moveWithinScreen(const QPoint &newPos)
{
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();

    int x = std::clamp(newPos.x(), screenGeometry.left(),  screenGeometry.right()  - width());
    int y = std::clamp(newPos.y(), screenGeometry.top(),   screenGeometry.bottom() - height());

    move(QPoint(x, y));
}
