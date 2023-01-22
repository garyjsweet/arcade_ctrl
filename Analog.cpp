/*
 * The MIT License (MIT)

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

#include "Analog.h"

#include "hardware/adc.h"

Analog::Analog(uint32_t channel) :
   m_channel(channel)
{
   if (!IsValid())
      return;

   adc_init();

   // Make sure analog GPIO is high-impedance, no pullups etc
   adc_gpio_init(channel + 26);

   // The rate at which the ADC gathers samples is determined by this line.
   // Clock divide (abbreviated as "clkdiv") allows you to split the 48 MHz
   // base clock and sample at a lesser rate. Currently, collecting a single
   // sample takes 96 cycles. This results in a maximum sample rate of
   // 500,000 samples per second (48,000,000 cycles per second / 96 cycles
   // each sample).
   // clkdiv : 9600 * 5 = 1ms
   adc_set_clkdiv(9600 * 5);
}

uint16_t Analog::Read() const
{
   adc_select_input(m_channel);
   return adc_read();
}
