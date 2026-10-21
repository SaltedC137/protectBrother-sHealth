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
	static bool isWaylandSession();

	// Query the current display server and return its controllable outputs.
	static QList<MonitorInfo> discover();

	// X11 changes one xrandr output; Wayland uses Redshift's compositor backend.
	static bool
	apply(const QString& output, double brightness, int temperature_kelvin, QString* error = nullptr);
};
