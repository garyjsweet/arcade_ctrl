/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 * Copyright (c) 2023 Gary Sweet
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "USB.h"

#include "bsp/board.h"
#include "tusb.h"

/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]         HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))
#define USB_PID(variant) (0x4000 | (uint8_t)((variant) << 8) | _PID_MAP(CDC, 0) | \
                         _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                         _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4))

#define USB_VID 0xBA5E
#define USB_BCD 0x0200

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define EPNUM_HID 0x81
enum
{
  ITF_NUM_HID,
  ITF_NUM_TOTAL
};

enum
{
  REPORT_ID_GAMEPAD = 1,
  REPORT_ID_MOUSE,
  REPORT_ID_COUNT
};

static USB *s_usbHandler;

// Descriptor contents must exist long enough for transfer to complete.
static uint8_t gamepadOnly[] =
{
   TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(REPORT_ID_GAMEPAD))
};
static uint8_t gamepadAndMouse[] =
{
   TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(REPORT_ID_GAMEPAD)),
   TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_MOUSE))
};

void RegisterUSBHandler(USB *usb)
{
   assert(s_usbHandler == nullptr);
   s_usbHandler = usb;
}

USB::USB(uint8_t pidDipValue, uint32_t numAnalogs, uint32_t numEncoders) :
   m_pidVariant(pidDipValue),
   m_numAnalogs(numAnalogs),
   m_numEncoders(numEncoders)
{
   tusb_init();
}

void USB::Process()
{
   // tinyusb device task
   tud_task();
}

const uint8_t *USB::DeviceDescriptor() const
{
   static const tusb_desc_device_t desc =
   {
      .bLength = sizeof(tusb_desc_device_t),
      .bDescriptorType = TUSB_DESC_DEVICE,
      .bcdUSB = USB_BCD,
      .bDeviceClass = 0x00,
      .bDeviceSubClass = 0x00,
      .bDeviceProtocol = 0x00,
      .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

      .idVendor = USB_VID,
      .idProduct = USB_PID(m_pidVariant),
      .bcdDevice = 0x0100,

      .iManufacturer = 0x01,
      .iProduct = 0x02,
      .iSerialNumber = 0x03,

      .bNumConfigurations = 0x01
   };

   return (const uint8_t*)&desc;
}

const uint8_t *USB::HIDDescReport() const
{
   return m_numEncoders > 0 ? gamepadAndMouse : gamepadOnly;
}

size_t USB::HIDDescReportSize() const
{
   return m_numEncoders > 0 ? sizeof(gamepadAndMouse) : sizeof(gamepadOnly);
}

const uint8_t *USB::DescriptorConfig() const
{
   static const uint8_t config[] =
   {
      // Config number, interface count, string index, total length, attribute, power in mA
      TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

      // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
      TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, HIDDescReportSize(), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5)
   };

   return config;
}

const uint16_t *USB::DescriptorString(uint8_t index, uint16_t langid) const
{
   // array of pointer to string descriptors
   char const *stringDescArray[] =
   {
      (const char[]){0x09, 0x04}, // 0: is supported language is English (0x0409)
      "Gary Sweet",               // 1: Manufacturer
      "Arcade Interface"          // 2: Product
      "1",                        // 3: Serials, should use chip ID
   };

   static uint16_t descStr[32];

   uint8_t chrCnt;

   if (index == 0)
   {
      memcpy(&descStr[1], stringDescArray[0], 2);
      chrCnt = 1;
   }
   else
   {
      // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
      // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

      if (!(index < sizeof(stringDescArray) / sizeof(stringDescArray[0])))
         return NULL;

      const char *str = stringDescArray[index];

      // Cap at max char
      chrCnt = strlen(str);
      if (chrCnt > 31)
         chrCnt = 31;

      // Convert ASCII string into UTF-16
      for (uint8_t i = 0; i < chrCnt; i++)
         descStr[1 + i] = str[i];
   }

   // first byte is length (including header), second byte is string type
   descStr[0] = (TUSB_DESC_STRING << 8) | (2 * chrCnt + 2);

   return descStr;
}

// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void USB::SendData(const InputData &input)
{
   // Remote wakeup
   if (tud_suspended())
   {
      // Wake up host if we are in suspend mode and REMOTE_WAKEUP feature is enabled by host
      tud_remote_wakeup();
   }
   else
   {
      // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
      m_inputData = input;
      SendHIDReport(REPORT_ID_GAMEPAD);
   }
}

// Only called if there is new data in s_cur_data
void USB::SendHIDReport(uint8_t reportID)
{
   // skip if hid is not ready yet
   if (!tud_hid_ready())
      return;

   switch (reportID)
   {
   case REPORT_ID_GAMEPAD:
   {
      hid_gamepad_report_t report = {};
      report.buttons = m_inputData.buttons;

      if (m_numAnalogs > 0)
         report.x = m_inputData.USBValueFromAnalog(0);
      if (m_numAnalogs > 1)
         report.y = m_inputData.USBValueFromAnalog(1);
      if (m_numAnalogs > 2)
         report.rx = m_inputData.USBValueFromAnalog(2);

      tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));

      if (m_numEncoders == 0) // Record last if no mouse data
         m_lastSentData = m_inputData;

      break;
   }
   case REPORT_ID_MOUSE: // Spinners and trackballs
   {
      if (m_numEncoders == 0)
         break;

      hid_mouse_report_t report = {};

      if (m_numEncoders > 0)
         report.x = m_inputData.angleDelta[0];
      if (m_numEncoders > 1)
         report.y = m_inputData.angleDelta[1];

      if (report.x == 0 && report.y == 0)
         break; // Don't send if no deltas

      tud_hid_report(REPORT_ID_MOUSE, &report, sizeof(report));

      m_lastSentData = m_inputData; // Record last

      break;
   }
   default:
      break;
   }
}
//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const *tud_descriptor_device_cb(void)
{
   return s_usbHandler->DeviceDescriptor();
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
   return s_usbHandler->HIDDescReport();
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
  // This example use the same configuration for both high and full speed mode
  return s_usbHandler->DescriptorConfig();
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
   return s_usbHandler->DescriptorString(index, langid);
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb()
{
   s_usbHandler->SetMounted(true);
}

// Invoked when device is unmounted
void tud_umount_cb()
{
   s_usbHandler->SetMounted(false);
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
   s_usbHandler->SetSuspended(true);
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
   s_usbHandler->SetSuspended(false);
   s_usbHandler->SetMounted(true);
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint8_t len)
{
   uint8_t nextReportID = report[0] + 1;

   if (nextReportID < REPORT_ID_COUNT)
      s_usbHandler->SendHIDReport(nextReportID);
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
   // Not Implemented
   return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
   // Not Implemented
}
