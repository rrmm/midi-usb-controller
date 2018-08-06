
#include "conf_usb.h"
#include "usb_protocol.h"
#include "midi/usb_protocol_midi.h"
#include "usb.h"
#include "udd.h"
#include "udc.h"
#include "midi/device/udi_midi.h"
#include <string.h>


// need this for the busy bit

/** Structure definition about job registered on an endpoint */
typedef struct {
	union {
		//! Callback to call at the end of transfer
		udd_callback_trans_t call_trans;
		//! Callback to call when the endpoint halt is cleared
		udd_callback_halt_cleared_t call_nohalt;
	};
	//! Buffer located in internal RAM to send or fill during job
	uint8_t *buf;
	//! Size of buffer to send or fill
	iram_size_t buf_size;
	//! Total number of data transferred on endpoint
	iram_size_t nb_trans;
	//! Endpoint size
	uint16_t ep_size;
	//! A job is registered on this endpoint
	uint8_t busy:1;
	//! A short packet is requested for this job on endpoint IN
	uint8_t b_shortpacket:1;
	//! The cache buffer is currently used on endpoint OUT
	uint8_t b_use_out_cache_buffer:1;
} udd_ep_job_t;


bool udi_midi_enable(void);
void udi_midi_disable(void);
bool udi_midi_setup(void);
uint8_t udi_midi_getsetting(void);
void udi_midi_sof_notify(void);

UDC_DESC_STORAGE udi_api_t udi_api_midi = {
	.enable = udi_midi_enable,
	.disable = udi_midi_disable,
	.setup = udi_midi_setup,
	.getsetting = udi_midi_getsetting,
	.sof_notify = udi_midi_sof_notify,
};


// udd doesn't expose this function
extern udd_ep_job_t* udd_ep_get_job(udd_ep_id_t ep);


bool dequeue_ctrl(uint8_t * n, uint16_t * value);
uint8_t move_queue_to_buffer(void);
void ep1_transmit_callback (udd_ep_status_t status, iram_size_t nb_transfered, udd_ep_id_t ep);


UDC_DESC_STORAGE uint8_t out_buffer[64];

typedef struct {
	uint8_t n;
	uint16_t value; 
} ctrlq_entry_t;

typedef struct {
	uint8_t size;
	uint8_t read_idx;
	uint8_t write_idx;
	ctrlq_entry_t q[128];  
} ctrlq_t;

ctrlq_t ctrlq = { 128, 0, 0, {{0}} };



bool enqueue_ctrl(uint8_t n, uint16_t value) {
	uint8_t idx = (ctrlq.write_idx+1)%ctrlq.size;
	if (idx == ctrlq.read_idx) {
		return false;
	}
	ctrlq.q[idx].n = n;
	ctrlq.q[idx].value = value;
	ctrlq.write_idx = idx;
	return true;
}


bool dequeue_ctrl(uint8_t * n, uint16_t * value) {
	if (ctrlq.write_idx == ctrlq.read_idx) {
		return false;
	}
	uint8_t pos = (ctrlq.read_idx+1)%ctrlq.size;
	*n = ctrlq.q[pos].n;
	*value = ctrlq.q[pos].value;
	ctrlq.read_idx = pos;
	return true;
}

// this must only happen when the buffer is not in use
// returns number of bytes
uint8_t move_queue_to_buffer(void) {
	uint8_t n; 
	uint16_t value;
	uint8_t count = 0; 
	while (dequeue_ctrl(&n, &value)) {

		if ((n&0xf0) == CTRL_PITCHBEND) {
			out_buffer[count+0] = 0x0e;
			out_buffer[count+1] = 0xe0;
			out_buffer[count+2] = (uint8_t)((value)&0x7f);
			out_buffer[count+3] = (uint8_t)((value>>7)&0x7f);
			count+=4;		 
		} else { 
			// default 0-127 controller
			out_buffer[count+0] = 0x0b; 
			out_buffer[count+1] = 0xb0;
			out_buffer[count+2] = n+11;
			out_buffer[count+3] = (uint8_t)value;
			count+=4;
		}
		if (count >= (sizeof(out_buffer))) {
			break;
		}
	}
	return count;
}



void ep1_transmit_callback (udd_ep_status_t status,
							iram_size_t nb_transfered, udd_ep_id_t ep) {

}

 
// use the sof notification to check if there is something in the queue and if so start a 
// transfer
void udi_midi_sof_notify(void) {
	udd_ep_job_t * ptr_job = udd_ep_get_job(0x82);
	if (ptr_job != NULL && !ptr_job->busy) {
		uint8_t n_bytes = move_queue_to_buffer();
		if (n_bytes > 0) {
			udd_ep_run(0x82, !(n_bytes==64) , out_buffer, n_bytes, &ep1_transmit_callback);
		}

	}
}


