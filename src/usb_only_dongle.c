/*
 * SPDX-License-Identifier: MIT
 *
 * ZMK main always advertises a split central as a BLE HID keyboard
 * (update_advertising in app/src/ble.c). There is no Kconfig for turning
 * that off without also turning off the split link. The halves connect by
 * the dongle scanning, not by this advertising, so stopping it leaves the
 * split up and leaves USB HID as the only host link.
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

static void stop_host_adv(struct k_work *work) {
    ARG_UNUSED(work);
    (void)bt_le_adv_stop();
}

static K_WORK_DELAYABLE_DEFINE(stop_adv_work, stop_host_adv);

static void stop_host_adv_later(void) {
    /* ble.c sometimes queues the restart. Run after that item. */
    (void)k_work_schedule(&stop_adv_work, K_MSEC(50));
}

static bool peripheral_role(struct bt_conn *conn) {
    struct bt_conn_info info;

    bt_conn_get_info(conn, &info);
    return info.role == BT_CONN_ROLE_PERIPHERAL;
}

static void host_connected(struct bt_conn *conn, uint8_t err) {
    ARG_UNUSED(err);

    if (peripheral_role(conn)) {
        stop_host_adv_later();
    }
}

static void host_disconnected(struct bt_conn *conn, uint8_t reason) {
    ARG_UNUSED(reason);

    if (peripheral_role(conn)) {
        stop_host_adv_later();
    }
}

BT_CONN_CB_DEFINE(sweep_usb_only) = {
    .connected = host_connected,
    .disconnected = host_disconnected,
};

static void host_pairing_complete(struct bt_conn *conn, bool bonded) {
    ARG_UNUSED(bonded);

    if (peripheral_role(conn)) {
        stop_host_adv_later();
    }
}

static struct bt_conn_auth_info_cb host_auth_info = {
    .pairing_complete = host_pairing_complete,
};

static int sweep_usb_only_commit(void) {
    /* ble.c's settings commit (priority 0) has already started advertising. */
    (void)bt_le_adv_stop();
    bt_conn_auth_info_cb_register(&host_auth_info);
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE_WITH_CPRIO(sweep_usb_only, "sweep_usb_only", NULL, NULL,
                                          sweep_usb_only_commit, NULL, 99);

static void stop_after_real(void) { (void)bt_le_adv_stop(); }

void __wrap_zmk_ble_clear_bonds(void) {
    extern void __real_zmk_ble_clear_bonds(void);

    __real_zmk_ble_clear_bonds();
    stop_after_real();
}

void __wrap_zmk_ble_clear_all_bonds(void) {
    extern void __real_zmk_ble_clear_all_bonds(void);

    __real_zmk_ble_clear_all_bonds();
    stop_after_real();
}

int __wrap_zmk_ble_prof_select(uint8_t index) {
    extern int __real_zmk_ble_prof_select(uint8_t index);
    int err = __real_zmk_ble_prof_select(index);

    stop_after_real();
    return err;
}

int __wrap_zmk_ble_prof_next(void) {
    extern int __real_zmk_ble_prof_next(void);
    int err = __real_zmk_ble_prof_next();

    stop_after_real();
    return err;
}

int __wrap_zmk_ble_prof_prev(void) {
    extern int __real_zmk_ble_prof_prev(void);
    int err = __real_zmk_ble_prof_prev();

    stop_after_real();
    return err;
}

int __wrap_zmk_ble_prof_disconnect(uint8_t index) {
    extern int __real_zmk_ble_prof_disconnect(uint8_t index);
    int err = __real_zmk_ble_prof_disconnect(index);

    stop_after_real();
    return err;
}

int __wrap_zmk_ble_set_device_name(char *name) {
    extern int __real_zmk_ble_set_device_name(char *name);
    int err = __real_zmk_ble_set_device_name(name);

    stop_after_real();
    return err;
}
