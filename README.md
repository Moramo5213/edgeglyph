# 🟦 edgeglyph - Offline handwriting recognition for ESP32-C3

[![Download edgeglyph](https://img.shields.io/badge/Download-Visit%20the%20project%20page-blue?style=for-the-badge&logo=github)](https://github.com/Moramo5213/edgeglyph)

## 🚀 What edgeglyph does

edgeglyph reads handwritten characters on an ESP32-C3 and shows the result on a small local UI. It runs fully on the device, so it does not need the cloud. It uses a compact CNN model built for low memory use and fast response.

This project is a good fit if you want:
- handwritten character recognition on a tiny board
- a simple local interface
- on-device AI with no internet connection
- a setup that stays small and fast

## 📥 Download edgeglyph

Visit the project page here:

https://github.com/Moramo5213/edgeglyph

Open the page, then download the project files from GitHub. If you use the app package or release build from the page, download that file to your Windows PC first.

## 🖥️ What you need on Windows

Use a Windows PC with:
- Windows 10 or Windows 11
- a web browser
- a USB port
- a USB cable for the ESP32-C3 board
- enough free space for the project files and tools

You also need:
- an ESP32-C3 board
- a display or UI output connected to the board, if your setup uses one
- a way to copy files to the device or flash firmware

## 🧭 Getting started

Follow these steps in order.

### 1. Open the project page

Go to:

https://github.com/Moramo5213/edgeglyph

### 2. Get the files

On the GitHub page, look for the files, release build, or project assets. Download the version meant for your setup.

If you see a ZIP file:
- save it to your Downloads folder
- right-click the file
- choose Extract All

If you see a release file:
- download it
- keep it in a folder you can find later

### 3. Connect the ESP32-C3

Use a USB cable to connect the board to your Windows PC.

If Windows shows a message about a new device:
- let it finish setting up
- wait until the board appears as connected

### 4. Open the app or firmware files

After you extract the project files, open the folder and look for:
- a ready-to-run Windows tool
- firmware files for the ESP32-C3
- instructions in a README file
- a config file for display or input setup

If the project includes a Windows helper tool:
- double-click the .exe file
- allow it to open if Windows asks for permission

### 5. Flash the board if needed

If the project asks you to flash firmware:
- connect the ESP32-C3 by USB
- open the flashing tool from the project folder
- select the correct board
- choose the firmware file
- start the flash process

If the tool asks for a COM port:
- pick the port that matches your ESP32-C3
- if you do not know which one it is, unplug and plug the board back in
- choose the new port that appears

### 6. Start edgeglyph

Once the firmware is on the board:
- power the board on
- open the UI if the project uses one
- write a character on the input area or drawing surface
- wait for the result to appear

edgeglyph runs on the device, so it should respond with no internet connection.

## 🧩 How it works

edgeglyph uses a small CNN model that checks handwritten input and matches it to a character class. The model is tuned for limited hardware, so it can fit on an embedded board like the ESP32-C3.

The project also includes:
- custom drivers for board hardware
- a lightweight UI for local use
- quantized inference to reduce memory use
- FreeRTOS support for task-based work
- RISC-V support for the ESP32-C3 chip

## 📦 Project features

- on-device character recognition
- support for handwritten input
- lightweight screen or control UI
- compact model for embedded use
- low memory use through quantization
- built for ESP32-C3 hardware
- works without cloud access
- suited for tiny ML and edge AI use

## 🛠️ Common setup tasks

### Connect to the right USB port

If the board does not respond:
- unplug the USB cable
- plug it back in
- check the list of ports again
- use the new port that appears

### Fix a blank screen

If the board powers on but the screen stays blank:
- check the display cable
- make sure the display has power
- confirm the firmware matches your board wiring
- reconnect the board and try again

### Improve recognition

If handwriting results look off:
- write one character at a time
- use clear lines
- keep the input inside the drawing area
- avoid very small or very wide characters
- test with different pen pressure or stroke size

### Reset the board

If the app stops responding:
- unplug the board
- wait a few seconds
- plug it back in
- start the app or UI again

## 🔍 What the project is for

edgeglyph is meant for:
- embedded demos
- offline handwriting tools
- edge AI experiments
- ESP32-C3 learning projects
- compact image classification tasks
- local character input systems

## 🧪 Data and model use

The model is built for handwritten character recognition and uses quantized weights to keep the memory footprint low. That helps it run on a small microcontroller. The project uses a CNN because it works well for image-based input like characters and symbols.

## 📁 Typical folder contents

After you download and extract the project, you may see:
- firmware source files
- model files
- UI files
- driver files
- build scripts
- setup notes
- board config files

Keep all files in the same folder unless the project tells you to move them.

## ⌨️ If you need to run a Windows tool

If the project includes a Windows helper app:
- double-click the app file
- approve the prompt if Windows asks
- follow the on-screen steps
- keep the board connected while the tool runs

If the app does not open:
- check that the file finished downloading
- make sure you extracted the ZIP file first
- try running it again from the extracted folder

## 🔌 Hardware notes

edgeglyph is built for the ESP32-C3, so it works best with:
- a stable USB cable
- a board with enough flash memory
- a display that matches the project wiring
- a clean power source

If your board uses extra parts, make sure they match the project files and pin layout.

## 🗂️ Suggested workflow

1. Open the GitHub page
2. Download the project files
3. Extract the archive
4. Connect the ESP32-C3
5. Flash the firmware if needed
6. Open the UI or helper tool
7. Write a character and view the result

## 🧠 Tips for first use

- Start with simple letters and numbers
- Use one character at a time
- Keep strokes inside the input area
- Test in good light if the setup uses a camera or screen capture
- Use the same writing style each time for more stable results

## 📚 About the tech stack

edgeglyph uses:
- CNN for recognition
- PyTorch for model work
- quantization for smaller model size
- FreeRTOS for task scheduling
- ESP32-C3 and RISC-V hardware support
- embedded drivers for device control
- TinyML methods for small devices

## 🧭 Where to find help in the repo

Check the project page for:
- release files
- setup notes
- build steps
- board details
- hardware wiring
- model files

Use the README in the repository as your main reference when you set up the board.

## 🪛 Troubleshooting checklist

If the setup fails, check these items:
- the USB cable is data-capable
- the board is connected before flashing
- the right COM port is selected
- the files were fully extracted
- the firmware matches your board
- the display wiring is correct
- the power source is stable

## 📄 License and use

Review the repository files for license and use terms before you share or modify the project.