#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTime>
#include <QTimer>

class Timer : public QObject {
    Q_OBJECT
  public:
    explicit Timer(int work_minutes, int break_minutes, QObject* parent = nullptr);

    void setDurations(int work_minutes, int break_minutes);
    void startWork();
    void startInterval();
    void stop();

    QTime getRemainingTime() const;
    bool isWorking() const { return m_is_working; }

    QTimer updateTimer;

  signals:
    void worktime_timeout(quint16 break_minutes);
    void interval_timeout();
    void remainingTimeChanged(const QTime& remaining, bool is_working);

  private:
    QTimer m_work_timer;
    QTimer m_break_timer;
    QElapsedTimer m_elapsed_timer;
    int m_work_minutes;
    int m_break_minutes;
    bool m_is_working = true;
};
