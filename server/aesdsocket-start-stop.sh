#!/bin/sh
# aesdsocket-start-stop.sh - start/stop the aesdsocket daemon using start-stop-daemon
# Usage: ./aesdsocket-start-stop.sh {start|stop|restart|status}
# Starts aesdsocket with -d (daemon) option and stops it with SIGTERM for graceful cleanup.

### Configuration
NAME=aesdsocket
# Preferred install location after make install
INSTALLED_DAEMON=/usr/bin/${NAME}
# Fallback to the build directory (same directory as this script)
SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
BUILD_DAEMON="${SCRIPT_DIR}/${NAME}"
PIDFILE=/var/run/${NAME}.pid
DATAFILE=/var/tmp/aesdsocketdata

# Determine which daemon binary to use
if [ -x "$BUILD_DAEMON" ]; then
  DAEMON="$BUILD_DAEMON"
elif [ -x "$INSTALLED_DAEMON" ]; then
  DAEMON="$INSTALLED_DAEMON"
else
  echo "Error: aesdsocket binary not found (looked for $BUILD_DAEMON and $INSTALLED_DAEMON)" >&2
  exit 1
fi

start() {
  if [ -f "$PIDFILE" ] && start-stop-daemon --quiet --status --pidfile "$PIDFILE"; then
    echo "$NAME already running"
    return 0
  fi
  echo "Starting $NAME"
  start-stop-daemon --start --quiet \
    --background \
    --make-pidfile --pidfile "$PIDFILE" \
    --exec "$DAEMON" -- -d
  RET=$?
  if [ $RET -ne 0 ]; then
    echo "Failed to start $NAME (rc=$RET)" >&2
  fi
  return $RET
}

stop() {
  if [ ! -f "$PIDFILE" ]; then
    echo "$NAME not running (no pidfile)"
    return 0
  fi
  echo "Stopping $NAME"
  # Send SIGTERM allowing daemon to remove its data file, retry then KILL if needed
  start-stop-daemon --stop --quiet --pidfile "$PIDFILE" --signal TERM --retry=TERM/5/KILL/2
  RET=$?
  if [ $RET -eq 0 ]; then
    rm -f "$PIDFILE"
  else
    echo "Failed to stop $NAME (rc=$RET)" >&2
  fi
  # Ensure data file removed if daemon already cleaned up
  [ -f "$DATAFILE" ] && rm -f "$DATAFILE"
  return $RET
}

status() {
  if [ -f "$PIDFILE" ] && start-stop-daemon --quiet --status --pidfile "$PIDFILE"; then
    echo "$NAME is running (pid $(cat "$PIDFILE"))"
    return 0
  else
    echo "$NAME is not running"
    return 3
  fi
}

case "$1" in
  start) start ;;
  stop) stop ;;
  restart) stop; start ;;
  status) status ;;
  *) echo "Usage: $0 {start|stop|restart|status}" >&2; exit 2 ;;
 esac
