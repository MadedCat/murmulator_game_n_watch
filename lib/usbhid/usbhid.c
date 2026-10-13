#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "tusb.h"
#include "usbhid.h"

/* src/ps2.c: updates kb_st_ps2 from a PS/2 set 2 scancode */
extern void translate_scancode(uint8_t code, bool is_press, bool is_e0, bool is_e1);

#define E0 0x100 /* extended key: E0 prefix */

/* HID usage (keyboard page) -> PS/2 set 2 make code, as in ps2.c trans_table */
static const uint16_t hid_to_ps2[256] = {
    [HID_KEY_A] = 0x1C, [HID_KEY_B] = 0x32, [HID_KEY_C] = 0x21, [HID_KEY_D] = 0x23,
    [HID_KEY_E] = 0x24, [HID_KEY_F] = 0x2B, [HID_KEY_G] = 0x34, [HID_KEY_H] = 0x33,
    [HID_KEY_I] = 0x43, [HID_KEY_J] = 0x3B, [HID_KEY_K] = 0x42, [HID_KEY_L] = 0x4B,
    [HID_KEY_M] = 0x3A, [HID_KEY_N] = 0x31, [HID_KEY_O] = 0x44, [HID_KEY_P] = 0x4D,
    [HID_KEY_Q] = 0x15, [HID_KEY_R] = 0x2D, [HID_KEY_S] = 0x1B, [HID_KEY_T] = 0x2C,
    [HID_KEY_U] = 0x3C, [HID_KEY_V] = 0x2A, [HID_KEY_W] = 0x1D, [HID_KEY_X] = 0x22,
    [HID_KEY_Y] = 0x35, [HID_KEY_Z] = 0x1A,
    [HID_KEY_1] = 0x16, [HID_KEY_2] = 0x1E, [HID_KEY_3] = 0x26, [HID_KEY_4] = 0x25,
    [HID_KEY_5] = 0x2E, [HID_KEY_6] = 0x36, [HID_KEY_7] = 0x3D, [HID_KEY_8] = 0x3E,
    [HID_KEY_9] = 0x46, [HID_KEY_0] = 0x45,
    [HID_KEY_ENTER] = 0x5A, [HID_KEY_ESCAPE] = 0x76, [HID_KEY_BACKSPACE] = 0x66,
    [HID_KEY_TAB] = 0x0D, [HID_KEY_SPACE] = 0x29, [HID_KEY_MINUS] = 0x4E,
    [HID_KEY_EQUAL] = 0x55, [HID_KEY_BRACKET_LEFT] = 0x54, [HID_KEY_BRACKET_RIGHT] = 0x5B,
    [HID_KEY_BACKSLASH] = 0x5D, [HID_KEY_EUROPE_1] = 0x5D, [HID_KEY_SEMICOLON] = 0x4C,
    [HID_KEY_APOSTROPHE] = 0x52, [HID_KEY_GRAVE] = 0x0E, [HID_KEY_COMMA] = 0x41,
    [HID_KEY_PERIOD] = 0x49, [HID_KEY_SLASH] = 0x4A, [HID_KEY_CAPS_LOCK] = 0x58,
    [HID_KEY_F1] = 0x05, [HID_KEY_F2] = 0x06, [HID_KEY_F3] = 0x04, [HID_KEY_F4] = 0x0C,
    [HID_KEY_F5] = 0x03, [HID_KEY_F6] = 0x0B, [HID_KEY_F7] = 0x83, [HID_KEY_F8] = 0x0A,
    [HID_KEY_F9] = 0x01, [HID_KEY_F10] = 0x09, [HID_KEY_F11] = 0x78, [HID_KEY_F12] = 0x07,
    [HID_KEY_SCROLL_LOCK] = 0x7E, [HID_KEY_PRINT_SCREEN] = E0 | 0x7C,
    [HID_KEY_INSERT] = E0 | 0x70, [HID_KEY_HOME] = E0 | 0x6C, [HID_KEY_PAGE_UP] = E0 | 0x7D,
    [HID_KEY_DELETE] = E0 | 0x71, [HID_KEY_END] = E0 | 0x69, [HID_KEY_PAGE_DOWN] = E0 | 0x7A,
    [HID_KEY_ARROW_RIGHT] = E0 | 0x74, [HID_KEY_ARROW_LEFT] = E0 | 0x6B,
    [HID_KEY_ARROW_DOWN] = E0 | 0x72, [HID_KEY_ARROW_UP] = E0 | 0x75,
    [HID_KEY_NUM_LOCK] = 0x77, [HID_KEY_KEYPAD_DIVIDE] = E0 | 0x4A,
    [HID_KEY_KEYPAD_MULTIPLY] = 0x7C, [HID_KEY_KEYPAD_SUBTRACT] = 0x7B,
    [HID_KEY_KEYPAD_ADD] = 0x79, [HID_KEY_KEYPAD_ENTER] = E0 | 0x5A,
    [HID_KEY_KEYPAD_1] = 0x69, [HID_KEY_KEYPAD_2] = 0x72, [HID_KEY_KEYPAD_3] = 0x7A,
    [HID_KEY_KEYPAD_4] = 0x6B, [HID_KEY_KEYPAD_5] = 0x73, [HID_KEY_KEYPAD_6] = 0x74,
    [HID_KEY_KEYPAD_7] = 0x6C, [HID_KEY_KEYPAD_8] = 0x75, [HID_KEY_KEYPAD_9] = 0x7D,
    [HID_KEY_KEYPAD_0] = 0x70, [HID_KEY_KEYPAD_DECIMAL] = 0x71,
    [HID_KEY_APPLICATION] = E0 | 0x2F,
};

