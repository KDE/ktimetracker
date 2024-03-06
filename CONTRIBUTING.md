<!--
# SPDX-FileCopyrightText: 2024 Thiago Masato Costa Sueto <thiago.sueto@kde.org>
# SPDX-License-Identifier: CC0-1.0
-->

# Contributing to KTimeTracker

The source code for KTimeTracker can be found at https://invent.kde.org/pim/ktimetracker
and the project uses QtWidgets + KDE Frameworks.

Issues should be reported to the [KDE Bugzilla](https://bugs.kde.org/enter_bug.cgi?product=ktimetracker&component=general).

## Project Structure

The KTimeTracker source code is modular and easy to understand.

The main structure is: main loads mainwindow, which loads timetrackerwidget, which loads taskview and searchline.

The basics for understanding tasks is: taskmodelitem -> task.
A task can have a representation as an event for iCalendar management.
The projectmodel manages both tasks and events.

The rest consists of minor dialogs or actions.

---

* **base/desktoplist**: a single typedef for DesktopList used in model/task, dialogs/taskpropertiesdialog, desktoptracker, plannerparser, taskview, timetrackerstorage, timetrackerwidget and main
* **base/desktoptracker**: X11-only feature where tasks only run in certain virtual desktops
* **base/focusdetector**: X11-only detection for which window is currently active, used in taskview
* **base/idletimedetector**: uses KIdleTime to store the time spent idling, includes KGuiItem dialog
* **base/ktimetrackerutility**: generic collection of utilities to manage locale, time format, errors (used in ktimetrackerwidget)
* **base/mainwindow**: KXmlGuiWindow with a central widget TimeTrackerWidget, manages X11 window tracking, TrayIcon, and with KActionCollection shortcuts
* **base/reportcriteria**: collection of enums and values for exporting backup copies (work reports)
* **base/taskview**: main area where tasks will be listed, initialized by ktimetrackerwidget which is initialized in mainwindow which is initialized in main
* **base/timetrackerstorage**: base to save and load **local** iCalendar files
* **base/timetrackerwidget**: very large, the container for the taskview, searchline and definitions for KActionCollection actions, as well as most actions from the first screen and menu, used as parent to generate DBus calls
* **base/tray**: simple KStatusNotifierItem and its actions
* **base/treeviewheadercontextmenu**: used for context menus in taskview, including when clicking the column titles

---

* **dialogs/taskpropertiesdialog**: small dialog for creating/editing new tasks
* **dialogs/edittimedialog**: small dialog that appears only with context menu
* **dialogs/historydialog**: small dialog that appears only in the Edit Time dialog
* **dialogs/exportdialog**: small dialog to export to file or clipboard

---

* **export/**: the logic for exporting backup copies in multiple formats

---

* **file/filecalendar**: the main KTimeTracker iCalendar file format
* **file/icalformatkio**: the load/save procedure for the filecalendar (**both local and remote**)

---

* **import/plannerparser**: generates an object representing a GNOME Planner XML import file

---

* **model/event**: object for date/time events
* **model/eventsmodel**: operations to manage date/time events
* **model/taskmodelitem**: base class for tasks
* **model/task**: main object for tasks
* **model/taskmodel**: operations to manage tasks
* **model/projectmodel**: high-level manager of events and tasks

---

* **settings/ktimetrackerconfigdialog**: the main Settings dialog
* **settings/cfgbehavior**: Behavior tab in the settings
* **settings/cfgdisplay**: Appearance tab in the settings
* **settings/cfgstorage**: Storage tab in the settings

---

* **tests/**: small collection of tests for various tasks like file load/save, time formatting, task creation/deletion, needs -DBUILD_TESTING

---

* **widgets/searchline**: main search bar
* **widgets/taskswidget**: Percent Complete progress bar, Percent Complete context menu, Priority context menu

## Available tasks (TODO)

* See if Planner import is still working, if it isn't, remove it
* Remove the desktop tracking feature OR make it run on Wayland
* Remove mention to karm.ics files
* Check why openFile() and saveFile() are commented out
* Find out the practical differente between timetrackerstorage and file/icalformatkio
* Remove all cyclic dependencies
* Make it build for Windows again

## Notes

* When running clangd, you might see the warning
"Must specify at least one argument for '...' parameter of variadic macro"
when using qCDebug / qCWarning. This is fine and ignorable and in fact is
present in ECM KDECompilerSettings by default. You are probably
using an IDE like QtCreator which has its own separate settings for clangd,
refer to https://planet.kde.org/ahmad-samir-2021-10-07-gcc-clang-d-lsp-client-kate-and-variadic-macro-warnings-a-short-story/
to disable these warnings.
