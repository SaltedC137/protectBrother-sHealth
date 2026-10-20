#include "screen_light_adjust/monitor_ctrl.h"

#include "screen_light_adjust/color_temp.h"

#include <QGuiApplication>
#include <QCoreApplication>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QScreen>
#include <QStandardPaths>

namespace {
QProcess* wayland_redshift_process = nullptr;

bool run_process(const QString& program, const QStringList& arguments, QString* error) {
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted(3000)) {
        if (error)
            *error = process.errorString();
        return false;
    }
    if (!process.waitForFinished(5000)) {
        process.kill();
        process.waitForFinished();
        if (error)
            *error = QStringLiteral("命令执行超时：%1").arg(program);
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (error) {
            const auto details = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
            *error = details.isEmpty() ? QStringLiteral("%1 返回错误码 %2").arg(program).arg(process.exitCode())
                                       : details;
        }
        return false;
    }
    return true;
}

bool start_wayland_adjustment(const QString& program,
                              int temperature_kelvin,
                              double brightness,
                              QString* error) {
    if (wayland_redshift_process) {
        if (wayland_redshift_process->state() != QProcess::NotRunning) {
            wayland_redshift_process->terminate();
            if (!wayland_redshift_process->waitForFinished(500)) {
                wayland_redshift_process->kill();
                wayland_redshift_process->waitForFinished();
            }
        }
        delete wayland_redshift_process;
        wayland_redshift_process = nullptr;
    }

    auto* application = QCoreApplication::instance();
    if (!application) {
        if (error)
            *error = QStringLiteral("Wayland 色温调整需要已启动的 Qt 应用程序");
        return false;
    }

    const auto temperature = QString::number(temperature_kelvin);
    const auto level = QString::number(brightness, 'f', 2);
    wayland_redshift_process = new QProcess(application);
    wayland_redshift_process->setProcessChannelMode(QProcess::MergedChannels);
    wayland_redshift_process->start(
        program,
        {QStringLiteral("-m"), QStringLiteral("wayland"),
         QStringLiteral("-l"), QStringLiteral("0:0"),
         QStringLiteral("-t"), temperature + QLatin1Char(':') + temperature,
         QStringLiteral("-b"), level + QLatin1Char(':') + level,
         QStringLiteral("-r")});

    if (!wayland_redshift_process->waitForStarted(3000)) {
        if (error)
            *error = wayland_redshift_process->errorString();
        delete wayland_redshift_process;
        wayland_redshift_process = nullptr;
        return false;
    }

    if (wayland_redshift_process->waitForFinished(250)) {
        if (error) {
            const auto details = QString::fromLocal8Bit(wayland_redshift_process->readAll()).trimmed();
            *error = details.isEmpty() ? QStringLiteral("Wayland 色温控制进程提前退出") : details;
        }
        delete wayland_redshift_process;
        wayland_redshift_process = nullptr;
        return false;
    }
    return true;
}
}

bool MonitorController::isWaylandSession() {
    const auto environment = QProcessEnvironment::systemEnvironment();
    const auto session_type = environment.value(QStringLiteral("XDG_SESSION_TYPE")).toLower();
    if (session_type == QStringLiteral("wayland"))
        return true;
    if (session_type == QStringLiteral("x11"))
        return false;
    if (!environment.value(QStringLiteral("WAYLAND_DISPLAY")).isEmpty())
        return true;
    return QGuiApplication::platformName().contains(QStringLiteral("wayland"), Qt::CaseInsensitive);
}

