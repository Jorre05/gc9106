Esphome component for gc9106 (fake st7735) displays

For displays that claim to be st7735 chips but are GalaxyCore GC9106 in reality
Code is a inspired by :
  https://github.com/Boyeen/TFT_eSPI/blob/master/TFT_Drivers/GC9106_Init.h
  https://github.com/Bodmer/TFT_eSPI/discussions/1310

To use this repository as an external component in Esphome:

external_components:
  - source: github://Jorre05/esphome@dev
    components: [ gc9106 ] 

Esphome configuration example:

display:
  - platform: gc9106
    id: my_display
    reset_pin: GPIO5
    cs_pin: GPIO10
    dc_pin: GPIO4
    rotation: 270
