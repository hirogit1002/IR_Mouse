# Flipper Zero IR Mouse

**Turn your Flipper Zero into a wireless USB mouse and keyboard using infrared signals!**

Flipper Zeroで赤外線リモコンの信号を受信し、Windows PCなどをUSB HIDデバイスとして操作するプロジェクトです。

赤外線リモコンを使って、マウス操作だけでなく、英数字の文字入力もできます。

## Features

### Mouse Features

- **Cursor Movement** – Move the mouse cursor using an infrared remote.
- **Diagonal Movement** – Move the cursor in eight directions.
- **Mouse Acceleration** – Cursor speed gradually increases when holding a direction button, based on received repeat signals.
- **Left Click** – Perform a left mouse click.
- **Right Click** – Perform a right mouse click.
- **Scrolling** – Scroll up and down.
- **Adjustable Mouse Speed** – Change the base mouse speed using Flipper Zero buttons.

### Keyboard Features

- **Text Input Mode** – Switch between mouse and keyboard modes using the MENU infrared command.
- **Multi-tap Text Input** – Enter English letters using the numeric buttons (0–9).
- **Character Confirmation** – Confirm the pending character immediately without waiting for the timeout.
- **Enter Key** – Send Enter when no character is waiting for confirmation.
- **Backspace** – Delete characters using the LEFT button or dedicated BKSP infrared command.
- **Space Input** – Send a space using the RIGHT button.
- **Uppercase / Lowercase Toggle** – Switch between uppercase and lowercase letters.
- **Number Mode** – Switch to direct numeric input.
- **Japanese IME Support** – Send English letters to Windows, allowing Japanese IME conversion when enabled.

### General Features

- **On-screen GUI** – Display the current mode, mouse status, speed, and pending character.
- **USB HID Mode** – Connect to a PC as a standard USB mouse and keyboard.
- **USB CDC Restore** – Automatically restore USB CDC mode when the application exits.

## Demo / Screenshots

Screenshots and demonstration videos will be added here.

## Hardware Requirements

- Flipper Zero
- Infrared remote control
- USB cable
- PC with USB HID support

## Software Requirements

| Component | Version |
|---|---|
| Flipper Zero Firmware | Official Firmware v1.5.1 |
| uFBT | Compatible with Target 7 |
| SDK API | 88.2 |
| Development OS | Ubuntu Linux |
| Target OS | Windows 11 |

Other firmware versions may work, but have not been tested.

## Installation

### 1. Clone the repository

```bash
git clone https://github.com/YOUR_USERNAME/ir_mouse_project.git
cd ir_mouse_project
```

Replace `YOUR_USERNAME` with your GitHub username.

### 2. Build the application

```bash
ufbt
```

After a successful build, the application file will be generated:

```text
dist/ir_mouse.fap
```

### 3. Install and launch

Connect your Flipper Zero and run:

```bash
ufbt launch
```

Close qFlipper before launching if it is using the USB serial connection.

## How to Use

1. Launch IR Mouse on your Flipper Zero.
2. Connect the Flipper Zero to your PC via USB.
3. The Flipper Zero switches to USB HID mode.
4. Point your infrared remote at the Flipper Zero.
5. Use the remote to control the mouse or enter text.

The application supports two operating modes:

- **Mouse Mode** – Control the PC mouse.
- **Keyboard Mode** – Enter text using multi-tap numeric input.

Press the BACK button on the Flipper Zero to return from keyboard mode to mouse mode. Press BACK again in mouse mode to exit the application.

## Operating Modes

### Switching Modes

Use the MENU infrared command to switch between mouse mode and keyboard mode.

| Remote Button | Function |
|---|---|
| MENU | Toggle Mouse / Keyboard Mode |

### Mouse Mode

In mouse mode, the remote controls the PC mouse.

See the Remote Control Mapping section below.

### Keyboard Mode

Keyboard mode allows you to enter English letters and numbers using the numeric buttons on your infrared remote.

The application uses a multi-tap input method similar to older mobile phones.

## Remote Control Mapping

The application uses NECext infrared commands.

### Mouse Mode Mapping

