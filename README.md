# Flipper Zero IR Mouse

**Turn your Flipper Zero into a wireless USB mouse using infrared signals!**

Flipper Zeroで赤外線リモコンの信号を受信し、Windows PCなどのUSBマウスとして操作するプロジェクトです。

赤外線リモコンのボタンを使って、カーソル移動、クリック、スクロール、マウス速度の調整ができます。

## Features

- **Cursor Movement** – Move the mouse cursor using an infrared remote.
- **Diagonal Movement** – Move the cursor in eight directions.
- **Mouse Acceleration** – Cursor speed gradually increases when holding a direction button, based on received repeat signals.
- **Left Click** – Perform a left mouse click.
- **Right Click** – Perform a right mouse click.
- **Scrolling** – Scroll up and down.
- **Adjustable Mouse Speed** – Change the base mouse speed using Flipper Zero buttons.
- **On-screen GUI** – Display the current mouse status and speed.
- **USB HID Mode** – Connect to a PC as a standard USB mouse.
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
5. Use the remote to control the mouse cursor.

The application remains active until you press the BACK button on your Flipper Zero.

## Remote Control Mapping

The application uses NECext infrared commands.

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

### Mouse Acceleration

When a direction button is held, the application increases cursor movement speed as repeat signals are received.

- Initial speed: Current base mouse speed
- Acceleration step: 4 pixels
- Maximum acceleration speed: 48 pixels

Acceleration resets when a different direction or another command is received.

**Note:** Acceleration depends on the infrared remote sending repeat signals while a button is held.

### Mouse Speed Adjustment

Use the physical buttons on the Flipper Zero:

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

Command definitions:

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

The decoded commands are translated into USB HID mouse actions.

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
- Wireless mouse operation improvements

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