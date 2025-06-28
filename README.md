# NodeMCU ESP8266 Pool Pump Controller with Home Assistant

This project enables you to control a pool pump remotely using a NodeMCU ESP8266 microcontroller integrated with Home Assistant via MQTT. The system allows you to turn the pump on/off manually or automate it based on a schedule, providing a cost-effective solution for pool maintenance.

#### After the first USB flash of the **NodeMCU (ESP8266)**, all subsequent program uploads can be done over Wi-Fi.

This makes it easy to place the **NodeMCU (ESP8266)** control board in a fixed location—such as the basement near the VFD/controller—without needing to remove it or bring it back to a USB port or the need for a laptop. **All the ESP8266 requires is a Wi-Fi connection on your local network.**

![image](https://github.com/user-attachments/assets/24ec2ab9-2674-4038-be94-181913472a68)

#### If you’ve ever found yourself wanting to control your pool filter through Home Assistant—like I did—this project might give you a solid starting point. I built this so I could monitor and control the pool filter remotely while we travel. It’s been helpful for peace of mind, and maybe it’ll be useful for you too.

#### A quick note: this is my first time publishing a project to GitHub, so please go easy on me. Also, the README was mostly generated with the help of AI. If something doesn’t quite make sense… let’s just blame the AI. 😉

## Table of Contents
- [Features](#features)
- [Prerequisites](#prerequisites)
- [Hardware Requirements](#hardware-requirements)
- [Wiring Diagram](#wiring-diagram)
- [Software Setup](#software-setup)
  - [Flashing the NodeMCU](#flashing-the-nodemcu)
  - [Configuring Home Assistant](#configuring-home-assistant)
- [Usage](#usage)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)

## Features
- **Remote Control**: Turn the pool pump on/off via Home Assistant’s dashboard or mobile app.
- **Scheduled Automation**: Run the pump on a daily schedule (e.g., on at 8:00 AM, off at 2:00 PM).
- **MQTT Integration**: Communicates with Home Assistant using lightweight MQTT protocol.
- **Reliable State Tracking**: Ensures the pump’s state is accurately reflected in Home Assistant.
- **Simple Hardware**: Uses a single relay module for easy setup.

## Prerequisites
- Basic knowledge of Arduino programming and Home Assistant configuration.
- An MQTT broker (e.g., Mosquitto) installed and running, accessible by both the NodeMCU and Home Assistant.
- A Home Assistant instance (version 2023.6 or later recommended).
- Arduino IDE installed for flashing the NodeMCU.

## Hardware Requirements
- **NodeMCU ESP8266** (e.g., NodeMCU V3).
![2025-06-23_13-10-28](https://github.com/user-attachments/assets/e0309f54-5547-42d5-b8e0-0b26bbacd5b0)

- **Single-channel 5V relay module** (active-low, compatible with 3.3V logic).
![2025-06-23_13-09-17](https://github.com/user-attachments/assets/27a46428-353d-4dec-b65a-47f5186acbe7)

- **Pool pump** (AC-powered, compatible with relay rating, typically <10A).
- **Jumper wires** and a **breadboard** (optional).
- **5V USB power adapter** for the NodeMCU.
- **Safety equipment**: Electrical enclosure, insulation tape, and tools for AC wiring (consult an electrician if unsure).

**Warning**: Working with high-voltage devices like pool pumps can be dangerous. Ensure all connections are properly insulated and comply with local electrical codes. Consult a licensed electrician for AC wiring.

## Wiring Diagram
![wiring_diagram](https://github.com/user-attachments/assets/9fe858d5-0e4d-46ed-953d-0b4e7586b404)

Connect the components as follows:

| NodeMCU Pin | Relay Module Pin | Description |
|-------------|------------------|-------------|
| D2 (GPIO4)  | IN               | Relay control signal |
| 3.3V        | VCC              | Relay power (or 5V if required by your module) |
| GND         | GND              | Ground |
| VIN (5V)    | USB adapter      | Power for NodeMCU (via USB or VIN) |

- **AC Wiring**: Connect the relay’s NO (normally open) and COM (common) terminals in series with the pool pump’s live wire. Ensure the rating matches the pool pump’s power requirements.
- **Safety**: Place all components in a waterproof enclosure for outdoor use.

**Note**: The `main.ino` code assumes the relay is active-low (HIGH on D2 turns it OFF, LOW turns it ON). Adjust the code if your relay is active-high.

## Software Setup

### Flashing the NodeMCU
1. **Install ESP8266 Board Support**:
   - Open Arduino IDE, go to **File > Preferences**.
   - Add `http://arduino.esp8266.com/stable/package_esp8266com_index.json` to **Additional Boards Manager URLs**.
   - Go to **Tools > Board > Boards Manager**, search for `esp8266`, and install the latest version.
   - Select **NodeMCU 1.0 (ESP-12E Module)** under **Tools > Board**.

2. **Install Libraries**:
   - Install the following libraries via **Sketch > Include Library > Manage Libraries**:
     - `ESP8266WiFi` (included with ESP8266 board support).
     - `PubSubClient` by Nick O’Leary (for MQTT).

3. **Configure `main`**:
   - Open `main.ino` from the repository.
   - Update the following variables with your settings:
     ```cpp
     const char* ssid = "YOUR_WIFI_SSID";
     const char* password = "YOUR_WIFI_PASSWORD";
     const char* mqtt = "YOUR_MQTT_BROKER_IP";
     const char* mqtt_user = "YOUR_MQTT_USERNAME"; // Optional
     const char* mqtt_password = "YOUR_MQTT_PASSWORD"; // Optional
     ```
   - Ensure `RELAY_PIN` is set to `D2` (GPIO4) or your chosen pin:
     ```cpp
     #define RELAY_PIN D2
     ```

4. **Flash the Code**:
   - Connect the NodeMCU to your computer via USB.
   - Select the correct port under **Tools > Port**.
   - Click **Upload** to flash the code.
   - Open the Serial Monitor (**Tools > Serial Monitor**, 115200 baud) to verify Wi-Fi and MQTT connection.

### Configuring Home Assistant
1. **Add MQTT**:
   - Edit your Home Assistant `configuration.yaml` (typically in `~/.homeassistant/`).
   - Add the following under the `switch:` section:
     ```yaml
     switch:
       - platform: mqtt
         name: "Pool Pump"
         state_topic: "pool_pump/state"
         command_topic: "pool_pump/set"
         payload_on: "ON"
         payload_off: "OFF"
         optimistic: false
         qos: 0
         retain: true
     ```
   - Ensure your MQTT broker is configured in `configuration.yaml`:
     ```yaml
     mqtt:
       broker: YOUR_MQTT_BROKER_IP
       username: YOUR_MQTT_USERNAME # Optional
       password: YOUR_MQTT_PASSWORD # Optional
     ```

2. **Add Automations**:
   - Create or edit `automations.yaml` (ensure it’s included in `configuration.yaml` via `automation: !include automations.yaml`).
   - Add the following to schedule the pool pump (runs from 8:00 AM to 2:00 PM):
     ```yaml
     - id: 'pool_pump_schedule'
       alias: Pool Pump Schedule
       trigger:
         - platform: time
           at: 08:00:00' '08:00'
       condition:
         - condition: state
           entity_id: switch.pool_pump
           state: 'off'
       action:
         - service: switch.turn_on
           entity_id: switch.pool_pump
     - id: 'pool_pump_off_schedule'
       alias: Pool Pump Off Schedule
       trigger:
         - platform: time
           at: '14:00:00'
       condition:
         - condition: state
           entity_id: switch.pool_pump
           state: 'on'
       action:
         - service: switch.turn_off
           entity_id: switch.pool_pump
     ```

3. **Restart Home Assistant**:
   - Save the YAML files and restart Home Assistant via **Configuration > Server Controls > Restart**.
   - Verify the `switch.pool_pump` entity appears in the Home Assistant dashboard under **Overview** or **Entities**.

## Usage
- **Manual Control**: Toggle the `Pool Pump` switch in the Home Assistant’s dashboard or app to turn the pump on/off.
- **Scheduled Operation**: The pump automatically turns on at 8:00 AM and off at 2:00 PM daily, unless manually overridden.
- **Customization**: Edit `automations.yaml` to adjust schedules or add conditions (e.g., weather-based triggers, if supported by your Home Assistant setup).

## Troubleshooting
- **NodeMCU Not Connecting to Wi-Fi**:
  - Check `ssid` and `password` in `main.ino`.
  - Ensure the NodeMCU is within Wi-Fi range.
  - Monitor Serial output for error messages.
- **MQTT Connection Fails**:
  - Verify the MQTT broker IP, username, and password.
  - Ensure the broker is running and accessible (e.g., `mosquitto -v` on the broker machine).
  - Check firewall settings to allow MQTT port (default: 1883).
- **Switch Not Appearing in Home Assistant**:
  - Validate YAML syntax using Home Assistant’s **Configuration > YAML** Validation.
  - Confirm MQTT topics (`pool_pump/state`, `pool_pump/set`) match `main.ino`.
  - Check Home Assistant logs (**Configuration > Logs**) for MQTT errors.
- **Relay Not Switching**:
  - Confirm wiring matches the diagram.
  - Test the relay with a simple Arduino sketch (e.g., toggle `D2` HIGH/LOW).
  - Ensure the relay module supports 3.3V logic or use a level shifter if needed.

## Contributing
Contributions are welcome! To contribute:
1. Fork the repository.
2. Create a feature branch (`git checkout -b feature/new-feature`).
3. Commit changes (`git commit -m "Add new feature"`).
4. Push to the branch (`git push origin feature/new-feature`).
5. Open a Pull Request with a detailed description.

Please report issues via the GitHub Issues tab, including logs and steps to reproduce.

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
