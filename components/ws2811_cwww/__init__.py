import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import CONF_ADDRESSABLE_LIGHT_ID, CONF_ID, CONF_NUM_LEDS
import esphome.final_validate as fv
from esphome.types import ConfigType

CODEOWNERS = []
DEPENDENCIES = ["light"]

CONF_BOUNDARIES = "boundaries"
CONF_ACTIVE_LEDS = "active_leds"
CONF_WS2811_CWWW_ID = "ws2811_cwww_id"
CONF_SEGMENT_INDEX = "segment_index"
CONF_BOUNDARY_INDEX = "boundary_index"
CONF_TYPE = "type"
TYPE_BOUNDARY = "boundary"
TYPE_ACTIVE_LEDS = "active_leds"
CONF_CENTER_POWER_TRANSITION = "center_power_transition"
CONF_CENTER_POWER_TRANSITION_OVERLAP = "center_power_transition_overlap"
CONF_POWER_ON_TRANSITION_LENGTH = "power_on_transition_length"
CONF_POWER_OFF_TRANSITION_LENGTH = "power_off_transition_length"
CONF_BOUNDARY_TRANSITION_LENGTH = "boundary_transition_length"
CONF_ACTIVE_LEDS_TRANSITION_LENGTH = "active_leds_transition_length"
CONF_MIN_SEGMENT_LENGTH = "min_segment_length"

ws2811_cwww_ns = cg.esphome_ns.namespace("ws2811_cwww")
WS2811CWWWHub = ws2811_cwww_ns.class_("WS2811CWWWHub", cg.Component)


def _get_parent_num_leds(config: ConfigType) -> int:
	fconf = fv.full_config.get()
	path = fconf.get_path_for_id(config[CONF_ADDRESSABLE_LIGHT_ID])[:-1]
	parent_config = fconf.get_config_for_path(path)
	return parent_config[CONF_NUM_LEDS]


def _validate_boundaries(config: ConfigType) -> ConfigType:
	min_segment_length = config[CONF_MIN_SEGMENT_LENGTH]
	previous = -1
	for index, boundary in enumerate(config[CONF_BOUNDARIES]):
		min_boundary = previous + min_segment_length
		if boundary < min_boundary:
			raise cv.Invalid(
				f"Boundary {index} ({boundary}) must leave at least {min_segment_length} LEDs for the previous segment",
				[CONF_BOUNDARIES, index],
			)
		previous = boundary
	return config


def _final_validate(config: ConfigType) -> ConfigType:
	num_leds = _get_parent_num_leds(config)
	if config[CONF_NUM_LEDS] != num_leds:
		raise cv.Invalid(
			f"Configured num_leds ({config[CONF_NUM_LEDS]}) must match the parent light ({num_leds})",
			[CONF_NUM_LEDS],
		)
	min_active_leds = (len(config[CONF_BOUNDARIES]) + 1) * config[CONF_MIN_SEGMENT_LENGTH]
	if config[CONF_ACTIVE_LEDS] < min_active_leds:
		raise cv.Invalid(
			f"active_leds ({config[CONF_ACTIVE_LEDS]}) must be at least {min_active_leds}",
			[CONF_ACTIVE_LEDS],
		)
	if config[CONF_ACTIVE_LEDS] > num_leds:
		raise cv.Invalid(
			f"active_leds ({config[CONF_ACTIVE_LEDS]}) must not exceed num_leds ({num_leds})",
			[CONF_ACTIVE_LEDS],
		)
	for index, boundary in enumerate(config[CONF_BOUNDARIES]):
		remaining_segments = len(config[CONF_BOUNDARIES]) - index
		max_boundary = config[CONF_ACTIVE_LEDS] - remaining_segments * config[CONF_MIN_SEGMENT_LENGTH] - 1
		if boundary > max_boundary:
			raise cv.Invalid(
				f"Boundary {index} ({boundary}) must be at most {max_boundary} to keep all following segments at least {config[CONF_MIN_SEGMENT_LENGTH]} LEDs long",
				[CONF_BOUNDARIES, index],
			)
	return config


CONFIG_SCHEMA = cv.All(
	cv.Schema(
		{
			cv.GenerateID(): cv.declare_id(WS2811CWWWHub),
			cv.Required(CONF_ADDRESSABLE_LIGHT_ID): cv.use_id(
				light.AddressableLightState
			),
			cv.Required(CONF_NUM_LEDS): cv.positive_int,
			cv.Optional(CONF_ACTIVE_LEDS): cv.positive_int,
			cv.Optional(
				CONF_BOUNDARY_TRANSITION_LENGTH, default="700ms"
			): cv.positive_time_period_milliseconds,
			cv.Optional(
				CONF_ACTIVE_LEDS_TRANSITION_LENGTH, default="900ms"
			): cv.positive_time_period_milliseconds,
			cv.Optional(CONF_MIN_SEGMENT_LENGTH, default=1): cv.positive_int,
			cv.Required(CONF_BOUNDARIES): cv.All(
				cv.ensure_list(cv.positive_int), cv.Length(min=1, max=4)
			),
		}
	).extend(cv.COMPONENT_SCHEMA),
	_validate_boundaries,
)

FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config: ConfigType) -> None:
	parent = await cg.get_variable(config[CONF_ADDRESSABLE_LIGHT_ID])
	var = cg.new_Pvariable(config[CONF_ID], parent)
	await cg.register_component(var, config)

	cg.add(var.set_num_leds(config[CONF_NUM_LEDS]))
	cg.add(var.set_active_leds(config[CONF_ACTIVE_LEDS]))
	cg.add(var.set_boundary_transition_length(config[CONF_BOUNDARY_TRANSITION_LENGTH]))
	cg.add(var.set_active_leds_transition_length(config[CONF_ACTIVE_LEDS_TRANSITION_LENGTH]))
	cg.add(var.set_min_segment_length(config[CONF_MIN_SEGMENT_LENGTH]))
	for boundary in config[CONF_BOUNDARIES]:
		cg.add(var.add_boundary(boundary))
