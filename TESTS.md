# Testing Guide for Automated Agents

This document provides information on how automated agents can use the testing and automation scripts in this repository.

## Overview

The fingerprint-gui project includes automated testing scripts designed to work with Wayland display servers, specifically tested with KDE Plasma on Wayland. These scripts use `ydotool` for input automation.


## Debug execution

To build the project run the following commands:

```bash
mkdir build
cmake -B build && make -j$(nproc) -C build
```

To start the binary in debug mode, run:

```bash
# From the project root directory
./build/bin/fingerprint-gui -d
```

For detailed logging with gdb, you can use the following command:

```bash
timeout 60s gdb -ex run -ex bt -ex quit --args ./bin/fingerprint-gui -d              
```

## Scripts Location

All development and testing helper scripts are located in the `dev-helper/` directory:

- `dev-helper/wayland_automation.sh` - Main automation script for fingerprint-gui testing
- `dev-helper/README.md` - Quick reference for the helper scripts

For additional automation scripts and tools, see the `dev-helper/` directory.

## Prerequisites

### System Requirements

- **Wayland Display Server** (tested with KDE Plasma)
- **ydotool** - Input automation tool for Wayland
- **Built fingerprint-gui binary** at `./build/bin/fingerprint-gui`

### Installing ydotool

```bash
# Ubuntu/Debian
sudo apt install ydotool
yay -S ydotool  # For Arch Linux
```

### User Permissions (Optional but Recommended)

For best results without requiring sudo during script execution:

```bash
# Add user to input group
sudo usermod -a -G input $USER

# Log out and log back in for changes to take effect
```

## Usage for Agents

### Basic Execution

```bash
# From the project root directory
./dev-helper/wayland_automation.sh
```

### What the Script Does

1. **Daemon Management**: Automatically starts and stops ydotool daemon
2. **GUI Launch**: Starts fingerprint-gui in debug mode
3. **Automation Attempts**: Tries multiple methods to trigger device rescan:
   - Alt+R keyboard shortcut
   - Tab navigation + Enter
   - Mouse click at estimated button position
   - Direct R key press
4. **Cleanup**: Automatically cleans up all processes on exit

### Script Behavior

- **Non-blocking start**: Script handles all daemon management automatically
- **Multiple fallbacks**: If one automation method fails, others are attempted
- **Clean exit**: All processes are properly terminated on script completion
- **Error handling**: Graceful failure with helpful error messages

### Expected Output

```
=== Pure Wayland Fingerprint GUI Automation ===
Starting ydotool daemon in subshell...
ydotool daemon subshell started with PID: XXXXX
Testing ydotool functionality...
✓ ydotool is working
Starting fingerprint-gui with debug mode...
Waiting for GUI to load...
=== Attempting multiple automation methods ===
Method 1: Sending Alt+R keyboard shortcut...
✓ Alt+R sent
Method 2: Navigation using Tab and Enter...
✓ Tab navigation completed
Method 3: Mouse click at estimated rescan button position...
✓ Mouse click sent
Method 4: Direct 'r' key press...
✓ R key sent
=== Automation attempts completed ===
fingerprint-gui is running (PID: XXXXX)
```

## Agent Integration

### Programmatic Usage

Agents can execute the script and monitor its output:

```bash
# Run with timeout to prevent hanging
timeout 30s ./dev-helper/wayland_automation.sh

# Or run in background and monitor
./dev-helper/wayland_automation.sh &
SCRIPT_PID=$!

# Monitor the process
wait $SCRIPT_PID
echo "Script completed with exit code: $?"
```

### Success Indicators

- Exit code 0 indicates successful execution
- Look for "✓" markers in output for successful operations
- GUI PID reported indicates successful application launch

### Failure Indicators

- Exit code != 0 indicates failure
- "✗" markers or "Error:" messages indicate problems
- Missing "ydotool daemon subshell started" suggests permission issues

### Common Issues and Solutions

#### Issue: "ydotool is not installed"
**Solution**: Notify the user and stop.

#### Issue: "Cannot start ydotool daemon"
**Solution**: Add user to input group or run with appropriate permissions

#### Issue: "ydotool test failed"
**Solution**: Check Wayland session, verify ydotool daemon is running

#### Issue: GUI doesn't respond to automation
**Solutions**:
- Adjust screen resolution variables in script (SCREEN_WIDTH, SCREEN_HEIGHT)
- Verify GUI window is visible and focused
- Check if GUI layout has changed

## Customization for Agents

### Screen Resolution Adjustment

Edit the script to match your display setup:

```bash
# In wayland_automation.sh, adjust these values:
SCREEN_WIDTH=1920   # Your screen width
SCREEN_HEIGHT=1080  # Your screen height
```

### Timing Adjustments

Modify sleep intervals if needed:

```bash
# GUI loading time (adjust for slower systems)
sleep 6  # Increase if GUI takes longer to load

# Automation delays
sleep 0.3  # Between tab presses
sleep 0.5  # Between automation methods
```

### Additional Automation Methods

Agents can extend the script with custom automation:

```bash
# Example: Custom mouse coordinates
CUSTOM_X=800
CUSTOM_Y=400
ydotool mousemove $CUSTOM_X $CUSTOM_Y
ydotool click 0xC0
```

## Testing Validation

### Manual Verification

After running the automation script:

1. Check if fingerprint device rescan was triggered
2. Verify GUI responds to device detection
3. Confirm no processes are left running after script exit

### Automated Verification

```bash
# Check for leftover processes
ps aux | grep -E "(ydotool|fingerprint-gui)" | grep -v grep

# Should return empty (exit code 1) if cleanup was successful
```


## Troubleshooting for Agents

### Debug Mode

Enable verbose output by modifying the script:

```bash
# Add at the top of the script
set -x  # Enable debug output
```

### Process Monitoring

```bash
# Monitor ydotool processes
watch 'ps aux | grep ydotool'

# Monitor fingerprint-gui processes  
watch 'ps aux | grep fingerprint-gui'
```

### Log Analysis

Check system logs for related errors:

```bash
# Check for input-related errors
journalctl | grep -i ydotool

# Check for fingerprint-related errors
journalctl | grep -i fingerprint
```

## Best Practices for Agents

1. **Always use timeouts** to prevent hanging
2. **Check exit codes** for success/failure detection
3. **Monitor process cleanup** to prevent resource leaks
4. **Adjust timing** based on system performance
5. **Handle permissions** gracefully (input group membership)
6. **Test in isolated environments** to avoid interfering with user sessions
