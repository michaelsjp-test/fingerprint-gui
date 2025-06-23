#!/bin/bash

# Pure Wayland automation script for fingerprint-gui using ydotool
# This script works specifically with KDE Wayland

set -e

echo "=== Pure Wayland Fingerprint GUI Automation ==="

# Check if ydotool is available
if ! command -v ydotool &> /dev/null; then
    echo "Error: ydotool is not installed"
    echo "Install with: sudo apt install ydotool"
    echo "Or compile from source: https://github.com/ReimuNotMoe/ydotool"
    exit 1
fi

# Global variables to track daemon and GUI PIDs
YDOTOOL_SUBSHELL_PID=""
DAEMON_STARTED_BY_SCRIPT=false
GUI_PID=""

# Function to stop any existing ydotool daemon processes
stop_existing_ydotool_daemon() {
    # Kill any existing daemon processes to avoid conflicts
    if pgrep -x ydotoold &> /dev/null; then
        echo "Stopping existing ydotool daemon..."
        pkill -f ydotoold 2>/dev/null || true
        sleep 2
    fi
}

# Function to start ydotool daemon in a subshell
start_ydotool_daemon() {
    stop_existing_ydotool_daemon
    
    echo "Starting ydotool daemon in subshell..."
    
    # Start daemon in a subshell that we can control
    (
        # Try to start daemon without sudo first
        if ydotoold &>/dev/null; then
            wait
        else
            # Fallback to sudo if needed
            sudo ydotoold &>/dev/null
            wait
        fi
    ) &
    
    YDOTOOL_SUBSHELL_PID=$!
    DAEMON_STARTED_BY_SCRIPT=true
    echo "ydotool daemon subshell started with PID: $YDOTOOL_SUBSHELL_PID"
    
    # Give daemon time to start
    sleep 3
}

# Function to test ydotool
test_ydotool() {
    echo "Testing ydotool functionality..."
    if ydotool type "test" 2>/dev/null; then
        echo "✓ ydotool is working"
        # Clear the test text
        sleep 0.5
        ydotool key 14:1 14:0 14:1 14:0 14:1 14:0 14:1 14:0  # Backspace x4
        return 0
    else
        echo "✗ ydotool test failed"
        return 1
    fi
}

# Cleanup function
cleanup() {
    echo ""
    echo "=== Cleaning up ==="
    
    if [ -n "$GUI_PID" ]; then
        echo "Stopping fingerprint-gui..."
        kill $GUI_PID 2>/dev/null || echo "GUI application already closed"
    fi

    if [ "$DAEMON_STARTED_BY_SCRIPT" = true ] && [ -n "$YDOTOOL_SUBSHELL_PID" ]; then
        echo "Stopping ydotool daemon subshell (PID: $YDOTOOL_SUBSHELL_PID)..."
        kill $YDOTOOL_SUBSHELL_PID 2>/dev/null || echo "ydotool daemon subshell already stopped"
        # Wait a moment for cleanup
        sleep 1
    fi
    
    # Ensure no ydotool processes are left running
    stop_existing_ydotool_daemon
    
    echo "Cleanup completed!"
}

# Set up trap for cleanup on script exit
trap cleanup EXIT

# Stop any existing ydotool daemon processes at start
stop_existing_ydotool_daemon

# Start daemon
start_ydotool_daemon

# Test ydotool
if ! test_ydotool; then
    echo "Error: ydotool is not working properly"
    exit 1
fi

# Change to the project directory
cd /home/jules/Repos/fingerprint-gui

echo "Starting fingerprint-gui with debug mode..."
./build/bin/fingerprint-gui -d &
GUI_PID=$!

echo "Waiting for GUI to load..."
sleep 6

echo ""
echo "=== Attempting multiple automation methods ==="

# Method 1: Alt+R keyboard shortcut
echo "Method 1: Sending Alt+R keyboard shortcut..."
# Alt key code: 56, R key code: 19
# Format: keycode:1 for press, keycode:0 for release
ydotool key 56:1 19:1 19:0 56:0
sleep 1
echo "✓ Alt+R sent"

# Method 2: Direct key sequence to navigate to rescan button
echo ""
echo "Method 2: Navigation using Tab and Enter..."
# Send multiple tabs to navigate to the rescan button
for i in {1..5}; do
    ydotool key 15:1 15:0  # Tab key (code 15)
    sleep 0.3
done
# Press Enter to activate the button
ydotool key 28:1 28:0  # Enter key (code 28)
sleep 0.5
echo "✓ Tab navigation completed"

# Method 3: Mouse click at estimated position
echo ""
echo "Method 3: Mouse click at estimated rescan button position..."
# Get screen resolution and estimate button position
SCREEN_WIDTH=1920  # Default, adjust as needed
SCREEN_HEIGHT=1080
BUTTON_X=$((SCREEN_WIDTH / 2))
BUTTON_Y=$((SCREEN_HEIGHT / 3))

echo "Moving mouse to estimated position: ${BUTTON_X},${BUTTON_Y}"
ydotool mousemove $BUTTON_X $BUTTON_Y
sleep 0.5
ydotool click 0xC0  # Left click (0xC0)
echo "✓ Mouse click sent"

# Method 4: Type the shortcut key directly
echo ""
echo "Method 4: Direct 'r' key press (assuming Alt is still held)..."
ydotool key 19:1 19:0  # R key
sleep 0.5
echo "✓ R key sent"

echo ""
echo "=== Automation attempts completed ==="
echo "fingerprint-gui is running (PID: $GUI_PID)"
echo ""
echo "If none of the automated methods worked, you can manually:"
echo "1. Click on the rescan button in the GUI"
echo "2. Or press Alt+R while the GUI is focused"
echo "3. Or use: ydotool mousemove X Y && ydotool click 0xC0"
echo "   (where X Y are the coordinates of the rescan button)"

# Wait for user input
echo ""
echo "The script will clean up automatically when you press Enter or Ctrl+C"
read -p "Press Enter to exit and clean up..."
