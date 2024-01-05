
# KTimeTracker

[KTimeTracker](https://apps.kde.org/ktimetracker/) tracks time spent on various tasks. It is useful for tracking hours to be billed to different clients or just to find out what percentage of your day is spent playing Doom or reading Slashdot.

You can install KTimeTracker on Linux via [Discover](appstream://org.kde.ktimetracker.desktop) or as a [flatpak on Flathub](https://flathub.org/apps/details/org.kde.ktimetracker). You can also install KTimeTracker (built with Qt 5) for [Windows](https://binary-factory.kde.org/job/KTimeTracker_Stable_win64/).

<a href='https://flathub.org/apps/details/org.kde.ktimetracker'><img width='190px' alt='Download on Flathub' src='https://flathub.org/assets/badges/flathub-badge-i-en.png'/></a>

If you'd like to contribute to KTimeTracker, see [CONTRIBUTING.md](CONTRIBUTING.md).

## Structure

* [src/dialogs/](src/dialogs): dialogs used for small tasks (creating tasks, editing time, history, export)
* [src/export/](src/export): the logic for exporting backup copies in multiple formats
* [src/file/](src/file): the main session file in `~/.local/share/ktimetracker/ktimetracker.ics`
* [src/import/](src/import): used to import GNOME Planner files
* [src/model/](src/model): the base for events/tasks/projects
* [src/settings/](src/settings): the Settings dialog, as well as the KConfigXT files
* [src/widgets/](src/widgets): specialized widgets (search bar, percent progress bar, context menus)

The `kf5` branch includes the latest Qt5 version. It will no longer receive updates.

The `master` branch includes the latest Qt6 version. It does not have a Windows build.

## Building

The easiest way to make changes and test KTimeTracker during development is to build it with [kdesrc-build](https://community.kde.org/Get_Involved/development/Build_software_with_kdesrc-build).

To build it as a standalone app:

```bash
cmake -B build/ -DCMAKE_INSTALL_PREFIX=$HOME/kde/usr
cmake --build build/
cmake --install build/
```

Use `-DBUILD_TESTING=ON` to enable tests.

## Credits

KTimeTracker was originaly written by Sirtaj Singh Kang and named KArm. The word "karm" means "work" in the author's native Punjabi.

KTimeTracker was inspired by Harald Tveit Alvestrand's very useful utility called titrax; we added some eyecandy and functionality like subtasks.
