#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "xinput_device.h"

static uint8_t s_presentation = USB_PRESENTATION_COMPOSITE;

void usb_descriptors_set_presentation(uint8_t presentation)
{
	s_presentation = presentation;
}

uint8_t usb_descriptors_get_presentation(void)
{
	return s_presentation;
}

//--------------------------------------------------------------------+
// Device Descriptor - Core USB device properties
//--------------------------------------------------------------------+

static const tusb_desc_device_t desc_device_composite = {
		.bLength = sizeof(tusb_desc_device_t),
		.bDescriptorType = TUSB_DESC_DEVICE,
		.bcdUSB = USB_BCD,
		.bDeviceClass = 0x00,
		.bDeviceSubClass = 0x00,
		.bDeviceProtocol = 0x00,
		.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

		.idVendor = USB_VID,
		.idProduct = USB_PID,
		.bcdDevice = 0x0100,

		.iManufacturer = STRID_MANUFACTURER,
		.iProduct = STRID_PRODUCT,
		.iSerialNumber = STRID_SERIAL,

		.bNumConfigurations = 0x01};

// bcdDevice 0x0120 and the 045E:028E pair are what the wired-360 driver
// matches on; the strings are kept identical to a real pad for the same reason.
static const tusb_desc_device_t desc_device_xinput = {
		.bLength = sizeof(tusb_desc_device_t),
		.bDescriptorType = TUSB_DESC_DEVICE,
		.bcdUSB = USB_BCD,
		.bDeviceClass = 0x00,
		.bDeviceSubClass = 0x00,
		.bDeviceProtocol = 0x00,
		.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

		.idVendor = USB_VID_XINPUT,
		.idProduct = USB_PID_XINPUT,
		.bcdDevice = 0x0120,

		.iManufacturer = STRID_XINPUT_MANUFACTURER,
		.iProduct = STRID_XINPUT_PRODUCT,
		.iSerialNumber = STRID_SERIAL,

		.bNumConfigurations = 0x01};

uint8_t const *tud_descriptor_device_cb(void)
{
	// The identity has to match the presentation, otherwise the host's driver
	// match and the interface it is handed disagree.
	if (s_presentation == USB_PRESENTATION_XINPUT)
	{
		return (uint8_t const *)&desc_device_xinput;
	}
	return (uint8_t const *)&desc_device_composite;
}

//--------------------------------------------------------------------+
// HID Report Descriptors
//--------------------------------------------------------------------+

uint8_t const desc_hid_keyboard[] = {TUD_HID_REPORT_DESC_KEYBOARD()};
uint8_t const desc_hid_mouse[] = {TUD_HID_REPORT_DESC_MOUSE()};
uint8_t const desc_hid_gamepad[] = {TUD_HID_REPORT_DESC_GAMEPAD()};
uint8_t const desc_hid_rawhid[] = {
		TUD_HID_REPORT_DESC_GENERIC_INOUT(CFG_TUD_HID_EP_BUFSIZE)};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t itf)
{
	if (itf == 0)
	{
		return desc_hid_keyboard;
	}
	else if (itf == 1)
	{
		return desc_hid_mouse;
	}
	else if (itf == 2)
	{
		return desc_hid_gamepad;
	}
	else if (itf == 3)
	{
		return desc_hid_rawhid;
	}

	return NULL;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

#define CONFIG_TOTAL_LEN_COMPOSITE (TUD_CONFIG_DESC_LEN + 3 * TUD_HID_DESC_LEN + TUD_HID_INOUT_DESC_LEN)
/* The vendor interface length comes from the module that owns the layout. */
#define CONFIG_TOTAL_LEN_XINPUT (TUD_CONFIG_DESC_LEN + 40)

uint8_t const desc_configuration[] = {
		TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN_COMPOSITE, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
		TUD_HID_DESCRIPTOR(INTERFACE_KEYBOARD, STRID_HID_KEYBOARD, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_keyboard), EPNUM_KEYBOARD, CFG_TUD_HID_EP_BUFSIZE, USB_POLLING_INTERVAL),
		TUD_HID_DESCRIPTOR(INTERFACE_MOUSE, STRID_HID_MOUSE, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_mouse), EPNUM_MOUSE, CFG_TUD_HID_EP_BUFSIZE, USB_POLLING_INTERVAL),
		TUD_HID_DESCRIPTOR(INTERFACE_GAMEPAD, STRID_HID_GAMEPAD, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_gamepad), EPNUM_GAMEPAD, CFG_TUD_HID_EP_BUFSIZE, USB_POLLING_INTERVAL),
		TUD_HID_INOUT_DESCRIPTOR(INTERFACE_RAWHID, STRID_HID_RAWHID, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_rawhid), EPNUM_RAWHID, 0x80 | EPNUM_RAWHID, CFG_TUD_HID_EP_BUFSIZE, USB_POLLING_INTERVAL)};

/*
	The XInput configuration is a single vendor interface. A single-interface
	function follows the rules for "class defined at the interface level", so
	the device class stays 0x00 and the class signature lives on the interface.
*/
static uint8_t s_xinput_config[CONFIG_TOTAL_LEN_XINPUT];
_Static_assert(sizeof(s_xinput_config) == TUD_CONFIG_DESC_LEN + 40,
	       "XInput configuration buffer size must match the interface layout");

static void build_xinput_configuration(void)
{
	uint8_t *p = s_xinput_config;
	uint8_t const cfg[] = {
			TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN_XINPUT, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100)};
	memcpy(p, cfg, sizeof(cfg));
	p += sizeof(cfg);

	uint16_t n = xinput_build_interface_desc(p, 0, EPNUM_XINPUT_IN,
						 EPNUM_XINPUT_OUT,
						 STRID_HID_GAMEPAD);
	(void)n;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
	(void)index;
	if (s_presentation == USB_PRESENTATION_XINPUT)
	{
		// Built once, on the first request; the content never changes.
		if (s_xinput_config[0] == 0)
		{
			build_xinput_configuration();
		}
		return s_xinput_config;
	}
	return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

char const *string_desc_arr[] = {
		(const char[]){0x09, 0x04}, // 0: English (United States)
		"PHDesign",		     // 1: Manufacturer
		"PHDesign PHAC_V1",	     // 2: Product
		NULL,			     // 3: Serial Number (provided by the board)
		NULL,			     // 4: Keyboard Interface (unused)
		NULL,			     // 5: Mouse Interface (unused)
		"PHAC Controller",	     // 6: Gamepad Interface
		"PHDesign PHAC Interface",   // 7: RawHID Interface
		"Microsoft",		     // 8: XInput manufacturer
		"Controller",		     // 9: XInput product
};

static uint16_t _desc_str[32 + 1];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
	(void)langid;
	size_t chr_count;

	switch (index)
	{
	case STRID_LANGID:
		memcpy(&_desc_str[1], string_desc_arr[0], 2);
		chr_count = 1;
		break;

	case STRID_SERIAL:
		chr_count = board_usb_get_serial(_desc_str + 1, 32);
		break;

	default:

		if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
			return NULL;

		const char *str = string_desc_arr[index];

		chr_count = strlen(str);
		size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1;
		if (chr_count > max_count)
			chr_count = max_count;

		for (size_t i = 0; i < chr_count; i++)
		{
			_desc_str[1 + i] = str[i];
		}
		break;
	}

	_desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

	return _desc_str;
}
