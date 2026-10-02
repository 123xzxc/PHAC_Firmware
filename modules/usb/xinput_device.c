#include "tusb.h"

/*
	The endpoint and class-driver plumbing a custom personality needs lives in
	TinyUSB's private device header; tusb.h only declares the public class API.
*/
#include "device/usbd_pvt.h"

#include "xinput_device.h"

/*
	Vendor interface layout, byte for byte:

	  interface (9)                 class 0xFF / sub 0x5D / proto 0x01
	  class-specific blob (17)      the "0x21" descriptor the XInput driver reads
	  IN  endpoint (7)              interrupt, 32-byte packets, 1 ms
	  OUT endpoint (7)              interrupt, 32-byte packets, 8 ms
*/
#define XINPUT_IFACE_DESC_LEN (9 + 17 + 7 + 7)

#define XINPUT_REPORT_LEN 20

/* Buttons the host expects in the first 16-bit word of the report. */
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

static struct
{
	uint8_t ep_in;
	uint8_t ep_out;
	volatile bool in_use;
	uint8_t in_buf[XINPUT_REPORT_LEN];
	volatile bool tx_pending;
} s_xi;

uint16_t xinput_interface_desc_len(void)
{
	return XINPUT_IFACE_DESC_LEN;
}

uint16_t xinput_build_interface_desc(uint8_t *desc, uint8_t itfnum,
				     uint8_t ep_in, uint8_t ep_out, uint8_t strid)
{
	const uint8_t t[XINPUT_IFACE_DESC_LEN] = {
		9, TUSB_DESC_INTERFACE, itfnum, 0x00, 0x02, 0xFF, 0x5D, 0x01,
		strid,
		/* class-specific blob: the XInput driver validates these bytes */
		0x11, 0x21, 0x00, 0x01, 0x01, 0x25, ep_in, 0x14, 0x00, 0x00,
		0x00, 0x00, 0x13, ep_out, 0x08, 0x00, 0x00,
		7, TUSB_DESC_ENDPOINT, ep_in, TUSB_XFER_INTERRUPT,
		U16_TO_U8S_LE(32), 1,
		7, TUSB_DESC_ENDPOINT, ep_out, TUSB_XFER_INTERRUPT,
		U16_TO_U8S_LE(32), 8,
	};
	memcpy(desc, t, XINPUT_IFACE_DESC_LEN);
	return XINPUT_IFACE_DESC_LEN;
}

bool xinput_ready(void)
{
	return s_xi.in_use && s_xi.ep_in != 0;
}

void xinput_send(uint16_t buttons, uint8_t lt, uint8_t rt, int16_t lx,
		 int16_t ly, int16_t rx, int16_t ry)
{
	if (!tud_mounted() || !s_xi.in_use || s_xi.ep_in == 0)
		return;

	/*
		Only refill while the endpoint is idle: the controller keeps DMA'ing
		in_buf until the transfer completes, so overwriting it mid-flight would
		tear the report. Dropping updates while busy is what a real pad does
		too -- the newest state wins on the next frame.
	*/
	if (usbd_edpt_busy(0, s_xi.ep_in))
		return;

	uint8_t *r = s_xi.in_buf;
	r[0] = 0x00;
	r[1] = 0x14;
	r[2] = buttons & 0xFF;
	r[3] = buttons >> 8;
	r[4] = lt;
	r[5] = rt;
	r[6] = lx & 0xFF;
	r[7] = lx >> 8;
	r[8] = ly & 0xFF;
	r[9] = ly >> 8;
	r[10] = rx & 0xFF;
	r[11] = rx >> 8;
	r[12] = ry & 0xFF;
	r[13] = ry >> 8;
	memset(r + 14, 0, XINPUT_REPORT_LEN - 14);

	if (usbd_edpt_claim(0, s_xi.ep_in)) {
		if (usbd_edpt_xfer(0, s_xi.ep_in, s_xi.in_buf, XINPUT_REPORT_LEN))
			s_xi.tx_pending = false;
		else
			usbd_edpt_release(0, s_xi.ep_in);
	}
}

/* ---- TinyUSB class driver ------------------------------------------------ */

static void xi_init(void)
{
	memset(&s_xi, 0, sizeof(s_xi));
}

static void xi_reset(uint8_t rhport)
{
	(void)rhport;
	s_xi.in_use = false;
	s_xi.ep_in = 0;
	s_xi.ep_out = 0;
}

static uint16_t xi_open(uint8_t rhport, tusb_desc_interface_t const *itf,
			uint16_t max_len)
{
	/*
		Only claim the interfaces that carry the XInput signature; every other
		interface in the configuration is handled by the regular class drivers.
	*/
	if (itf->bInterfaceClass != 0xFF || itf->bInterfaceSubClass != 0x5D ||
	    itf->bInterfaceProtocol != 0x01)
		return 0;

	uint16_t const total = XINPUT_IFACE_DESC_LEN;
	if (max_len < total)
		return 0;

	uint8_t itfnum = itf->bInterfaceNumber;
	uint8_t const *p = (uint8_t const *)itf + sizeof(tusb_desc_interface_t);
	uint16_t remaining = total - sizeof(tusb_desc_interface_t);

	/*
		Walk the class-specific blob and the endpoints. The blob has to be
		skipped by its own bLength rather than assumed to be 17 bytes, because
		that is the only length the host actually declared.
	*/
	/*
		Descriptors carry their own length in the first byte, which is the only
		reliable way to walk them: the class-specific blob does not have a fixed
		size in the specification.
	*/
	while (remaining >= 2)
	{
		uint8_t len = p[0];
		uint8_t type = p[1];
		if (len < 2 || len > remaining)
			break;

		if (type == TUSB_DESC_ENDPOINT)
		{
			tusb_desc_endpoint_t const *ep = (tusb_desc_endpoint_t const *)d;
			uint8_t addr = ep->bEndpointAddress;
			if (ep->bmAttributes.xfer == TUSB_XFER_INTERRUPT)
				usbd_edpt_open(rhport, ep);
			if (addr & TUSB_DIR_IN_MASK)
				s_xi.ep_in = addr;
			else
				s_xi.ep_out = addr;
		}

		p += len;
		remaining -= (uint16_t)len;
	}

	(void)itfnum;
	s_xi.in_use = true;
	return total;
}

static bool xi_control_xfer_cb(uint8_t rhport, uint8_t stage,
			       tusb_control_request_t const *request)
{
	(void)rhport;
	(void)stage;
	/*
		The XInput driver issues no class requests it needs an answer to; the
		standard requests are handled by the device stack.
	*/
	if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD)
		return false;
	return true;
}

static bool xi_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result,
		       uint32_t xferred_bytes)
{
	(void)rhport;
	(void)ep_addr;
	(void)result;
	(void)xferred_bytes;
	/* An OUT transfer is the host's rumble packet; the pad ignores it. */
	return true;
}

static const usbd_class_driver_t s_xi_driver = {
	.name = "XInput",
	.init = xi_init,
	.deinit = NULL,
	.reset = xi_reset,
	.open = xi_open,
	.control_xfer_cb = xi_control_xfer_cb,
	.xfer_cb = xi_xfer_cb,
	.sof = NULL,
};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count)
{
	*driver_count = 1;
	return &s_xi_driver;
}
