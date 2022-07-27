import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import spi
from esphome.components import display
from esphome.const import (
    CONF_DC_PIN,
    CONF_ID,
    CONF_LAMBDA,
    CONF_MODEL,
    CONF_RESET_PIN,
    CONF_PAGES,
)
from . import gc9106_ns

CODEOWNERS = ["@Jorre05"]

DEPENDENCIES = ["spi"]

CONF_DEVICE_WIDTH = "device_width"
CONF_DEVICE_HEIGHT = "device_height"
CONF_ROW_START = "row_start"
CONF_COL_START = "col_start"
CONF_EIGHT_BIT_COLOR = "eight_bit_color"
CONF_USE_BGR = "use_bgr"
CONF_INVERT_COLORS = "invert_colors"

SPIGC9106 = gc9106_ns.class_(
    "GC9106", cg.PollingComponent, display.DisplayBuffer, spi.SPIDevice
)
GC9106Model = gc9106_ns.enum("GC9106Model")

GC9106_SCHEMA = display.FULL_DISPLAY_SCHEMA.extend(
    {
        cv.Optional(CONF_RESET_PIN): pins.gpio_output_pin_schema,
    }
).extend(cv.polling_component_schema("1s"))

CONFIG_SCHEMA = cv.All(
    GC9106_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(SPIGC9106),
            cv.Required(CONF_DC_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_DEVICE_WIDTH, default=80): cv.int_,
            cv.Optional(CONF_DEVICE_HEIGHT, default=160): cv.int_,
            cv.Optional(CONF_COL_START, default=0): cv.int_,
            cv.Optional(CONF_ROW_START, default=0): cv.int_,
            cv.Optional(CONF_EIGHT_BIT_COLOR, default=False): cv.boolean,
            cv.Optional(CONF_USE_BGR, default=True): cv.boolean,
            cv.Optional(CONF_INVERT_COLORS, default=False): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema()),
    cv.has_at_most_one_key(CONF_PAGES, CONF_LAMBDA),
)


async def setup_gc9106(var, config):
    await cg.register_component(var, config)
    await display.register_display(var, config)

    if CONF_RESET_PIN in config:
        reset = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
        cg.add(var.set_reset_pin(reset))
    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayBufferRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))


async def to_code(config):
    var = cg.new_Pvariable(
        config[CONF_ID],
        config[CONF_DEVICE_WIDTH],
        config[CONF_DEVICE_HEIGHT],
        config[CONF_COL_START],
        config[CONF_ROW_START],
        config[CONF_EIGHT_BIT_COLOR],
        config[CONF_USE_BGR],
        config[CONF_INVERT_COLORS],
    )
    await setup_gc9106(var, config)
    await spi.register_spi_device(var, config)

    dc = await cg.gpio_pin_expression(config[CONF_DC_PIN])
    cg.add(var.set_dc_pin(dc))
