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
#include "BlinkLED.h"

#include "hardware/gpio.h"
#include "pico/stdlib.h"

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void BlinkLED::Process()
{
   // blink is disabled
   if (!m_intervalMS)
   {
      if (m_ledState)
      {
         m_ledState = false;
         gpio_clr_mask(LED_MASK);
      }
      return;
   }

   // Blink every interval ms
   uint32_t now = to_ms_since_boot(get_absolute_time());
   if (now - m_lastMS < m_intervalMS)
      return;

   m_lastMS = now;

   if (m_ledState)
      gpio_set_mask(LED_MASK);
   else
      gpio_clr_mask(LED_MASK);

   m_ledState = !m_ledState;
}
