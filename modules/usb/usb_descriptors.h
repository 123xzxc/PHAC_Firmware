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

// XInput identity. A host that only knows Microsoft's driver stack -- iOS and
// Windows among them -- binds a wired 360 pad by this pair, not by a HID usage,
// so gamepad mode has to present exactly it or the device is ignored.
#define USB_VID_XINPUT 0x045E
#define USB_PID_XINPUT 0x028E

#define EPNUM_KEYBOARD 0x81
#define EPNUM_MOUSE 0x82
#define EPNUM_GAMEPAD 0x83
#define EPNUM_XINPUT_IN 0x81
#define EPNUM_XINPUT_OUT 0x01
#define EPNUM_RAWHID 0x04
#define USB_POLLING_INTERVAL 1 // Do not modify - knob filtering algorithm depends on this

//--------------------------------------------------------------------+
// Presentation Modes
//--------------------------------------------------------------------+

// A generic HID gamepad is invisible to hosts that only implement Microsoft's
// controller stack, so gamepad mode presents the XInput vendor interface
// instead. The composite personality is what the PC configuration tool talks
// to and is kept for keyboard/mouse mode.
enum
{
	USB_PRESENTATION_COMPOSITE,
	USB_PRESENTATION_XINPUT,
};

// Selected before enumeration starts and never changed afterwards: TinyUSB
// builds the configuration descriptor once, when the host asks for it.
void usb_descriptors_set_presentation(uint8_t presentation);
uint8_t usb_descriptors_get_presentation(void);

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

	// Gamepad mode strings come after the composite ones so the composite
	// descriptor keeps its existing indices.
	STRID_XINPUT_MANUFACTURER,
	STRID_XINPUT_PRODUCT,
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
