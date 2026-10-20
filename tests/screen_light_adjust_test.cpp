#include "break_reminder.h"
#include "break_timer.h"
#include "preset_manager.h"
#include "settings_manager.h"

#include <QApplication>
#include <QJsonObject>
#include <QScreen>
#include <QtTest>

class ScreenLightAdjustTest : public QObject {
    Q_OBJECT

  private slots:
    void reminderCreatesOneWindowPerScreen();
    void reminderUsesTheSameMessageOnEveryScreen();
    void timerTracksWorkAndBreakPhases();
    void settingsPersistBreakOptionsAndDefaultPreset();
    void presetsStoreDisplayAndBreakSettingsTogether();
    void presetsRejectPathTraversalNames();
};

void ScreenLightAdjustTest::reminderCreatesOneWindowPerScreen() {
    const auto available_screens = QGuiApplication::screens();
    QVERIFY(!available_screens.isEmpty());

    QList<QScreen*> targets{available_screens.first(), available_screens.first(), available_screens.first()};
    BreakReminder reminder;
    reminder.show_break_on_screens(20, QStringLiteral("休息一下"), targets);

    QCOMPARE(reminder.visible_window_count(), targets.size());
    reminder.close_break();
    QCOMPARE(reminder.visible_window_count(), 0);
}

void ScreenLightAdjustTest::reminderUsesTheSameMessageOnEveryScreen() {
    const auto available_screens = QGuiApplication::screens();
    QVERIFY(!available_screens.isEmpty());

    const QString message = QStringLiteral("请休息并眺望远方");
    QList<QScreen*> targets{available_screens.first(), available_screens.first()};
    BreakReminder reminder;
    reminder.show_break_on_screens(15, message, targets);

    QCOMPARE(reminder.visible_messages(), QStringList({message, message}));
    reminder.close_break();
}

void ScreenLightAdjustTest::timerTracksWorkAndBreakPhases() {
    BreakTimer timer(50, 10);
    QVERIFY(timer.is_working());

    timer.start_work();
    QVERIFY(timer.remaining_time() > QTime(0, 0, 0));
    timer.start_break();
    QVERIFY(!timer.is_working());
    QVERIFY(timer.remaining_time() > QTime(0, 0, 0));

    timer.stop();
    QCOMPARE(timer.remaining_time(), QTime(0, 0, 0));
}

void ScreenLightAdjustTest::settingsPersistBreakOptionsAndDefaultPreset() {
    SettingsManager settings;
    settings.set_default_preset(QStringLiteral("晚间"));
    settings.set_break_enabled(true);
    settings.set_work_minutes(50);
    settings.set_break_minutes(10);
    settings.set_break_message(QStringLiteral("抬头看看远处"));
    settings.save();

    SettingsManager restored;
    restored.load();
    QCOMPARE(restored.default_preset(), QStringLiteral("晚间"));
    QVERIFY(restored.break_enabled());
    QCOMPARE(restored.work_minutes(), 50);
    QCOMPARE(restored.break_minutes(), 10);
    QCOMPARE(restored.break_message(), QStringLiteral("抬头看看远处"));
}

void ScreenLightAdjustTest::presetsStoreDisplayAndBreakSettingsTogether() {
    PresetManager presets;
    const QJsonObject preset{
        {QStringLiteral("sync_brightness"), true},
        {QStringLiteral("monitors"), QJsonArray{QJsonObject{{QStringLiteral("name"), QStringLiteral("DP-1")},
                                                            {QStringLiteral("brightness"), 90}}}},
        {QStringLiteral("break"), QJsonObject{{QStringLiteral("enabled"), true},
                                              {QStringLiteral("work_minutes"), 50},
                                              {QStringLiteral("break_minutes"), 10},
                                              {QStringLiteral("message"), QStringLiteral("休息一下")}}}
    };

    QVERIFY(presets.save(QStringLiteral("unified"), preset));
    QCOMPARE(presets.load(QStringLiteral("unified")), preset);
    QVERIFY(presets.remove(QStringLiteral("unified")));
}

void ScreenLightAdjustTest::presetsRejectPathTraversalNames() {
    PresetManager presets;
    QVERIFY(!presets.save(QStringLiteral("../outside"), {}));
    QVERIFY(presets.load(QStringLiteral("../outside")).isEmpty());
    QVERIFY(!presets.remove(QStringLiteral("../outside")));
}

int main(int argc, char* argv[]) {
    QTemporaryDir config_home;
    if (!config_home.isValid())
        return 2;
    qputenv("XDG_CONFIG_HOME", config_home.path().toLocal8Bit());

    QApplication app(argc, argv);
    ScreenLightAdjustTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "screen_light_adjust_test.moc"
