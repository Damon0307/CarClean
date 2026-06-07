#!/bin/sh

# Function to check network connectivity
check_network() {
  local host="www.bing.com"
  ping -c1 "$host" > /dev/null 2>&1
  return $?
}

# Function to start CarClean if not running
start_carclean() {
  local carclean_pattern="/CarClean"

  # Check if CarClean is running using 'ps' and 'grep'
  if ! ps aux | grep -q -E "$carclean_pattern\$"; then
    if [ -x /CarClean ]; then
      echo "Starting CarClean from /CarClean"
      /CarClean &
    else
      echo "Warning: /CarClean executable not found or not executable."
    fi
  fi
}

# Main loop
while true; do
  # Check network connectivity
  if ! check_network; then
    echo "Network not available. Waiting for network..."
    while ! check_network; do
      sleep 1
    done
    echo "Network is now available."
  fi

  # Start CarClean if not already running
  start_carclean

  # Monitor CarClean process every 10 seconds
  carclean_pid=$(ps aux | grep -E "$carclean_pattern\$" | awk '{print $2}')
  if [ -z "$carclean_pid" ]; then
    echo "CarClean unexpectedly stopped. Restarting..."
    start_carclean
  fi

  # Wait for 10 seconds before next check
  sleep 10
done

exit 0