volatile bool DEVICE_ENUMERATED_RUNNING = false;


	/*
	 * This function is called when the host selects a configuration
	 * to which this interface belongs through a Set Configuration
	 * request, and when the host selects an alternate setting of
	 * this interface through a Set Interface request.
	 *
	 * \return \c 1 if function was successfully done, otherwise \c 0.
	 */

bool udi_midi_enable(void)
{
	// setup two eps for midi
	
	if (udd_ep_alloc(0x82, USB_DEVICE_ENDPOINT_TYPE_BULK, 64) &&
		udd_ep_alloc(0x01, USB_DEVICE_ENDPOINT_TYPE_BULK, 64)) {
		// couldn't allocate our end points
		return false;
	}
	DEVICE_ENUMERATED_RUNNING = true;
	return true;
}


/**
	 * \brief Disable the interface.
	 *
	 * This function is called when this interface is currently
	 * active, and
	 * - the host selects any configuration through a Set
	 *   Configuration request, or
	 * - the host issues a USB reset, or
	 * - the device is detached from the host (i.e. Vbus is no
	 *   longer present)
	 */
void udi_midi_disable(void)
{
	DEVICE_ENUMERATED_RUNNING = false;
	udd_ep_free(0x82);
	udd_ep_free(0x01);
}


	/**
	 * \brief Handle a control request directed at an interface.
	 *
	 * This function is called when this interface is currently
	 * active and the host sends a SETUP request
	 * with this interface as the recipient.
	 *
	 * Use udd_g_ctrlreq to decode and response to SETUP request.
	 *
	 * \return \c 1 if this interface supports the SETUP request, otherwise \c 0.
	 */
bool udi_midi_setup(void)
{
	//uint8_t port = udi_cdc_setup_to_port();

	if (Udd_setup_is_in()) {
		// GET Interface Requests
		if (Udd_setup_type() == USB_REQ_TYPE_CLASS) {
			// Requests Class Interface Get
			/*
			switch (udd_g_ctrlreq.req.bRequest) {
			case USB_REQ_CDC_GET_LINE_CODING:
				// Get configuration of CDC line
				if (sizeof(usb_cdc_line_coding_t) !=
						udd_g_ctrlreq.req.wLength)
					return false; // Error for USB host
				udd_g_ctrlreq.payload =
						(uint8_t *) &
						udi_cdc_line_coding[port];
				udd_g_ctrlreq.payload_size =
						sizeof(usb_cdc_line_coding_t);
				return true;
			}*/
		}
	}
	if (Udd_setup_is_out()) {
		// SET Interface Requests
		if (Udd_setup_type() == USB_REQ_TYPE_CLASS) {
			// Requests Class Interface Set
			/*
			switch (udd_g_ctrlreq.req.bRequest) {
			case USB_REQ_CDC_SET_LINE_CODING:
				// Change configuration of CDC line
				if (sizeof(usb_cdc_line_coding_t) !=
						udd_g_ctrlreq.req.wLength)
					return false; // Error for USB host
				udd_g_ctrlreq.callback =
						udi_cdc_line_coding_received;
				udd_g_ctrlreq.payload =
						(uint8_t *) &
						udi_cdc_line_coding[port];
				udd_g_ctrlreq.payload_size =
						sizeof(usb_cdc_line_coding_t);
				return true;
			case USB_REQ_CDC_SET_CONTROL_LINE_STATE:
				// According cdc spec 1.1 chapter 6.2.14
				UDI_CDC_SET_DTR_EXT(port, (0 !=
						(udd_g_ctrlreq.req.wValue
						 & CDC_CTRL_SIGNAL_DTE_PRESENT)));
				UDI_CDC_SET_RTS_EXT(port, (0 !=
						(udd_g_ctrlreq.req.wValue
						 & CDC_CTRL_SIGNAL_ACTIVATE_CARRIER)));
				return true;
			}*/
		}
	}
	return false;  // request Not supported
}


	/**
	 * \brief Returns the current setting of the selected interface.
	 *
	 * This function is called when UDC when know alternate setting of selected interface.
	 *
	 * \return alternate setting of selected interface
	 */
uint8_t udi_midi_getsetting(void)
{
	return 0;      // only one alternate setting
}

