/*
 * The MIT License (MIT)
 *
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

#pragma once
#include <stddef.h>

#include "InputData.h"

class USB
{
public:
   USB() = default;
   USB(uint8_t pidDipValue, uint32_t numAnalogs, uint32_t numEncoders);

   void SendData(const InputData &input);
   const InputData &LastSentData() const { return m_lastSentData; }

   void SendHIDReport(uint8_t reportID);

   void SetMounted(bool tf) { m_mounted = tf; }
   bool IsMounted() const   { return m_mounted; }

   void SetSuspended(bool tf) { m_suspended = tf; }
   bool IsSuspended() const   { return m_suspended; }

   void Process();

   const uint8_t  *DeviceDescriptor() const;
   const uint8_t  *HIDDescReport() const;
   size_t          HIDDescReportSize() const;
   const uint8_t  *DescriptorConfig() const;
   const uint16_t *DescriptorString(uint8_t index, uint16_t langid) const;

private:
   uint8_t   m_pidVariant  = 0;
   uint32_t  m_numAnalogs  = 0;
   uint32_t  m_numEncoders = 0;
   bool      m_mounted     = false;
   bool      m_suspended   = false;
   InputData m_inputData {};
   InputData m_lastSentData {};
};

void RegisterUSBHandler(USB *usb);