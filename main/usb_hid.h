#ifndef USB_HID_H
#define USB_HID_H

void usb_init();
bool usb_mounted();
bool usb_mouse_report(uint8_t click, int8_t delta_x, int8_t delta_y);

#endif

