## Pin Connections

### nRF24L01 Pin Connections

| **nRF24L01 Pin** | **Arduino Pin** | **Description**        |
|------------------|-----------------|------------------------|
| GND              | GND             | Ground                 |
| VCC              | 3.3V            | Power (3.3V)           |
| CE               | Pin 7           | Chip Enable (CE)       |
| CSN              | Pin 8           | Chip Select (CSN)      |
| SCK              | Pin 13          | SPI Clock (SCK)        |
| MOSI             | Pin 11          | SPI Master Out Slave In (MOSI) |
| MISO             | Pin 12          | SPI Master In Slave Out (MISO) |
| IRQ              | Not connected   | We don't need it|

### Soil Moisture Sensor Pin Connections

| **Soil Moisture Sensor Pin** | **Arduino Pin** | **Description**         |
|------------------------------|-----------------|-------------------------|
| VCC                          | Pin 2           | Power (controlled by digital pin 2) |
| GND                          | GND             | Ground                  |
| Analog Out                   | A0              | Analog Data Pin          |

### Temperature Sensor (LM35) Pin Connections

| **LM35 Pin**  | **Arduino Pin** | **Description**         |
|---------------|-----------------|-------------------------|
| VCC           | Pin 2           | Power (controlled by digital pin 2) |
| GND           | GND             | Ground                  |
| Output        | A1              | Analog Output (temperature data) |

### pH Sensor Pin Connections

| **pH Sensor Pin** | **Arduino Pin** | **Description**         |
|-------------------|-----------------|-------------------------|
| VCC               | Pin 2           | Power (controlled by digital pin 2) |
| GND               | GND             | Ground                  |
| Analog Out        | A2              | Analog Data Pin          |

### Humidity Sensor Pin Connections

| **Humidity Sensor Pin** | **Arduino Pin** | **Description**         |
|-------------------------|-----------------|-------------------------|
| VCC                     | Pin 2           | Power (controlled by digital pin 2) |
| GND                     | GND             | Ground                  |
| Analog Out              | A3              | Analog Data Pin          |