QList<MonitorInfo> MonitorController::discover() {
    if (isWaylandSession()) {
        if (QGuiApplication::screens().isEmpty())
            return {};

        MonitorInfo global;
        global.name = QStringLiteral("wayland-global");
        return {global};
    }

    const auto xrandr = QStandardPaths::findExecutable(QStringLiteral("xrandr"));
    if (xrandr.isEmpty())
        return {};

    QProcess process;
    process.start(xrandr, {QStringLiteral("--verbose"), QStringLiteral("--query")});
    if (!process.waitForStarted(3000) || !process.waitForFinished(3000))
        return {};
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return {};

    const auto lines = QString::fromLocal8Bit(process.readAllStandardOutput())
                           .split('\n', Qt::SkipEmptyParts);

    QList<MonitorInfo> monitors;
    MonitorInfo current;
    bool in_monitor = false;
    const QRegularExpression connected_re(QStringLiteral(R"(^(\S+)\s+connected(?:\s|$))"));
    const QRegularExpression gamma_re(QStringLiteral(R"(^\s*Gamma:\s+(\S+))"));
    const QRegularExpression brightness_re(QStringLiteral(R"(^\s*Brightness:\s+(\S+))"));

    for (const auto& line : lines) {
        const auto connected_match = connected_re.match(line);
        if (connected_match.hasMatch()) {
            if (in_monitor)
                monitors.append(current);

            current = MonitorInfo{};
            current.name = connected_match.captured(1);
            in_monitor = true;
            continue;
        }

        if (!in_monitor)
            continue;

        const auto gamma_match = gamma_re.match(line);
        if (gamma_match.hasMatch()) {
            const auto channels = gamma_match.captured(1).split(':');
            if (channels.size() == 3) {
                current.gamma_r = channels[0].toDouble();
                current.gamma_g = channels[1].toDouble();
                current.gamma_b = channels[2].toDouble();
            }
            continue;
        }

        const auto brightness_match = brightness_re.match(line);
        if (brightness_match.hasMatch())
            current.brightness = brightness_match.captured(1).toDouble();
    }

    if (in_monitor)
        monitors.append(current);
    return monitors;
}

bool MonitorController::apply(const QString& output,
                              double brightness,
                              int temperature_kelvin,
                              QString* error) {
    brightness = qBound(0.1, brightness, 2.0);
    const int temperature = qBound(1000, temperature_kelvin, 7500);

    if (isWaylandSession()) {
        const auto redshift = QStandardPaths::findExecutable(QStringLiteral("redshift"));
        const auto gammastep = QStandardPaths::findExecutable(QStringLiteral("gammastep"));
        if (redshift.isEmpty() && gammastep.isEmpty()) {
            if (error)
                *error = QStringLiteral("Wayland 调色温需要 redshift 或 gammastep");
            return false;
        }

        QString redshift_error;
        if (!redshift.isEmpty()
            && start_wayland_adjustment(redshift, temperature, brightness, &redshift_error))
            return true;

        if (!gammastep.isEmpty()) {
            QString gammastep_error;
            if (start_wayland_adjustment(gammastep, temperature, brightness, &gammastep_error))
                return true;
            if (error) {
                *error = redshift_error.isEmpty()
                             ? gammastep_error
                             : QStringLiteral("Redshift: %1；gammastep: %2")
                                   .arg(redshift_error, gammastep_error);
            }
            return false;
        }

        if (error)
            *error = redshift_error;
        return false;
    }

    const auto xrandr = QStandardPaths::findExecutable(QStringLiteral("xrandr"));
    if (xrandr.isEmpty()) {
        if (error)
            *error = QStringLiteral("未找到 xrandr，无法调整 X11 显示输出");
        return false;
    }

    const auto rgb = temperature == 6500
                         ? RGB{1.0, 1.0, 1.0}
                         : kelvin_to_rgb(static_cast<uint16_t>(temperature));
    const auto gamma = QStringLiteral("%1:%2:%3")
                           .arg(rgb.r, 0, 'f', 3)
                           .arg(rgb.g, 0, 'f', 3)
                           .arg(rgb.b, 0, 'f', 3);
    return run_process(xrandr,
                       {QStringLiteral("--output"), output,
                        QStringLiteral("--brightness"), QString::number(brightness, 'f', 2),
                        QStringLiteral("--gamma"), gamma},
                       error);
}
