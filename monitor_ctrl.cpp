#include "monitor_ctrl.h"
#include <QProcess>
#include <QRegularExpression>

QList<MonitorInfo> MonitorController::discover() {
	QProcess proc;
	proc.start("xrandr", {"--verbose", "--query"});
	if(!proc.waitForFinished(3000))
		return {};

	auto lines = QString::fromUtf8(proc.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);

	QList<MonitorInfo> monitors;
	MonitorInfo		   current;
	bool			   in_monitor = false;

	QRegularExpression conn_re(R"(^(\S+)\s+connected\s+)");
	QRegularExpression gamma_re(R"(^\s*Gamma:\s+(\S+))");
	QRegularExpression bright_re(R"(^\s*Brightness:\s+(\S+))");

	for(const auto& line : lines) {
		auto conn_match = conn_re.match(line);
		if(conn_match.hasMatch()) {
			if(in_monitor)
				monitors.append(current);

			current		 = MonitorInfo{};
			current.name = conn_match.captured(1);
			in_monitor	 = true;
			continue;
		}

		if(!in_monitor)
			continue;

		// Stop collecting on next output or disconnected line
		if(line.contains(" disconnected "))
			continue;

		auto gamma_match = gamma_re.match(line);
		if(gamma_match.hasMatch()) {
			auto parts = gamma_match.captured(1).split(':');
			if(parts.size() == 3) {
				current.gamma_r = parts[0].toDouble();
				current.gamma_g = parts[1].toDouble();
				current.gamma_b = parts[2].toDouble();
			}
			continue;
		}

		auto bright_match = bright_re.match(line);
		if(bright_match.hasMatch()) {
			current.brightness = bright_match.captured(1).toDouble();
		}
	}
	if(in_monitor)
		monitors.append(current);

	return monitors;
}

bool MonitorController::apply(const QString& output,
							  double		 brightness,
							  double		 gamma_r,
							  double		 gamma_g,
							  double		 gamma_b) {
	QString gamma_str =
		QString("%1:%2:%3").arg(gamma_r, 0, 'f', 2).arg(gamma_g, 0, 'f', 2).arg(gamma_b, 0, 'f', 2);

	QProcess proc;
	proc.start("xrandr",
			   {
				   "--output",
				   output,
				   "--brightness",
				   QString::number(brightness, 'f', 2),
				   "--gamma",
				   gamma_str,
			   });

	if(!proc.waitForFinished(3000))
		return false;

	return proc.exitCode() == 0;
}
