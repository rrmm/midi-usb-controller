/**
 * \file
 *
 * \brief Default descriptors for a USB Device with a single interface CDC
 *
 * Copyright (c) 2009-2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#include "conf_usb.h"
#include "udd.h"
#include "udc_desc.h"
#include "udi_midi.h"


/**
 * \defgroup udi_cdc_group_single_desc USB device descriptors for a single interface
 *
 * The following structures provide the USB device descriptors required for
 * USB Device with a single interface CDC.
 *
 * It is ready to use and do not require more definition.
 *
 * @{
 */


# define USB_VERSION   USB_V2_0

//! USB Device Descriptor
COMPILER_WORD_ALIGNED
UDC_DESC_STORAGE usb_dev_desc_t udc_device_desc = {
	.bLength                   = sizeof(usb_dev_desc_t),
	.bDescriptorType           = USB_DT_DEVICE,
	.bcdUSB                    = LE16(USB_VERSION),
	.bDeviceClass              = 0,  // only supported at interface level
	.bDeviceSubClass           = 0,
	.bDeviceProtocol           = 0,
	.bMaxPacketSize0           = USB_DEVICE_EP_CTRL_SIZE,
	.idVendor                  = LE16(USB_DEVICE_VENDOR_ID),
	.idProduct                 = LE16(USB_DEVICE_PRODUCT_ID),
	.bcdDevice                 = LE16((USB_DEVICE_MAJOR_VERSION << 8)
                                      | USB_DEVICE_MINOR_VERSION),
#ifdef USB_DEVICE_MANUFACTURE_NAME
	.iManufacturer = 1,
#else
	.iManufacturer             = 0,  // No manufacture string
#endif
#ifdef USB_DEVICE_PRODUCT_NAME
	.iProduct = 2,
#else
	.iProduct                  = 0,  // No product string
#endif
#if (defined USB_DEVICE_SERIAL_NAME || defined USB_DEVICE_GET_SERIAL_NAME_POINTER) 
	.iSerialNumber = 3,
#else
	.iSerialNumber             = 0,  // No serial string
#endif
	.bNumConfigurations = 1
};



#ifdef USB_DEVICE_LPM_SUPPORT
//! USB Device Qualifier Descriptor
COMPILER_WORD_ALIGNED
UDC_DESC_STORAGE usb_dev_lpm_desc_t udc_device_lpm = {
	.bos.bLength               = sizeof(usb_dev_bos_desc_t),
	.bos.bDescriptorType       = USB_DT_BOS,
	.bos.wTotalLength          = LE16(sizeof(usb_dev_bos_desc_t) + sizeof(usb_dev_capa_ext_desc_t)),
	.bos.bNumDeviceCaps        = 1,
	.capa_ext.bLength          = sizeof(usb_dev_capa_ext_desc_t),
	.capa_ext.bDescriptorType  = USB_DT_DEVICE_CAPABILITY,
	.capa_ext.bDevCapabilityType = USB_DC_USB20_EXTENSION,
	.capa_ext.bmAttributes     = USB_DC_EXT_LPM,
};
#endif


#define USB_CLASS_AUDIO               0x01
#define USB_SUBCLASS_MIDI_STREAMING   0x03


#define IFACE_SUBTYPE_MS_HEADER       0x01
#define IFACE_SUBTYPE_MIDI_IN_JACK    0x02
#define IFACE_SUBTYPE_MIDI_OUT_JACK   0x03
#define IFACE_SUBTYPE_ELEMENT         0x04


#define EP_SUBTYPE_MS_GENERAL         0x01

#define MS_MIDI_JACK_TYPE_EMBEDDED    0x01
#define MS_MIDI_JACK_TYPE_EXTERNAL         0x02


#define  CS_INTERFACE     0x24	//!< Interface Functional Descriptor
#define  CS_ENDPOINT      0x25	//!< Endpoint Functional Descriptor


COMPILER_PACK_SET(1)
typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  le16_t  bcdMSC;
  le16_t  wTotalLength;
} ms_iface_desc_t;
COMPILER_PACK_RESET()


COMPILER_PACK_SET(1)
typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint8_t bJackType;
  uint8_t bJackID;
  uint8_t iJack; 
} ms_iface_in_jack_t;
COMPILER_PACK_RESET()

