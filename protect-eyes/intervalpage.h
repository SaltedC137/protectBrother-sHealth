#pragma once

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QStringList>

class QScreen;
class QWidget;

class IntervalPage : public QObject {
    Q_OBJECT
  public:
    explicit IntervalPage(QObject* parent = nullptr);
    ~IntervalPage() override;

    void showIntervalPage(quint16 break_minutes);
    void showIntervalPageWithMessage(int break_minutes, const QString& message);
    void showIntervalPageOnScreens(int break_minutes,
                                   const QString& message,
                                   const QList<QScreen*>& screens);
    void closeIntervalPage();

    int visibleWindowCount() const;
    QStringList visibleMessages() const;

  private:
    void addWindow(QScreen* screen);

    QList<QPointer<QWidget>> m_windows;
    QString m_message;
    QDateTime m_deadline;
    bool m_active = false;
};
