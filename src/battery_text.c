/*
 * SPDX-License-Identifier: MIT
 *
 * Types both peripheral battery levels. A '+' after the slot number means
 * that half is plugged into USB power (the nice!nano charges in that case).
 *
 * ZMK's split battery report is only a percentage, and the Battery Service
 * rejects values above 100, so each half sends a follow-up marker the stock
 * report never uses: 128+level while USB power is present, or 255 when it
 * is not. Slot 0 is the first half that paired, not the left half.
 */

#define DT_DRV_COMPAT zmk_behavior_battery_text

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <drivers/behavior.h>

#include <dt-bindings/zmk/keys.h>
#include <zmk/behavior.h>
#include <zmk/behavior_queue.h>
#include <zmk/battery.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/split/central.h>
#include <zmk/split/bluetooth/service.h>
#else
#include <zmk/split/peripheral.h>
#endif

#if IS_ENABLED(CONFIG_SOC_SERIES_NRF52X)
#include <hal/nrf_power.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define SWEEP_CHARGE_FLAG 0x80
#define SWEEP_NOT_CHARGING 255

struct sweep_half {
    uint8_t level;
    bool charging;
    bool seen;
};

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

static struct sweep_half halves[2];
static struct zmk_behavior_binding_event pending_event;

static int on_peripheral_battery(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);

    if (ev == NULL || ev->source >= ARRAY_SIZE(halves)) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    struct sweep_half *half = &halves[ev->source];
    uint8_t soc = ev->state_of_charge;

    if (soc == SWEEP_NOT_CHARGING) {
        half->charging = false;
        half->seen = true;
    } else if (soc >= SWEEP_CHARGE_FLAG && soc <= (SWEEP_CHARGE_FLAG + 100)) {
        half->charging = true;
        half->level = soc - SWEEP_CHARGE_FLAG;
        half->seen = true;
    } else if (soc <= 100) {
        half->level = soc;
        half->seen = true;
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(sweep_battery_text, on_peripheral_battery);
ZMK_SUBSCRIPTION(sweep_battery_text, zmk_peripheral_battery_state_changed);

#else /* peripheral */

static bool half_usb_powered(void) {
#if IS_ENABLED(CONFIG_SOC_SERIES_NRF52X)
    return nrf_power_usbregstatus_vbusdet_get(NRF_POWER);
#else
    return false;
#endif
}

static void send_charge_report(void) {
    uint8_t level = zmk_battery_state_of_charge();

    if (level > 100) {
        level = 100;
    }

    struct zmk_split_transport_peripheral_event level_ev = {
        .type = ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_BATTERY_EVENT,
        .data.battery_event.level = level,
    };
    zmk_split_peripheral_report_event(&level_ev);

    struct zmk_split_transport_peripheral_event mark_ev = {
        .type = ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_BATTERY_EVENT,
        .data.battery_event.level = half_usb_powered() ? (SWEEP_CHARGE_FLAG | level) : SWEEP_NOT_CHARGING,
    };
    zmk_split_peripheral_report_event(&mark_ev);
}

static void charge_report_work(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(charge_report_dwork, charge_report_work);

static void charge_report_work(struct k_work *work) {
    ARG_UNUSED(work);
    send_charge_report();
    k_work_schedule(&charge_report_dwork, K_SECONDS(5));
}

static int charge_report_init(void) {
    k_work_schedule(&charge_report_dwork, K_SECONDS(1));
    return 0;
}

SYS_INIT(charge_report_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static int on_local_battery(const zmk_event_t *eh) {
    if (as_zmk_battery_state_changed(eh) != NULL) {
        send_charge_report();
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(sweep_battery_charge, on_local_battery);
ZMK_SUBSCRIPTION(sweep_battery_charge, zmk_battery_state_changed);

#endif /* CONFIG_ZMK_SPLIT_ROLE_CENTRAL */

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

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

static int type_levels(const struct zmk_behavior_binding_event *event) {
    int err = 0;

    for (uint8_t i = 0; i < 2; i++) {
        uint8_t level = halves[i].level;

        if (!halves[i].seen) {
            uint8_t fetched = 0;

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
            if (zmk_split_central_get_peripheral_battery_level(i, &fetched) == 0 && fetched <= 100) {
                level = fetched;
            }
#else
            ARG_UNUSED(fetched);
#endif
        }

        if (i > 0) {
            err = queue_key(event, SPACE);
            if (err) {
                return err;
            }
        }

        err = queue_key(event, i == 0 ? N0 : N1);
        if (err) {
            return err;
        }
        if (halves[i].charging) {
            err = queue_key(event, PLUS);
            if (err) {
                return err;
            }
        }
        err = queue_key(event, SPACE);
        if (err) {
            return err;
        }
        err = queue_level(event, level);
        if (err) {
            return err;
        }
        err = queue_key(event, PERCENT);
        if (err) {
            return err;
        }
    }

    return err;
}

static void type_later(struct k_work *work) {
    ARG_UNUSED(work);
    type_levels(&pending_event);
}

static K_WORK_DELAYABLE_DEFINE(type_dwork, type_later);

#endif /* CONFIG_ZMK_SPLIT_ROLE_CENTRAL */

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    struct zmk_behavior_binding refresh = {
        .behavior_dev = DEVICE_DT_NAME(DT_DRV_INST(0)),
    };

    BUILD_ASSERT(sizeof(DEVICE_DT_NAME(DT_DRV_INST(0))) <= ZMK_SPLIT_RUN_BEHAVIOR_DEV_LEN,
                 "battery behavior name must fit in the split behavior command");

    for (uint8_t i = 0; i < 2; i++) {
        zmk_split_central_invoke_behavior(i, &refresh, event, true);
    }

    pending_event = event;
    k_work_schedule(&type_dwork, K_MSEC(250));
    ARG_UNUSED(binding);
    return 0;
#else
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    send_charge_report();
    return 0;
#endif
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