COMPILER_PACK_SET(1)
typedef struct {
  uint8_t baSourceID;
  uint8_t baSourcePin;
} ms_midi_out_jack_source_t;
COMPILER_PACK_RESET()

// defines an output port descriptor with p sources
#define MS_IFACE_OUT(p) COMPILER_PACK_SET(1)    \
  typedef struct {                              \
    uint8_t bLength;                            \
    uint8_t bDescriptorType;                    \
    uint8_t bDescriptorSubType;                 \
    uint8_t bJackType;                          \
    uint8_t bJackID;                            \
    uint8_t bNrInputPins;                       \
    ms_midi_out_jack_source_t sources[p];   \
    uint8_t iJack;                              \
  } ms_iface_out_##p##_jack_t;                 \
COMPILER_PACK_RESET()




// defines an ms class endpoint descriptor with p sources
#define MS_EP_DESC(p) COMPILER_PACK_SET(1)    \
  typedef struct {                              \
    uint8_t bLength;                            \
    uint8_t bDescriptorType;                    \
    uint8_t bDescriptorSubType;                 \
    uint8_t bNumEmbMIDIJack;                       \
    uint8_t baAssocJackID[p];               \
  } ms_ep_n##p##_desc_t;                        \
COMPILER_PACK_RESET()
  


MS_IFACE_OUT(1)
MS_EP_DESC(1)

COMPILER_PACK_SET(1)
typedef struct {
  ms_iface_desc_t iface;
  ms_iface_in_jack_t in0;
  ms_iface_out_1_jack_t out0;
  ms_iface_in_jack_t in1;
  ms_iface_out_1_jack_t out1;  
} ms_desc_t;
COMPILER_PACK_RESET()


//! Structure for USB Device Configuration Descriptor
COMPILER_PACK_SET(1)
typedef struct {
  usb_conf_desc_t conf;
  usb_iface_desc_t iface;
  ms_desc_t ms;
  
  usb_ep_desc_t epOut; 
  ms_ep_n1_desc_t msepOut;
  usb_ep_desc_t epIn; 
  ms_ep_n1_desc_t msepIn;
} udc_desc_t;
COMPILER_PACK_RESET()

//! USB Device Configuration Descriptor filled for full and high speed