| Remote Button | Function |
|---|---|
| OK | Left Click |
| LEFT CLICK | Left Click |
| RIGHT CLICK | Right Click |
| UP | Move Cursor Up |
| DOWN | Move Cursor Down |
| LEFT | Move Cursor Left |
| RIGHT | Move Cursor Right |
| ALT UP | Move Cursor Up |
| ALT DOWN | Move Cursor Down |
| ALT LEFT | Move Cursor Left |
| ALT RIGHT | Move Cursor Right |
| RIGHT DOWN | Move Cursor Diagonally Down-Right |
| RIGHT UP | Move Cursor Diagonally Up-Right |
| LEFT DOWN | Move Cursor Diagonally Down-Left |
| LEFT UP | Move Cursor Diagonally Up-Left |
| SCROLL UP | Scroll Up |
| SCROLL DOWN | Scroll Down |
| MENU | Switch to Keyboard Mode |

### Keyboard Mode Mapping

| Remote Button | Function |
|---|---|
| 0–9 | Multi-tap character input |
| OK | Confirm the pending character |
| RIGHT CLICK | Confirm pending character, or send Enter if no character is pending |
| BKSP | Send Backspace |
| LEFT | Send Backspace |
| RIGHT | Send Space |
| UP | Toggle uppercase / lowercase |
| DOWN | Toggle number mode |
| MENU | Switch to Mouse Mode |

### Multi-tap Character Mapping

Each numeric button represents a group of characters.

| Button | Characters |
|---|---|
| 0 | Space, 0 |
| 1 | `. , ? ! 1` |
| 2 | a, b, c |
| 3 | d, e, f |
| 4 | g, h, i |
| 5 | j, k, l |
| 6 | m, n, o |
| 7 | p, q, r, s |
| 8 | t, u, v |
| 9 | w, x, y, z |

Press the same numeric button repeatedly to cycle through its characters.

For example:

```text
2       → a
2, 2    → b
2, 2, 2 → c
```

The selected character remains pending for approximately one second.

- Press the same number again before the timeout to select the next character.
- Press a different number to commit the previous character and begin a new one.
- Wait approximately one second to commit the pending character automatically.
- Press OK or RIGHT CLICK to commit the pending character immediately.

### RIGHT CLICK: Character Confirmation / Enter

The RIGHT CLICK infrared command has two functions in keyboard mode:

| Input State | Function |
|---|---|
| A character is pending | Confirm the pending character immediately |
| No character is pending | Send Enter |

This allows one remote button to perform both character confirmation and Enter.

In mouse mode, the same command continues to perform a normal right click.

### Number Mode

Press DOWN to toggle number mode.

In number mode, numeric buttons send digits directly instead of using multi-tap character selection.

Press DOWN again to return to normal multi-tap input mode.

### Uppercase / Lowercase

Press UP to toggle between uppercase and lowercase input.

For example:

```text
Lowercase: hello
Uppercase: HELLO
```

### Japanese IME Support

The application sends English letters and numbers through USB HID.

When Windows Japanese IME is enabled, the received letters can be converted into Japanese text using the normal IME conversion process.

The actual Japanese conversion behavior depends on the Windows IME settings.

## Mouse Acceleration

When a direction button is held, the application increases cursor movement speed as repeat signals are received.

- Initial speed: Current base mouse speed
- Acceleration step: 4 pixels
- Maximum acceleration speed: 48 pixels

Acceleration resets when a different direction or another command is received.

**Note:** Acceleration depends on the infrared remote sending repeat signals while a button is held.

## Mouse Speed Adjustment

Use the physical buttons on the Flipper Zero while in mouse mode:

| Flipper Zero Button | Function |
|---|---|
| LEFT | Decrease base mouse speed |
| RIGHT | Increase base mouse speed |
| BACK | Exit application |

Available base speed levels:

```text
4 / 8 / 12 / 16 / 20 / 24 px
```

Default speed:

```text
12 px
```

## Infrared Command Configuration

The infrared commands are defined in `ir_mouse.c`.

The current remote address is:

```c
#define REMOTE_ADDRESS 0x00007201
```

### Mouse Command Definitions

