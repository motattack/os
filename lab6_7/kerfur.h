#ifndef KERFUR_H
#define KERFUR_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QMouseEvent>
#include <QMap>
#include <QImage>
#include <QGuiApplication>
#include <QScreen>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QRandomGenerator>

namespace Ui {
class Kerfur;
}

class Kerfur : public QWidget
{
    Q_OBJECT

public:
    explicit Kerfur(QWidget *parent = nullptr);
    ~Kerfur();

    enum class KerfurState {
        Neutral,
        NeutralBlink,
        Murmur,
        Evil,
        EvilMurmur,
        SpiralEyes
    };

    void setAnimation(KerfurState state);

    void setUnkillable(bool val) {
        m_isUnkillable = val;
    }

private:
    Ui::Kerfur *ui;
    QTimer *timer_cycle;
    QMediaPlayer *meowPlayer;
    QAudioOutput *audioOutput;

    QMap<KerfurState, QVector<QImage>> animations;
    KerfurState currentState;
    int prevFrame{0};
    int currentFrame{0};

    bool isMurmuring;
    bool isDragging;
    bool m_isUnkillable;
    QPoint dragPosition;

    void loadAnimations();
    void moveWithinScreen(const QPoint &newPos);

private slots:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void cycle();

protected:
    void closeEvent(QCloseEvent *event) override {
        if (m_isUnkillable) {
            setAnimation(Kerfur::KerfurState::Evil);
            event->ignore();
        } else {
            QWidget::closeEvent(event);
        }
    }

public slots:
    void onAnimationChanged(KerfurState state)
    {
        setAnimation(state);
    }
};

#endif // KERFUR_H
