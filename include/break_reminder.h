#pragma once

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <qtmetamacros.h>

class QScreen;
class QWidget;

class BreakReminder : public QObject {
	Q_OBJECT
  public:
	explicit BreakReminder(QObject* parent = nullptr);
	~BreakReminder() override;

	void show_break(int break_minutes);
	void show_break_with_message(int break_minutes, const QString& message);
	void show_break_on_screens(int break_minutes, const QString& message, const QList<QScreen*>& screens);
	void close_break();

	int			visible_window_count() const;
	QStringList visible_messages() const;

  signals:
	void break_skip_requested();

  private:
	void addWindow(QScreen* screen);

	QList<QPointer<QWidget>> m_windows;
	QString					 m_message;
	QDateTime				 m_deadline;
	bool					 m_active = false;
};
