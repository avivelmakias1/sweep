/*
 * SPDX-License-Identifier: MIT
 *
 * Types the two peripheral battery levels at the cursor, as keystrokes.
 * The computer sees the same HID codes as the number keys. Slot 0 is the
 * first half that paired to the dongle, slot 1 is the second.
 */

#define DT_DRV_COMPAT zmk_behavior_battery_text

#include <zephyr/device.h>
#include <drivers/behavior.h>

#include <dt-bindings/zmk/keys.h>
#include <zmk/behavior.h>
#include <zmk/behavior_queue.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
#include <zmk/split/central.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static const uint32_t digit_usage[] = {N0, N1, N2, N3, N4, N5, N6, N7, N8, N9};

static int queue_key(const struct zmk_behavior_binding_event *event, uint32_t usage) {
    const struct zmk_behavior_binding binding = {
        .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(kp)),
        .param1 = usage,
    };
    int err;

    err = zmk_behavior_queue_add(event, binding, true, CONFIG_ZMK_MACRO_DEFAULT_TAP_MS);
    if (err) {
        return err;
    }

    return zmk_behavior_queue_add(event, binding, false, CONFIG_ZMK_MACRO_DEFAULT_WAIT_MS);
}

static int queue_level(const struct zmk_behavior_binding_event *event, uint8_t level) {
    int err;

    if (level > 100) {
        level = 100;
    }

    if (level >= 100) {
        err = queue_key(event, N1);
        if (err) {
            return err;
        }
        err = queue_key(event, N0);
        if (err) {
            return err;
        }
        return queue_key(event, N0);
    }

    if (level >= 10) {
        err = queue_key(event, digit_usage[level / 10]);
        if (err) {
            return err;
        }
    }

    return queue_key(event, digit_usage[level % 10]);
}

static int type_levels(const struct zmk_behavior_binding_event *event) {
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
    uint8_t levels[2] = {0, 0};
    int err;

    for (uint8_t i = 0; i < 2; i++) {
        if (zmk_split_central_get_peripheral_battery_level(i, &levels[i]) < 0) {
            levels[i] = 0;
        }
    }

    err = queue_key(event, N0);
    if (err) {
        return err;
    }
    err = queue_key(event, SPACE);
    if (err) {
        return err;
    }
    err = queue_level(event, levels[0]);
    if (err) {
        return err;
    }
    err = queue_key(event, PERCENT);
    if (err) {
        return err;
    }
    err = queue_key(event, SPACE);
    if (err) {
        return err;
    }
    err = queue_key(event, N1);
    if (err) {
        return err;
    }
    err = queue_key(event, SPACE);
    if (err) {
        return err;
    }
    err = queue_level(event, levels[1]);
    if (err) {
        return err;
    }
    return queue_key(event, PERCENT);
#else
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
#endif
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    return type_levels(&event);
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_battery_text_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_battery_text_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