/* modifier bit (KEYBOARD_MODIFIER_*) -> PS/2 set 2 make code */
static const uint16_t mod_to_ps2[8] = {
    0x14,      /* left ctrl   */
    0x12,      /* left shift  */
    0x11,      /* left alt    */
    E0 | 0x1F, /* left gui    */
    E0 | 0x14, /* right ctrl  */
    0x59,      /* right shift */
    E0 | 0x11, /* right alt   */
    E0 | 0x27, /* right gui   */
};

static hid_keyboard_report_t prev_report;

static bool report_has_key(hid_keyboard_report_t const *r, uint8_t key) {
    for (int i = 0; i < 6; i++)
        if (r->keycode[i] == key)
            return true;
    return false;
}

static void send_ps2(uint16_t code, bool press) {
    if (code)
        translate_scancode((uint8_t)code, press, (code & E0) != 0, false);
}

static void send_key(uint8_t key, bool press) {
    if (key == HID_KEY_PAUSE)
        translate_scancode(0x14, press, false, true); /* E1 14 */
    else
        send_ps2(hid_to_ps2[key], press);
}

static void process_kbd_report(hid_keyboard_report_t const *r) {
    uint8_t changed = r->modifier ^ prev_report.modifier;
    for (int b = 0; b < 8; b++) {
        if (changed & (1u << b))
            send_ps2(mod_to_ps2[b], (r->modifier & (1u << b)) != 0);
    }
    /* releases first, then presses */
    for (int i = 0; i < 6; i++) {
        uint8_t k = prev_report.keycode[i];
        if (k && !report_has_key(r, k))
            send_key(k, false);
    }
    for (int i = 0; i < 6; i++) {
        uint8_t k = r->keycode[i];
        if (k > 3 /* 1..3: rollover/POST/undefined errors */ && !report_has_key(&prev_report, k))
            send_key(k, true);
    }
    prev_report = *r;
}

#define MAX_REPORT 4
static struct {
    uint8_t report_count;
    tuh_hid_report_info_t report_info[MAX_REPORT];
} hid_info[CFG_TUH_HID];

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
    if (instance < CFG_TUH_HID &&
        tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_NONE) {
        hid_info[instance].report_count =
            tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
    }
    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr; (void)instance;
    /* release everything that was held on the unplugged keyboard */
    hid_keyboard_report_t empty;
    memset(&empty, 0, sizeof(empty));
    process_kbd_report(&empty);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    uint8_t const protocol = tuh_hid_interface_protocol(dev_addr, instance);
    if (protocol == HID_ITF_PROTOCOL_KEYBOARD) {
        if (len >= sizeof(hid_keyboard_report_t))
            process_kbd_report((hid_keyboard_report_t const *)report);
    } else if (protocol == HID_ITF_PROTOCOL_NONE && instance < CFG_TUH_HID) {
        /* report-protocol keyboard: find the keyboard report by usage */
        uint8_t const rpt_count = hid_info[instance].report_count;
        tuh_hid_report_info_t *rpt_info_arr = hid_info[instance].report_info;
        tuh_hid_report_info_t *rpt_info = NULL;
        if (rpt_count == 1 && rpt_info_arr[0].report_id == 0) {
            rpt_info = &rpt_info_arr[0];
        } else if (len > 0) {
            uint8_t const rpt_id = report[0];
            for (uint8_t i = 0; i < rpt_count; i++) {
                if (rpt_id == rpt_info_arr[i].report_id) {
                    rpt_info = &rpt_info_arr[i];
                    break;
                }
            }
            report++;
            len--;
        }
        if (rpt_info && rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP &&
            rpt_info->usage == HID_USAGE_DESKTOP_KEYBOARD &&
            len >= sizeof(hid_keyboard_report_t)) {
            process_kbd_report((hid_keyboard_report_t const *)report);
        }
    }
    tuh_hid_receive_report(dev_addr, instance);
}

void usbhid_init(void) {
    memset(&prev_report, 0, sizeof(prev_report));
    tuh_init(BOARD_TUH_RHPORT);
}

void usbhid_task(void) {
    tuh_task();
}
