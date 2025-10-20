# Development Helper Scripts

This directory contains automation and testing scripts for fingerprint-gui development.

## Scripts

### `wayland_automation.sh`
Main automation script for testing fingerprint-gui on Wayland systems.

**Usage:**
```bash
# From project root
./dev-helper/wayland_automation.sh
```

**What it does:**
- Automatically manages ydotool daemon
- Launches fingerprint-gui in debug mode
- Attempts multiple automation methods to trigger device rescan
- Cleans up all processes on exit

**Requirements:**
- Wayland display server (tested with KDE Plasma)
- ydotool package installed
- Built fingerprint-gui binary at `./build/bin/fingerprint-gui`

**For automated agents:**
See the main `TESTS.md` file in the project root for detailed integration information.

## Setup

1. Install ydotool:
   ```bash
   sudo apt install ydotool
   ```

2. (Optional) Add user to input group for better permissions:
   ```bash
   sudo usermod -a -G input $USER
   # Log out and log back in
   ```

3. Build fingerprint-gui:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

4. Run the automation:
   ```bash
   ./dev-helper/wayland_automation.sh
   ```

## Notes

- These scripts are designed for Wayland environments only
- For X11 systems, consider using xdotool instead
- Scripts handle all daemon management automatically
- Clean process termination is guaranteed via trap handlers
