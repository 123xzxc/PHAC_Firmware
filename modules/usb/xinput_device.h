#ifndef XINPUT_DEVICE_H_
#define XINPUT_DEVICE_H_

/*
	Xbox 360 wired (XInput) personality.

	A real Xbox 360 pad is not a HID device: it presents a vendor interface
	(class 0xFF / subclass 0x5D / protocol 0x01) carrying a fixed 20-byte input
	report, and the host binds it through the 045E:028E VID:PID pair. That
	personality is what makes the controller usable on hosts whose gamepad
	support is built around Microsoft's own driver stack rather than around
	generic HID gamepads.

	The interface descriptor is not static: its interface and endpoint numbers
	are allocated by TinyUSB when the descriptor is assembled, so it is built
	into a caller-provided buffer.
*/

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

	/* Bytes the vendor interface contributes to the configuration descriptor. */
	uint16_t xinput_interface_desc_len(void);

	/* Build the vendor interface at `desc`; returns the bytes written. */
	uint16_t xinput_build_interface_desc(uint8_t *desc, uint8_t itfnum,
					     uint8_t ep_in, uint8_t ep_out,
					     uint8_t strid);

	/* Latch the next input report. Called from the main loop. */
	void xinput_send(uint16_t buttons, uint8_t lt, uint8_t rt, int16_t lx,
			 int16_t ly, int16_t rx, int16_t ry);

	/* True once the host has opened the interface and its IN endpoint. */
	bool xinput_ready(void);

	/* Button bits of the 16-bit button word in the input report. */
#define XB_DPAD_UP 0x0001
#define XB_DPAD_DOWN 0x0002
#define XB_DPAD_LEFT 0x0004
#define XB_DPAD_RIGHT 0x0008
#define XB_START 0x0010
#define XB_BACK 0x0020
#define XB_L3 0x0040
#define XB_R3 0x0080
#define XB_LB 0x0100
#define XB_RB 0x0200
#define XB_A 0x1000
#define XB_B 0x2000
#define XB_X 0x4000
#define XB_Y 0x8000

#ifdef __cplusplus
}
#endif

#endif /* XINPUT_DEVICE_H_ */

