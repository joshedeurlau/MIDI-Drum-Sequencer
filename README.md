# Hardware MIDI Drum Sequencer

An 8 beat programmable drum machine built from scratch using Arduino Uno. This project utilizes a custom coded LED matrix, shift register multiplexing, a 4x4 matrix keypad input, and a Serial to MIDI bridge to trigger audio in real time.

##  Systems Demonstration
| Logic State Input | Hardware Metronome & Multiplexing |
| :---: | :---: |
| ![Building a beat](assets/drum_build.gif) | ![Sweep logic](assets/drum_play.gif) |



## Technical Stack & Hardware Architecture
* **Microcontroller:** Arduino Uno
* **Logic IC:** 74HC595 (8 Bit Shift Register)
* **Firmware:** C++
* **Data Pipeline:** Serial UART from the Arduino UNO -> loopMIDI -> Hairless MIDI Bridge -> Online Web Sequencer (midi.city)
* **Schematic Capture:** EasyEDA


### Pin Configuration Matrix

| Component | Arduino Pin | Control Logic / Mode | Description |
| :--- | :---: | :--- |:--- |
| **Shift Reg (Data)** | `10` | `OUTPUT` | Serial data input line  |
| **Shift Reg (Clock)**| `11` | `OUTPUT` | Shifts serial data into shift register |
| **Shift Reg (Latch)**| `12` | `OUTPUT` | Latches data to storage register |
| **Keypad Rows** | `5, 4, 3, 2` | `INPUT_PULLUP`* | Reads membrane keypad state |
| **Keypad Cols** | `6, 7, 8, 9` | `OUTPUT`* | Drives keypad columns low during scan0 |
| **Matrix Cols 1-7** | `13, A0-A5` | `OUTPUT` (Active High) | Sets column state |
| **Matrix Col 8** | `0 (RX)` | `OUTPUT` | UART RX disabled through `UCSR0B` register override |
| **MIDI Out** | `1 (TX)` | `Serial.write()` | UART Transmit Line using USB Cable |

---

##  Core Engineering Principles 

### 1. Matrix Multiplexing (74HC595)
Since there are not enough I/O pins on the microcontroller to drive the entire matrix directly, the matrix rows (ground paths) are multiplexed using a 74HC595 8 bit shift register. It quickly cycles the active LOW states with the following sequence
```text
  Bit:   Q7   Q6   Q5   Q4   Q3   Q2   Q1   Q0
        [ I ][ I ][ I ][ X ][ X ][ X ][ X ][ S ]
          |    |    |    |    |    |    |    |
          |    |    |    |    |    |    |    +---> [Bit 0]  Matrix Row 8: Sweep Animation (Pin 15)
          |    |    |    +----+----+----+--------> [Bits 1-4] Unused Bits 
          |    |    +----------------------------> [Bit 5]  Matrix Row 3: Kick Drum (Pin 5)
          |    +---------------------------------> [Bit 6]  Matrix Row 2: Snare Drum (Pin 6)
          +--------------------------------------> [Bit 7]  Matrix Row 1: Hi Hat (Pin 7)
```
* **Built-in Memory via 2D Bit Arrays:** The programmed musical sequence is stored in a `3x8` boolean array (`beatMatrix[row][col]`). The display logic then reads these saved bit states to physically draw the corresponding LEDs and trigger the MIDI audio.
* **8-Bit Assembly:** Instead of toggling individual output pins, the code constructs a single 8-bit byte (`rowBits`) during every multiplexing cycle using `bitWrite()`. Bits 5, 6, and 7 pull their states from the 3x8 memory array to control the instrument rows (Kick, Snare, Hi-Hat).
* **Dedicated Hardware Bit for Sweep Animation:** The physical sweeping metronome is coded directly into the lowest bit (Bit 0) of the shift register byte. The sweep animation across row 8 is fully based on the adjustable BPM within the code, which determines the speed of the LED lights.

### 2. Understanding Voltage Difference to Light Up Specific LEDs
In order to turn an LED ON, the microcontroller drives a column to 5V (HIGH) while the shift register pulls a row to 0V (LOW). This creates a forward-biased circuit, allowing current to flow and turn on the LED. Conversely, to keep an LED OFF, the system sets the row to 5V and the column to 0V. Because diodes physically block reverse current, this inverted voltage state stops the electron flow, which keeps the unselected LEDs dark.

### 3. Connection to Real-time Laptop Audio
The sequencer functions as a real digital instrument by generating and transmitting raw MIDI payloads (153 [Note On], Pitch, Speed) over the microcontroller's hardware UART, also know as the Universal Asynchronous Receiver Transmitter (we'll be focussing on the Transmit Pin 1 since this transmits data onto another device). These serial packets from the Arduino code are transmitted via the TX line into a software bridge (Hairless MIDI) and sent through a virtual cable (loopMIDI) to our online browser DAW (Digital Audio Workstation, we used midi.city here)

### Schematics
The complete electrical logic design were mapped out using **EasyEDA**. 

* [View Schematic PDF](hardware/drum_sequencer_schematic.pdf)

## Project Directory Structure

```text
├── hardware/
│   ├── drum_sequencer_schematic.pdf                             # Wiring diagrams and I/O connections
│   ├── drum_sequencer_schematic.tel
│   ├── hardware_top_view.jpg                                    # Photos of Hardware and 74HC595 Shift Register connections
├── src/
│   └── Drum_Beat_LED.ino                                        # Main embedded C++ loop logic
└── README.md                                                    # Project documentation and engineering report
