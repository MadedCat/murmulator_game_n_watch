#pragma once

/* USB HID keyboard (TinyUSB host on the native USB port). Key events are
   translated to the PS/2 set 2 scancodes the PS/2 driver decodes and passed
   to translate_scancode(), so both keyboards update the same kb_st_ps2. */
void usbhid_init(void);
void usbhid_task(void);
