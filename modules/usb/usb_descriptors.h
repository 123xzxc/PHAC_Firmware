#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

//--------------------------------------------------------------------+
// Macro Constants
//--------------------------------------------------------------------+

#define USB_VID 0x5048
#define USB_BCD 0x0200

#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))
#define USB_PID (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
				 _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4))

#define EPNUM_KEYBOARD 0x81
#define EPNUM_MOUSE 0x82
#define EPNUM_GAMEPAD 0x83
#define EPNUM_RAWHID 0x04
#define USB_POLLING_INTERVAL 1 // Do not modify - knob filtering algorithm depends on this

//--------------------------------------------------------------------+
// Presentation Modes
//--------------------------------------------------------------------+

// The composite descriptor (Keyboard + Mouse + Gamepad + RawHID) is what the
// PC tooling expects, but iOS refuses a multi-interface device as a gamepad:
// IOHIDFamily only accepts a single, bare "Game Pad" HID interface. When the
// firmware runs in gamepad mode it therefore enumerates as this stripped-down
// one-interface device, which PC hosts accept as well.
enum
{
	USB_PRESENTATION_COMPOSITE,
	USB_PRESENTATION_GAMEPAD_ONLY,
};

// Selected before USB enumeration starts (i.e. before tud_task() runs) and
// never changed afterwards: TinyUSB cannot rebuild the configuration descriptor
// of an already mounted device.
void usb_descriptors_set_presentation(uint8_t presentation);
uint8_t usb_descriptors_get_presentation(void);

// Interface number of the gamepad/HID interface reporting in the active
// presentation: 2 in the composite descriptor, 0 in the gamepad-only one.
uint8_t usb_descriptors_gamepad_itf(void);

//--------------------------------------------------------------------+
// Enums
//--------------------------------------------------------------------+

enum
{
	INTERFACE_KEYBOARD,
	INTERFACE_MOUSE,
	INTERFACE_GAMEPAD,
	INTERFACE_RAWHID,
	ITF_NUM_TOTAL
};

enum
{
	STRID_LANGID = 0,
	STRID_MANUFACTURER,
	STRID_PRODUCT,
	STRID_SERIAL,
	STRID_HID_KEYBOARD,
	STRID_HID_MOUSE,
	STRID_HID_GAMEPAD,
	STRID_HID_RAWHID,
};

//--------------------------------------------------------------------+
// External Declarations
//--------------------------------------------------------------------+

#ifdef __cplusplus
extern "C"
{
#endif

	extern uint8_t const desc_hid_keyboard[];
	extern uint8_t const desc_hid_mouse[];
	extern uint8_t const desc_hid_gamepad[];
	extern uint8_t const desc_hid_rawhid[];

	extern const char *string_desc_arr[];

	uint8_t const *tud_descriptor_device_cb(void);
	uint8_t const *tud_hid_descriptor_report_cb(uint8_t itf);
	uint8_t const *tud_descriptor_configuration_cb(uint8_t index);
	uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid);

#ifdef __cplusplus
}
#endif

#endif /* USB_DESCRIPTORS_H_ */