COMPILER_WORD_ALIGNED
UDC_DESC_STORAGE udc_desc_t udc_desc_fs = {
	.conf.bLength              = sizeof(usb_conf_desc_t),
	.conf.bDescriptorType      = USB_DT_CONFIGURATION,
	.conf.wTotalLength         = LE16(sizeof(udc_desc_t)),
	.conf.bNumInterfaces       = 1,  
	.conf.bConfigurationValue  = 1,
	.conf.iConfiguration       = 0,
	.conf.bmAttributes         = USB_CONFIG_ATTR_MUST_SET | USB_DEVICE_ATTR,
	.conf.bMaxPower            = USB_CONFIG_MAX_POWER(USB_DEVICE_POWER),

    .iface.bLength             = sizeof(usb_iface_desc_t),
    .iface.bDescriptorType     = USB_DT_INTERFACE,
    .iface.bInterfaceNumber    = 0,
    .iface.bAlternateSetting   = 0,
    .iface.bNumEndpoints       = 2,
    .iface.bInterfaceClass     = USB_CLASS_AUDIO,
    .iface.bInterfaceSubClass  = USB_SUBCLASS_MIDI_STREAMING,
    .iface.bInterfaceProtocol  = 0,
    .iface.iInterface          = 0,

    .ms.iface.bLength          = sizeof(ms_iface_desc_t),
    .ms.iface.bDescriptorType  = CS_INTERFACE,
    .ms.iface.bDescriptorSubType = IFACE_SUBTYPE_MS_HEADER,
    .ms.iface.bcdMSC           = 0x0100,
    .ms.iface.wTotalLength     = LE16(sizeof(ms_desc_t)),

    .ms.in0.bLength            = sizeof(ms_iface_in_jack_t),
    .ms.in0.bDescriptorType    = CS_INTERFACE,
    .ms.in0.bDescriptorSubType = IFACE_SUBTYPE_MIDI_IN_JACK,
    .ms.in0.bJackType          = MS_MIDI_JACK_TYPE_EMBEDDED,
    .ms.in0.bJackID            = 16,
    .ms.in0.iJack              = 0,

    .ms.out0.bLength            = sizeof(ms_iface_out_1_jack_t),
    .ms.out0.bDescriptorType    = CS_INTERFACE,
    .ms.out0.bDescriptorSubType = IFACE_SUBTYPE_MIDI_OUT_JACK,
    .ms.out0.bJackType          = MS_MIDI_JACK_TYPE_EMBEDDED,
    .ms.out0.bJackID            = 48,
    .ms.out0.bNrInputPins       = 1,
    .ms.out0.sources[0].baSourceID  = 32,
    .ms.out0.sources[0].baSourcePin  = 1,        
    .ms.out0.iJack              = 0,    
     
    
    .ms.in1.bLength            = sizeof(ms_iface_in_jack_t),
    .ms.in1.bDescriptorType    = CS_INTERFACE,
    .ms.in1.bDescriptorSubType = IFACE_SUBTYPE_MIDI_IN_JACK,
    .ms.in1.bJackType          = MS_MIDI_JACK_TYPE_EXTERNAL,
    .ms.in1.bJackID            = 32,
    .ms.in1.iJack              = 0,

    .ms.out1.bLength            = sizeof(ms_iface_out_1_jack_t),
    .ms.out1.bDescriptorType    = CS_INTERFACE,
    .ms.out1.bDescriptorSubType = IFACE_SUBTYPE_MIDI_OUT_JACK,
    .ms.out1.bJackType          = MS_MIDI_JACK_TYPE_EXTERNAL,
    .ms.out1.bJackID            = 64,
    .ms.out1.bNrInputPins       = 1,
    .ms.out1.sources[0].baSourceID  = 16,
    .ms.out1.sources[0].baSourcePin  = 1,        
    .ms.out1.iJack              = 0,



    .epOut.bLength              = sizeof(usb_ep_desc_t),
    .epOut.bDescriptorType      = USB_DT_ENDPOINT,
    .epOut.bEndpointAddress     = 0x01,
    .epOut.bmAttributes         = USB_EP_TYPE_BULK,
    .epOut.wMaxPacketSize       = 64,
    .epOut.bInterval            = 0,


    .msepOut.bLength            = sizeof(ms_ep_n1_desc_t),
    .msepOut.bDescriptorType    = CS_ENDPOINT,
    .msepOut.bDescriptorSubType = EP_SUBTYPE_MS_GENERAL,
    .msepOut.bNumEmbMIDIJack    = 1,
    .msepOut.baAssocJackID[0]      = 16, 
    

    .epIn.bLength              = sizeof(usb_ep_desc_t),
    .epIn.bDescriptorType      = USB_DT_ENDPOINT,
    .epIn.bEndpointAddress     = 0x82,
    .epIn.bmAttributes         = USB_EP_TYPE_BULK,
    .epIn.wMaxPacketSize       = 64,
    .epIn.bInterval            = 0,


    .msepIn.bLength            = sizeof(ms_ep_n1_desc_t),
    .msepIn.bDescriptorType    = CS_ENDPOINT,
    .msepIn.bDescriptorSubType = EP_SUBTYPE_MS_GENERAL,
    .msepIn.bNumEmbMIDIJack    = 1,
    .msepIn.baAssocJackID[0]      = 48 
    
};


udi_api_t udi_api_midi;

//! Associate an UDI for each USB interface
UDC_DESC_STORAGE udi_api_t *udi_apis[1] = {
	&udi_api_midi,
};

//! Add UDI with USB Descriptors FS & HS
UDC_DESC_STORAGE udc_config_speed_t udc_config_fs[1] = { {
	.desc          = (usb_conf_desc_t UDC_DESC_STORAGE*)&udc_desc_fs,
	.udi_apis = udi_apis,
}};
 

//! Add all information about USB Device in global structure for UDC
UDC_DESC_STORAGE udc_config_t udc_config = {
	.confdev_lsfs = &udc_device_desc,
	.conf_lsfs = udc_config_fs,
	.conf_bos = NULL,
};

//@}
//@}
