#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize NimBLE and start advertising as a BLE HID Mouse
 */
void bluetooth_init(void);

#ifdef __cplusplus
}
#endif

#endif // BLUETOOTH_H