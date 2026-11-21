/*
    SPDX-FileCopyrightText: 2026 Marian Kadanka <marian@kadanka.net>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QApplication>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTest>

#include <KStatusNotifierItem>

#include "base/mainwindow.h"
#include "base/timetrackerwidget.h"
#include "helpers.h"
#include "ktimetracker.h"

class MainWindowTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testCloseWithoutTrayIcon();
    void testCloseWithTrayIcon();
    void testTrayIconSettingAppliedWithoutRestart();
    void testTrayIconAsksToStayInThePanel();

private:
    static bool closeRequestAccepted(QWidget *window);
    static void setTrayIconSetting(bool enabled);
};

void MainWindowTest::initTestCase()
{
    // Send D-Bus to a socket that does not exist, before any window is built.
    // The tray icon of the second test registers itself as a status notifier
    // item, and on the session bus of a running desktop that puts an icon in
    // the panel for as long as the test runs.
    qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/ktimetracker-test-no-such-bus");

    // The test switches the tray icon setting on and off, which must not end up
    // in the config file of the user running the test.
    QStandardPaths::setTestModeEnabled(true);

    // Qt ends an application when its last window closes, which would stop the
    // test run as soon as the first window is gone.
    qApp->setQuitOnLastWindowClosed(false);
}

/**
 * Asks the window to close, the way a window manager does when the close button
 * in the title bar is clicked, and reports whether the window agreed to close.
 *
 * @param window the window to send the close event to
 * @return true if the window accepted the request, false if it refused it, which
 *         is what keeps a window on screen or, after hiding itself, alive
 */
bool MainWindowTest::closeRequestAccepted(QWidget *window)
{
    QCloseEvent event;
    QApplication::sendEvent(window, &event);
    return event.isAccepted();
}

/**
 * Writes the "Place an icon to the system tray" setting to the config file, the way the settings
 * dialog does, so that the program picks the new value up when it re-reads its settings.
 *
 * @param enabled true to ask for a tray icon, false to ask for none
 */
void MainWindowTest::setTrayIconSetting(bool enabled)
{
    KTimeTrackerSettings::setTrayIcon(enabled);
    KTimeTrackerSettings::self()->save();
}

void MainWindowTest::testCloseWithoutTrayIcon()
{
    KTimeTrackerSettings::setTrayIcon(false);

    const QUrl icsFile = createTempFile(this);
    QVERIFY(!icsFile.isEmpty());

    MainWindow window(icsFile);
    window.show();

    // With no tray icon to bring the window back, closing it ends the program:
    // the request is accepted and the task file is saved and closed on the way,
    // which turns the empty temporary file into an iCalendar file.
    QVERIFY(closeRequestAccepted(&window));
    QVERIFY(QFileInfo(icsFile.toLocalFile()).size() > 0);
}

void MainWindowTest::testCloseWithTrayIcon()
{
    KTimeTrackerSettings::setTrayIcon(true);

    const QUrl icsFile = createTempFile(this);
    QVERIFY(!icsFile.isEmpty());

    MainWindow window(icsFile);
    window.show();

    // The tray icon can bring the window back, so closing it only hides the
    // window: the request is refused and the task file stays open, leaving the
    // temporary file as empty as it was created.
    QVERIFY(!closeRequestAccepted(&window));
    QVERIFY(!window.isVisible());
    QCOMPARE(QFileInfo(icsFile.toLocalFile()).size(), 0);
}

void MainWindowTest::testTrayIconSettingAppliedWithoutRestart()
{
    setTrayIconSetting(false);

    MainWindow window(createTempFile(this));
    window.show();

    // Switching the setting on has to add the tray icon to the running program. The icon is
    // not reachable from here, so the test asks the window what it does with a close request:
    // it refuses and hides only when a tray icon can bring it back.
    setTrayIconSetting(true);
    qobject_cast<TimeTrackerWidget *>(window.centralWidget())->loadSettings();
    QVERIFY(!closeRequestAccepted(&window));
    QVERIFY(!window.isVisible());

    // The window is hidden now and only the icon can bring it back, while the settings are
    // reachable from the icon's own context menu, so switching the setting off has to make the
    // window reappear, minimised into the task bar, rather than leave nothing to click.
    setTrayIconSetting(false);
    qobject_cast<TimeTrackerWidget *>(window.centralWidget())->loadSettings();
    QVERIFY(window.isVisible());
    QVERIFY(window.isMinimized());

    // Without the icon, closing ends the program again.
    QVERIFY(closeRequestAccepted(&window));
}

void MainWindowTest::testTrayIconAsksToStayInThePanel()
{
    setTrayIconSetting(true);

    MainWindow window(createTempFile(this));
    window.show();

    // A status notifier item is Passive unless it says otherwise, and a tray is free to keep
    // Passive items out of the panel: Plasma shows them only in its pop-up of hidden icons,
    // while this icon is what brings the window back once it has been closed to the tray.
    auto *trayIcon = window.findChild<KStatusNotifierItem *>();
    QVERIFY(trayIcon);
    QCOMPARE(trayIcon->status(), KStatusNotifierItem::Active);
}

QTEST_MAIN(MainWindowTest)

#include "mainwindowtest.moc"
