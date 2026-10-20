#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTime>
#include <QTimer>

class BreakTimer : public QObject {
    Q_OBJECT
  public:
    explicit BreakTimer(int work_minutes, int break_minutes, QObject* parent = nullptr);

    void set_durations(int work_minutes, int break_minutes);
    void start_work();
    void start_break();
    void stop();

    QTime remaining_time() const;
    bool is_working() const { return m_is_working; }

  signals:
    void work_period_finished(int break_minutes);
    void break_period_finished();
    void remaining_time_changed(const QTime& remaining, bool is_working);

  private:
    QTimer m_work_timer;
    QTimer m_break_timer;
    QTimer m_update_timer;
    QElapsedTimer m_elapsed_timer;
    int m_work_minutes;
    int m_break_minutes;
    bool m_is_working = true;
};
