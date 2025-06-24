# System Monitor
GNOME System Monitor is a GNOME process viewer and system monitor with an attractive, 
easy-to-use interface, It has features, such as a tree view for process dependencies,
icons for processes, the ability to hide processes that you don't want to see,
graphical time histories of CPU/memory/swap usage,
the ability to kill/renice processes needing root access,
as well as the standard features that you might expect from a process viewer.

## License
This project is licensed under the **GNU General Public License v2.0**. [Learn more](https://choosealicense.com/licenses/gpl-2.0/)

## Building
The steps described below show how to compile and install _GNOME System Monitor_ from its source.

### Install required dependencies
To build the application, the following dependencies are required:

#### Apt (Debian/Ubuntu/Derivatives - Debian-Based Package Management)
Use the following command to install dependencies:
`sudo apt install meson gettext appstream-util catch2 itstool libglibmm-2.68-dev libgtkmm-4.0-dev libgtop2-dev librsvg2-dev libadwaita-1-dev libsystemd-dev uncrustify`

#### DNF (Fedora/Centos/Derivatives - RPM-Based Package Management)
Use the following command to install dependencies:
`sudo dnf install meson gettext appstream itstool glibmm2.68-devel gtkmm4.0-devel libgtop2-devel librsvg2-devel libadwaita-devel systemd-devel catch catch-devel uncrustify`

#### Optional dependencies:
- polkit - recommended
- gksu2
- libgnomesu
- libselinux
- lsb_release in PATH - recommended on linux
- libwnck


### Building and installing
Before following the steps below, clone the repository and change to its working directory.

##### Configure and create the build directory with Meson.
`meson setup build`

Where `build` is just a directory name, and is up to your choosing.
##### Build the application - this compiles the source.
`ninja -C build`
 
##### Install the application on your system - required to run _GNOME System Monitor_.
`ninja -C build install`

### Cleanup

##### Use the following command to clean up the build directory and remove old build files.
`ninja -C build -t clean`

##### Remove the build directory to rebuild from scratch.
`rm -rf build`

## Bugs

Please file System-Monitor bugs at:
https://gitlab.gnome.org/GNOME/gnome-system-monitor/issues
