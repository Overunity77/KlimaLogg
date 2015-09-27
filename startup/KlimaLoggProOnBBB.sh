#!/bin/bash

### BEGIN INIT INFO

# Provides:		KlimaLoggProOnBBB
# Required-Start:	$remote_fs $syslog
# Required-Stop:	$remote_fs $syslog
# Default-Start:	2 3 4 5
# Default-Stop:		0 1 6
# Short-Description:	Simple script to start a program at boot
# Description:		A simple script which will
#			start / stop a program at boot / shutdown.
#
### END INIT INFO

# If you want a command to always run, put it here

# Carry out specific functions when asked to by the system
case "$1" in
	start)
		echo "Starting KlimaLoggProOnBBB"
		# Export library path
		export LD_LIBRARY_PATH=/lib:/usr/lib:/usr/local/lib:/usr/local/qt-5.3/lib:$LD_LIBRARY_PATH
		#
		# Setup the environment for the touch screen.
		#
		export TSLIB_CONSOLEDEVICE=none
		export TSLIB_FBDEVICE=/dev/fb0
		export TSLIB_TSDEVICE=/dev/input/event1
		export TSLIB_TSEVENTTYPE=INPUT
		export TSLIB_CALIBFILE=/usr/local/etc/pointercal
		export TSLIB_CONFFILE=/usr/local/etc/ts.conf
		export TSLIB_PLUGINDIR=/usr/local/lib/ts
		export TSTS_INFO_FILE=/sys/devices/virtual/input/input1/uevent
		# Setup Qt environment
		export QT_PLUGIN_PATH=/usr/local/qt-5.3/plugins
		export QT_QPA_FONTDIR=/usr/local/qt-5.3/lib/fonts
		export QT_QPA_PLATFORM_PLUGIN_PATH=/usr/local/qt-5.3/lib/plugins/platforms
		export QT_QPA_PLATFORM=linuxfb
		export QT_QPA_EVDEV_MOUSE_PARAMETERS=rotate=270:dejitter=10
		export QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS=rotate=90
		export QT_QPA_EGLFS_PHYSICAL_WIDTH=140
		export QT_QPA_EGLFS_PHYSICAL_HEIGHT=200
		export QT_QPA_EGLFS_DEPTH=24
		export QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/event1
		# Run application you want to start
		/usr/local/bin/KlimaLoggProOnBBB &
		;;
	stop)
		echo "Stopping KlimaLoggProOnBBB"
		# Kill application you want to stop
		pkill KlimaLoggProOnBBB
		;;
	*)
		echo "Usage: /etc/init.d/runApp {start|stop}"
		exit 1
		;;
esac

exit 0
