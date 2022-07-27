Esphome component for gc9106 (fake st7735) displays

For displays that claim to be st7735 chips but are GalaxyCore GC9106 in reality
Code is a inspired by :<br>
  https://github.com/Boyeen/TFT_eSPI/blob/master/TFT_Drivers/GC9106_Init.h<br>
  https://github.com/Bodmer/TFT_eSPI/discussions/1310

To use this repository as an external component in Esphome:

<pre>
external_components:<br>
  - source: github://Jorre05/gc9106
    components: [ gc9106 ] 
</pre>
Esphome configuration example:

<pre>
display:
  - platform: gc9106
    id: my_display
    reset_pin: GPIO5
    cs_pin: GPIO10
    dc_pin: GPIO4
    rotation: 270
</pre>
