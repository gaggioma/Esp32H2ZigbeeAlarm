# Fracarro Alarm + ESP32H2 + Zigbee2MQTT + HA
<picture>
  <img src="/images/fracarro.png" alt="fracarro" style="width:150px;">
  <img src="/images/esp32h2.jpg" alt="esp32h2" style="width:150px;">
  <img src="/images/z2mqtt.png" alt="z2mqtt" style="width:150px;">
  <img src="/images/ha.png" alt="hs" style="width:150px;">
</picture>

## Introduction
The purpose of this project is to connect my [Fracarro Alarm](https://fracarro.com/it/product/910377-defender-24/) to my Zigbee network and by [Zigbe2MQTT](https://www.zigbee2mqtt.io/) integrate the alarm's states change into my [HA](https://www.home-assistant.io/integrations/mqtt/) ecosystem.

To connect alarm to Zigbee network i choose the [ESP32H2 supemini board](https://www.espboards.dev/esp32/esp32-h2-super-mini/), this SoC allow (by mean of GPIO) to read external state and trasfer them into a Zigbee network.
However i choose this board because is designed for very low consumption, ideal for battery power supply.

## Overall connection scheme

<picture>
  <img src="/images/connection_scheme.png" alt="connection_scheme" style="width:110%;">
</picture>

In picture above there are all connections between ESP32H2, alarm and power supply block.

## Battery voltage measurement scheme
<picture>
  <img src="/images/power_scheme.PNG" alt="power_scheme" style="width:auto;">
</picture>

The circuit measure the voltage of battery (used to know the consumpion) by means of voltage divider.

This divider is reccomended because the battery voltage reach 4.2V when fully charged (too high for max of 3.3V for GPIO), so with a divider factor of 0.78 it reduce the max voltage to approximatively 3.3V.

The advantage of this circuit is that the voltage divider drain current only when the GPIO10 enable the N-Mosfet gate. This increment the efficiency of overall circuit.

## Esp32H2 connection description and documentations

Following a brief description of GPIO that has bee used.

* GPIO01 (ADC): connected to voltage divider to read the battery voltage and get the remained capacity;
* GPIO04 (input with pull up enabled): when high (high impedence) the chip not enter in deep sleep but stay awake to listen Zigbee network messages;
* GPIO10: connected to power module, this GPIO controls the Gate of N-Mosfet transitor (shown in schema below) to enable or disable the voltage divider. In this way the voltage divider will be switched off (no power drain) when not used.
* GPIO011, GPIO12, GPIO14 (input with pull up enabled): these inputs read value from alarm;
* 3.3V: used to battery power supply. The voltage must be regulated in [3.0, 3.6]V so i've used a [TZT voltage regulator](https://it.aliexpress.com/item/1005004680099594.html?spm=a2g0o.order_list.order_list_main.22.9ff936963w80yh&gatewayAdapt=glo2ita);
* GND: common ground. All grounds must be in common.

ESP32H2 supermini board uses ESP32H2 chip so the [datasheet will be here](https://documentation.espressif.com/esp32-h2_datasheet_en.pdf).
An overview on [GPIO here](https://docs.espressif.com/projects/esp-idf/en/stable/esp32h2/api-reference/peripherals/gpio.html).

An important thing:
* avoid to use GPIO relative to strapping pins, this can create conflicts with important pin functions;
* GPIO pins designed for Deep Sleep wake up, must be RTC pins.

## ESP32H2 software

All the software required to make ESP32H2 works has been written using [Espressif Arduino library](https://docs.espressif.com/projects/arduino-esp32/en/latest/getting_started.html). Arduino library is really well written and contains a lot of examples for every purpose.

Basically the software carry out these functionalities:

* configure GPIO with pull-up to read alarm state. These GPIOs wakes up the ESP32 when alarm state change;
* once the ESP32 is awake the voltage divider will be enabled and the ADC will read the value to avaluate battery consumption (to have as much as possible a precise value a mean over 100 samples has been done);
* transmit all states over Zigbee network;
* reduce power consumption with Deep Sleep;
* if no alarm states change until 24 hours, a battery measurement is trasmitted. 

Documentation about Zigbee APIs [here](https://docs.espressif.com/projects/arduino-esp32/en/latest/libraries.html#zigbee-apis) i've found all documentation about Zigbee APIs.

Arduino ESP32 generic libraries [here](https://docs.espressif.com/projects/arduino-esp32/en/latest/libraries.html#apis).

My implemetation is [here](/alarm_esp32h2.ino)

## Fracarro Defender 24 alarm

Fracarro Defender alarm provides 5 customizable Open Collector outputs with maximum operation range of 50mA 50V [here for details](https://fracarro.com/wp-content/uploads/product-downloadable-contents/manuals/910377_DEFENDER_24_INST_MI_IT.pdf).

This means that every output will be:
 - signal low (disable) -> High Impedence --pull-up--> High;
 - signal high (enable) -> GND --pull-up--> Low.

For my purpose i've configured 3 out of 5 custom output:
1) enable/disable alarm;
2) sensor 1
3) sensor 2.

## Realization photo

<picture>
  <img src="/images/overall_photo.jpeg" alt="overall_photo" style="width:80%;">
</picture>

before connecting to the alarm and put all into the case, this is a photo of my relization with a label of the main blocks.

You can see:
* power supply is provided from a [Li-Io 18650 recharged battery](https://it.aliexpress.com/item/1005008772657403.html?spm=a2g0o.order_list.order_list_main.63.70f23696T7hVuV&gatewayAdapt=glo2ita);
* voltage stabilizer as dercribed [here](#esp32h2-connection-description-and-documentations);
* voltage divider to measure consumption [here](#battery-voltage-measurement-scheme);
* block to enable voltage divider only when required [here](#battery-voltage-measurement-scheme);
* ESP32H2;
* alarm output [here](#overall-connection-scheme)

## Assembled photo

<picture>
  <img src="/images/photo_with_alarm.jpeg" alt="overall_photo" style="width:60%;">
</picture>

## Conclusions

1) With a fully charged battery, i noticed that ESP32H2 works for about two months.
2) In deep-sleep the overall circuit drain about 1mA.

## Further improvement

1) The only thing i'm watching is how to improve the duration with a single battey charge. 