```c
#define CMD_OK          0x0000A05F
#define CMD_UP          0x0000FC03
#define CMD_DOWN        0x0000A15E
#define CMD_RIGHT       0x0000BF40
#define CMD_LEFT        0x0000FD02

#define CMD_ALT_UP      0x0000E01F
#define CMD_ALT_DOWN    0x0000E817
#define CMD_ALT_RIGHT   0x0000AB54
#define CMD_ALT_LEFT    0x0000E916

#define CMD_RIGHT_DOWN  0x0000AF50
#define CMD_RIGHT_UP    0x0000A758
#define CMD_LEFT_DOWN   0x0000ED12
#define CMD_LEFT_UP     0x0000E51A

#define CMD_RIGHT_CLICK 0x0000B34C
#define CMD_LEFT_CLICK  0x0000E41B

#define CMD_SCR_UP      0x0000B748
#define CMD_SCR_DOWN    0x0000BB44
```

### Keyboard Command Definitions

```c
#define CMD_MENU        0x0000F807
#define CMD_BKSP        0x0000E718

#define CMD_NUM_0       0x0000EC13
#define CMD_NUM_1       0x0000E51A
#define CMD_NUM_2       0x0000E01F
#define CMD_NUM_3       0x0000A758
#define CMD_NUM_4       0x0000E916
#define CMD_NUM_5       0x0000E41B
#define CMD_NUM_6       0x0000AB54
#define CMD_NUM_7       0x0000ED12
#define CMD_NUM_8       0x0000E817
#define CMD_NUM_9       0x0000AF50
```

**Important:** Some numeric infrared commands intentionally share the same values as mouse commands.

The application interprets these commands according to the current operating mode:

- In mouse mode, they perform the original mouse actions.
- In keyboard mode, they are interpreted as numeric input.

If you use a different infrared remote, you may need to change the address and command values to match your remote.

## Project Structure

```text
ir_mouse_project/
│
├── ir_mouse.c
├── application.fam
├── README.md
│
└── dist/
    └── ir_mouse.fap
```

## Technical Details

This project uses the Flipper Zero infrared worker to receive and decode infrared signals.

The decoded commands are translated into USB HID mouse and keyboard actions.

Main APIs:

| API | Purpose |
|---|---|
| `infrared_worker_alloc()` | Allocate infrared worker |
| `infrared_worker_rx_start()` | Start infrared reception |
| `infrared_worker_get_decoded_signal()` | Read decoded infrared signal |
| `furi_hal_hid_mouse_move()` | Move mouse cursor |
| `furi_hal_hid_mouse_press()` | Press mouse button |
| `furi_hal_hid_mouse_release()` | Release mouse button |
| `furi_hal_hid_mouse_scroll()` | Scroll mouse wheel |
| `furi_hal_hid_kb_press()` | Press keyboard key |
| `furi_hal_hid_kb_release()` | Release keyboard key |
| `furi_hal_hid_kb_release_all()` | Release all keyboard keys |
| `furi_hal_usb_set_config()` | Switch USB interface |

USB HID mode is used while the application is running. When the application exits, USB CDC mode is restored so that qFlipper can reconnect.

## Tested Environment

Successfully built and tested with:

- Flipper Zero
- Official Firmware v1.5.1
- Target 7
- API 88.2
- uFBT
- Ubuntu Linux development environment
- Windows 11 target PC

All implemented functions were successfully tested on the hardware.

## Future Improvements

Possible future features:

- Customizable infrared command mapping
- Adjustable acceleration settings
- Configurable scroll speed
- Support for additional infrared protocols
- On-screen configuration menu
- Additional text input features
- Customizable multi-tap character mapping

## License

No license has been specified yet.

If you want others to use, modify, and distribute this project, consider adding an open-source license such as MIT.

## Acknowledgements

- [Flipper Zero](https://flipperzero.one/)
- [Flipper Devices](https://github.com/flipperdevices)
- [uFBT](https://github.com/flipperdevices/flipperzero-ufbt)

---

**Made with Flipper Zero, infrared signals, and USB HID.**

Enjoy controlling your PC with an infrared remote!