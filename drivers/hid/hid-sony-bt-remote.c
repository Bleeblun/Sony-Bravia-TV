/*
 *	HID driver for Sony BT remote
 *
 */

#include <linux/device.h>
#include <linux/hid.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>

#include "hid-ids.h"

#define map_key_clear(c)	hid_map_usage_clear(field->hidinput, usage, bit, max, EV_KEY, (c))

struct sony_btremote_sc {
	unsigned long quirks;
};

static const unsigned char hid_sony_bt_remote_keyboard[256] = {
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,148,149,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static int sony_bt_remote_input_mapping(struct hid_device *hdev, struct hid_input *hi,
	struct hid_field *field, struct hid_usage *usage,
	unsigned long **bit, int *max)
{
	if (hdev->vendor == USB_VENDOR_ID_SONY) {
		if (hdev->product == USB_DEVICE_ID_SONY_BT_REMOTE_002||
		    hdev->product == USB_DEVICE_ID_SONY_BT_REMOTE_003||
		    hdev->product == USB_DEVICE_ID_SONY_BT_REMOTE_004) {
			switch (usage->hid & HID_USAGE_PAGE) {
			case HID_UP_KEYBOARD:
				set_bit(EV_REP, hi->input->evbit);
				if ((usage->hid & HID_USAGE) < 256) {
					if(hid_sony_bt_remote_keyboard[usage->hid & HID_USAGE] != 0) {
						map_key_clear(hid_sony_bt_remote_keyboard[usage->hid & HID_USAGE]);
						return 1;
					}
				}
				break;
			case HID_UP_CONSUMER:
				switch (usage->hid & HID_USAGE) {
				case 0x004: map_key_clear(KEY_SONY_MICROPHONE);	return 1;
				case 0x020: map_key_clear(KEY_SONY_10KEY);		return 1;
				case 0x041: map_key_clear(KEY_SELECT);			return 1;
				case 0x061: map_key_clear(KEY_SUBTITLE);		return 1;
				case 0x067: map_key_clear(KEY_SONY_PICTURE);	return 1;
				case 0x069: map_key_clear(KEY_RED);				return 1;
				case 0x06a: map_key_clear(KEY_GREEN);			return 1;
				case 0x06b: map_key_clear(KEY_BLUE);			return 1;
				case 0x06c: map_key_clear(KEY_YELLOW);			return 1;
				case 0x0a5: map_key_clear(KEY_SONY_JUMP);		return 1;
				case 0x0a6: map_key_clear(KEY_SONY_TELETEXT);	return 1;
				case 0x0f6: map_key_clear(KEY_NEXT);			return 1;
				case 0x1d9: map_key_clear(KEY_SONY_DIGITAL);	return 1;
				case 0x1da: map_key_clear(KEY_SONY_ANALOG_CS);	return 1;
				case 0x1db: map_key_clear(KEY_SONY_BS);			return 1;
				case 0x1dc: map_key_clear(KEY_SONY_ANT);		return 1;
				case 0x1dd: map_key_clear(KEY_SONY_ANALOG);		return 1;
				case 0x1de: map_key_clear(KEY_SONY_VIDEO_SLCT);	return 1;
				case 0x1df: map_key_clear(KEY_SONY_TITLE_LIST);	return 1;
				case 0x1e0: map_key_clear(KEY_SONY_3D);			return 1;
				case 0x1f0: map_key_clear(KEY_SONY_SYNC_MENU);	return 1;
				case 0x1f1: map_key_clear(KEY_SONY_SPLIT);		return 1;
				case 0x1f2: map_key_clear(KEY_SONY_DATA);		return 1;
				case 0x1f3: map_key_clear(KEY_SONY_RADIO);		return 1;
				case 0x1f8: map_key_clear(KEY_SONY_MPX_MTS);	return 1;
				case 0x1f9: map_key_clear(KEY_SONY_NETFLIX);	return 1;
				case 0x1fa: map_key_clear(KEY_SONY_NEW);		return 1;
				case 0x1fd: map_key_clear(KEY_SONY_DISCOVER);	return 1;
				case 0x1fe: map_key_clear(KEY_SONY_ACT_MENU);	return 1;
				case 0x232: map_key_clear(KEY_SONY_WIDE);		return 1;
				default: /* use default key map */
					break;
				}
				break;
			default: /* ignore input data except keyboard keys */
				return -1;
			}
		}
	}
	return 0;
}

static const struct hid_device_id sony_bt_remote_devices[] = {
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_SONY, USB_DEVICE_ID_SONY_BT_REMOTE_002) },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_SONY, USB_DEVICE_ID_SONY_BT_REMOTE_003) },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_SONY, USB_DEVICE_ID_SONY_BT_REMOTE_004) },
	{ }
};
MODULE_DEVICE_TABLE(hid, sony_bt_remote_devices);

static struct hid_driver sony_bt_remote_driver = {
	.name = "sony_bt_remote",
	.id_table = sony_bt_remote_devices,
	.input_mapping = sony_bt_remote_input_mapping,
};

static int __init sony_bt_remote_init(void)
{
	return hid_register_driver(&sony_bt_remote_driver);
}

static void __exit sony_bt_remote_exit(void)
{
	hid_unregister_driver(&sony_bt_remote_driver);
}

module_init(sony_bt_remote_init);
module_exit(sony_bt_remote_exit);


