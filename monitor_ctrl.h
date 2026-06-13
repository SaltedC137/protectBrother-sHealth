#pragma once
#include <QList>
#include <QString>

struct MonitorInfo {
	QString name;
	double	brightness = 1.0;  // 0.1 - 1.0
	double	gamma_r	   = 1.0;
	double	gamma_g	   = 1.0;
	double	gamma_b	   = 1.0;
};

class MonitorController {
  public:
	// Query xrandr and return currently connected monitors
	static QList<MonitorInfo> discover();

	// Apply brightness + gamma to a single output
	static bool
	apply(const QString& output, double brightness, double gamma_r, double gamma_g, double gamma_b);
};
