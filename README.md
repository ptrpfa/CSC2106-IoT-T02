# CSC2106 IoT (Team 02): Elderly Tracking System
The increasing number of elderly individuals living independently in Singapore or with conditions such as dementia presents a critical challenge for caregivers and society at large. With a significant portion of the elderly population at risk of getting lost or encountering emergencies without immediate assistance, there is an urgent need for innovative solutions to monitor and ensure their safety within their living environments. 

The absence of effective and precise indoor tracking systems within residential compounds intensifies this issue, as it hinders the ability of caregivers to promptly determine the location and well-being of their elderly dependents and despite the government’s efforts, there remain untapped opportunities for the direct integration of technology into elderly-focused estates.

Building upon these efforts, we propose the development of a comprehensive elderly tracking system, which will be directly integrated with these elderly-focused estates. A hybrid of technologies are used for elderly tracking, with support for geofencing of predefined areas. 

## Team Members
- Peter Febrianto Afandy (*2200959*)
- Lionel Sim Wei Xian (*2201132*)
- Ashley Tay Yong Jun (*2200795*)
- Joshua Lim (*2200687*)
- Colin Ng Kar Jun (*2200920*)

## System Design
Components:
- Flask server
- LILYGO T3-S3
- M5StickCPlus

Technologies:
- painlessMesh
- LoRa


## Getting Started
- Run the LILYGO Main Nodes
    - Ensure the following are installed on Arduino IDE when flashing the firmware to the LILYGO devices:
        - Board Manager
            > Install `esp32 by Espressif Systems`
        - Library Manager
            > Install `ESP8266 and ESP32 OLED driver for SSD1306`
    - When flashing the LILYGO devices, choose `ESP32S3 Dev Module` as the board type

- Run the M5StickCPlus Mesh Nodes
    - Follow the instructions [here](https://docs.m5stack.com/en/quick_start/m5stickc_plus/arduino) to set up Arduino IDE before flashing the firmware to the M5StickCPlus devices.


- Run the Flask server
    1. Ensure that you have `python3` and `pip3` installed on your machine. Click here for installation instructions if they are not already installed.

    2. Create a Python `virtualenv` on your local environment:
        ```
        python3 -m venv .venv
        ```

    3. Activate your created `virtualenv`:
        ```
        source .venv/bin/activate
        ```

    4. Install the necessary program dependencies for this project by running the following command on a terminal:
        ```
        pip3 install -r requirements.txt
        ```
    
    5. Run the server
        ```
        python3 server.py
        ```