Elecricity spot price display using NodeMCU and e-paper
=======================================================

Or an “is this a good time to do laundry at 90°C” machine.

Displays current hourly electricity price as text and hourly prices for up to
48h as graph. Higher prices are shown with configurable gradient from black to
red.

Ingredients
-----------

- NodeMCU module
  - pretty much any ESP8266 module with 4MB flash or more should also work
- WeAct 2.9″ red-black-white e-paper display module
- Home Assistant and [Nord Pool integration][homeassistant-nordpool]

Preparation
-----------

1. connect e-paper module to ESP8266 module
   - pin assignments are in `epaper-electricity-price.yaml` under `spi` and
     `display`
   - connect power pins to NodeMCU's 3V and G pins
2. install [ESPHome][esphome]
3. create `secrets.yaml` and add values for all `!secret` references in the
   main yaml file
4. download the font Arial from wherever (it's copyrighted), and put
   `arial.ttf`, `arialn.ttf`, and `arialnb.ttf` in this directory
5. build and flash: `esphome run epaper-electricity-price.yaml`
6. add the Home Assistant automation in `homeassistant-automation.yaml`
   - it can be copy-pasted to Home Assistant's web interface
7. adjust settings in Home Assistant device configuration screen

If the display is sometimes garbled, install a 0.1 µF decoupling capacitor
between VCC and GND on the e-paper module.


[esphome]: https://esphome.io/
[homeassistant-nordpool]: https://github.com/custom-components/nordpool
