#include "timer.h"

#include <algorithm>

namespace {
constexpr int kMinimumMinutes = 1;
constexpr int kMaximumMinutes = 150;
}

Timer::Timer(int work_minutes, int break_minutes, QObject* parent)
    : QObject(parent), m_work_minutes(work_minutes), m_break_minutes(break_minutes) {
    m_work_timer.setSingleShot(true);
    m_break_timer.setSingleShot(true);
    updateTimer.setInterval(1000);

    connect(&m_work_timer, &QTimer::timeout, this, [this] {
        emit worktime_timeout(m_break_minutes);
    });
    connect(&m_break_timer, &QTimer::timeout, this, &Timer::interval_timeout);
    connect(&updateTimer, &QTimer::timeout, this, [this] {
        emit remainingTimeChanged(getRemainingTime(), m_is_working);
    });

    setDurations(work_minutes, break_minutes);
}

void Timer::setDurations(int work_minutes, int break_minutes) {
    m_work_minutes = std::clamp(work_minutes, kMinimumMinutes, kMaximumMinutes);
    m_break_minutes = std::clamp(break_minutes, kMinimumMinutes, kMaximumMinutes);
}

void Timer::startWork() {
    m_break_timer.stop();
    m_is_working = true;
    m_elapsed_timer.start();
    m_work_timer.start(m_work_minutes * 60 * 1000);
    updateTimer.start();
    emit remainingTimeChanged(getRemainingTime(), m_is_working);
}

void Timer::startInterval() {
    m_work_timer.stop();
    m_is_working = false;
    m_elapsed_timer.start();
    m_break_timer.start(m_break_minutes * 60 * 1000);
    updateTimer.start();
    emit remainingTimeChanged(getRemainingTime(), m_is_working);
}

void Timer::stop() {
    m_work_timer.stop();
    m_break_timer.stop();
    updateTimer.stop();
}

QTime Timer::getRemainingTime() const {
    const bool timer_active = m_is_working ? m_work_timer.isActive() : m_break_timer.isActive();
    if (!timer_active || !m_elapsed_timer.isValid())
        return QTime(0, 0, 0);

    const int phase_minutes = m_is_working ? m_work_minutes : m_break_minutes;
    const qint64 remaining_seconds = std::max<qint64>(
        0, (static_cast<qint64>(phase_minutes) * 60 * 1000 - m_elapsed_timer.elapsed()) / 1000);
    return QTime(static_cast<int>(remaining_seconds / 3600),
                 static_cast<int>((remaining_seconds % 3600) / 60),
                 static_cast<int>(remaining_seconds % 60));
}
