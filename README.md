# GD32VW553 - Wi-Fi SoftAP LED Control

This project is an example application for the **GD32VW553** microcontroller. It demonstrates how to control three LEDs through a web interface served directly by the board.

The firmware configures the GD32VW553 as a **Wi-Fi SoftAP**, starts an embedded **HTTP server**, and provides a simple HTML page that can be accessed from a phone or computer connected to the board's Wi-Fi network.

---

## Overview

This project demonstrates:

- GD32VW553 SDK initialization
- Wi-Fi operation in SoftAP mode
- Embedded HTTP server using LwIP sockets
- GPIO output control
- HTML, CSS, and JavaScript embedded in firmware
- HTTP GET commands to control hardware
- RTOS task creation and execution


Download Putty for testing:
https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html
---

## Hardware

- Microcontroller: **GD32VW553**
- SDK: **GD32VW55x SDK / MSDK**
- Three LEDs connected to GPIO pins:

| LED | GPIO Port | Pin |
|---|---|---|
| LED1 | GPIOB | PB0 |
| LED2 | GPIOA | PA12 |
| LED3 | GPIOB | PB4 |

If your board uses different LED pins, update the LED definitions in `main.c`.

---

## How It Works

At startup, the firmware performs the following sequence:

```text
Microcontroller boot
   ↓
RTOS initialization
   ↓
Platform initialization
   ↓
Application initialization
   ↓
Wi-Fi initialization
   ↓
SoftAP task creation
   ↓
SoftAP startup
   ↓
LED GPIO initialization
   ↓
HTTP server startup
   ↓
LED control from browser
```

The board creates its own Wi-Fi network. After connecting to this network, the user opens a browser and accesses the web page hosted by the GD32VW553.

---

## Wi-Fi SoftAP Configuration

The main SoftAP parameters are defined in `main.c`:

```c
#define SOFTAP_SSID            "GD32-LED_SeuNome"
#define SOFTAP_PASSWORD        "12345678"
#define SOFTAP_CHANNEL         11
#define SOFTAP_HIDDEN          0
```

### Parameters

| Parameter | Description |
|---|---|
| `SOFTAP_SSID` | Name of the Wi-Fi network created by the board |
| `SOFTAP_PASSWORD` | Wi-Fi password |
| `SOFTAP_CHANNEL` | Wi-Fi channel used by the SoftAP |
| `SOFTAP_HIDDEN` | Defines whether the SSID is visible or hidden |

To create an open Wi-Fi network, use an empty password:

```c
#define SOFTAP_PASSWORD        ""
```

---

## How to Use

1. Build and flash the firmware to the GD32VW553 board.
2. Reset the board.
3. On a phone or computer, connect to the Wi-Fi network:

```text
GD32-LED_SeuNome
```

4. Use the default password:

```text
12345678
```

5. Open a browser and access:

```text
http://192.168.4.1/
```

6. Use the web page to control the LEDs.

If `192.168.4.1` does not work, check the gateway IP address assigned to the phone or computer by the board's SoftAP.

---

## Web Interface

The embedded web page allows the user to:

- Turn LED1 on
- Turn LED1 off
- Turn LED2 on
- Turn LED2 off
- Turn LED3 on
- Turn LED3 off
- Turn all LEDs on
- Turn all LEDs off
- Run a demo LED sequence
- Read the current LED status

The page is stored directly in firmware as a C string:

```c
static const char k_index_html[] = "...";
```

---

## Available HTTP Routes

The HTTP server handles simple `GET` requests.

| Route | Function |
|---|---|
| `/` | Returns the main HTML page |
| `/led1/on` | Turns LED1 on |
| `/led1/off` | Turns LED1 off |
| `/led2/on` | Turns LED2 on |
| `/led2/off` | Turns LED2 off |
| `/led3/on` | Turns LED3 on |
| `/led3/off` | Turns LED3 off |
| `/all/on` | Turns all LEDs on |
| `/all/off` | Turns all LEDs off |
| `/demo` | Runs a demo LED sequence |
| `/status` | Returns the current LED states in JSON format |
| `/favicon.ico` | Returns an empty response to avoid browser errors |

---

## `/status` Response Example

Accessing:

```text
http://192.168.4.1/status
```

returns a JSON response:

```json
{
  "led1": 1,
  "led2": 0,
  "led3": 1
}
```

Value meaning:

| Value | Meaning |
|---|---|
| `0` | LED off |
| `1` | LED on |

---

## LED Configuration

The LED pins are configured in `main.c`:

```c
#define LED1_PORT              GPIOB
#define LED1_PIN               GPIO_PIN_0
#define LED1_RCU               RCU_GPIOB

#define LED2_PORT              GPIOA
#define LED2_PIN               GPIO_PIN_12
#define LED2_RCU               RCU_GPIOA

#define LED3_PORT              GPIOB
#define LED3_PIN               GPIO_PIN_4
#define LED3_RCU               RCU_GPIOB
```

---

## LED Polarity

Some boards turn LEDs on with a high logic level. Others turn LEDs on with a low logic level.

This behavior is configured with:

```c
#define LED_ACTIVE_LOW         0
```

Use:

```c
#define LED_ACTIVE_LOW         0
```

for active-high LEDs.

Use:

```c
#define LED_ACTIVE_LOW         1
```

for active-low LEDs.

---

## Main Functions

### `main()`

Application entry point.

It initializes the RTOS, initializes the platform, starts the application, and starts the scheduler:

```c
sys_os_init();
platform_init();
application_init();
sys_os_start();
```

---

### `application_init()`

Initializes SDK components and starts the Wi-Fi application task.

When Wi-Fi initialization succeeds, it creates the task:

```c
softap_http_led_task
```

---

### `softap_http_led_task()`

Main application task.

Responsibilities:

- Configure SSID, password, Wi-Fi channel, and authentication mode
- Start the SoftAP
- Initialize the LED GPIOs
- Start the HTTP server loop

---

### `leds_init()`

Configures the LED GPIO pins as digital outputs.

---

### `leds_apply()`

Applies the current software LED states to the physical GPIO pins.

The LED states are stored in:

```c
led1_state
led2_state
led3_state
```

---

### `http_server_loop()`

Creates and runs the HTTP server.

It uses TCP sockets through LwIP:

```c
socket()
bind()
listen()
accept()
recv()
send()
close()
```

---

### `http_handle_client()`

Processes each received HTTP request.

It extracts the requested route and executes the corresponding action, such as turning an LED on or off.

---

## Simplified HTTP Server Flow

```text
http_server_loop()
   ↓
accept()
   ↓
http_handle_client()
   ↓
recv()
   ↓
parse HTTP route
   ↓
change LED state
   ↓
send HTTP response
   ↓
close connection
```

---

## SDK Dependencies

This project uses several GD32VW553 SDK components:

- Wi-Fi Management
- Wi-Fi Init
- LwIP sockets
- OSAL / RTOS wrapper
- GPIO HAL
- Platform initialization
- User settings
- Utility module

Main header files:

```c
#include "app_cfg.h"
#include "gd32vw55x_platform.h"
#include "gd32vw55x.h"
#include "lwip/sockets.h"
#include "lwip/priv/sockets_priv.h"
#include "wifi_management.h"
#include "wifi_init.h"
#include "util.h"
#include "user_setting.h"
```

---

## Limitations

This HTTP server is intentionally simple and intended for training and demonstration purposes.

It does not implement:

- HTTPS
- User authentication
- Multiple simultaneous clients
- Full HTTP parsing
- Advanced timeout handling
- Protection against complex malformed requests
- Production-grade security

For a final product, add proper security, robust error handling, connection management, and input validation.

---

## Hands-On Training Use

This example is suitable for a Wi-Fi hands-on session with the GD32VW553 because it provides an immediate and visible result.

During the training, the user can:

1. Flash the firmware.
2. Connect to the Wi-Fi network created by the board.
3. Open the web page on a phone.
4. Control physical LEDs.
5. Understand the relationship between Wi-Fi, HTTP, sockets, JavaScript, RTOS tasks, and GPIO.

---

## Possible Improvements

Future improvements may include:

- Add Station mode to connect the board to an existing router
- Add a REST-style API
- Add simple authentication
- Display board IP and MAC address on the page
- Add PWM brightness control
- Add physical button reading
- Add WebSocket support for real-time updates
- Split HTML, CSS, and JavaScript into separate modules
- Add a combined BLE + Wi-Fi example
- Add detailed UART logs

---

## License

This project is based on example code from the GD32VW55x SDK by GigaDevice.

Check the original license header in `main.c` and the official GigaDevice SDK documentation for licensing terms.
