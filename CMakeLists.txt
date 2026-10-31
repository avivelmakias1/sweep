# SPDX-License-Identifier: MIT

if(CONFIG_SWEEP_USB_ONLY_DONGLE)
  zephyr_library_named(sweep_usb_only)
  zephyr_library_sources(src/usb_only_dongle.c)
  # These are called from behavior_bt.c, outside ble.c, so the linker wrap
  # runs after ble.c has started host advertising.
  zephyr_ld_options(
    -Wl,--wrap=zmk_ble_clear_bonds
    -Wl,--wrap=zmk_ble_clear_all_bonds
    -Wl,--wrap=zmk_ble_prof_select
    -Wl,--wrap=zmk_ble_prof_next
    -Wl,--wrap=zmk_ble_prof_prev
    -Wl,--wrap=zmk_ble_prof_disconnect
    -Wl,--wrap=zmk_ble_set_device_name
  )
endif()
