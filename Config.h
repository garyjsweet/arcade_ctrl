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

#include <cstdint>
#include <cassert>
#include <memory.h>
#include <utility>

using PinPair = std::pair<uint32_t, uint32_t>;

class Config
{
public:
   static constexpr uint32_t MAX_ADCS = 3;

public:
   Config(bool hasADC[MAX_ADCS], PinPair enc0Pins = {}, PinPair enc1Pins = {}) :
      m_enc0Pins(enc0Pins),
      m_enc1Pins(enc1Pins)
   {
      memcpy(m_hasADC, hasADC, sizeof(m_hasADC));
      m_hasEncoders = enc0Pins.first != 0 || enc1Pins.first != 0;
   }

   bool HasADC(uint32_t adc) const
   {
      assert(adc < MAX_ADCS);
      return m_hasADC[adc];
   }

   bool HasAnyADC() const
   {
      for (uint32_t i = 0; i < MAX_ADCS; i++)
         if (m_hasADC[i])
            return true;
      return false;
   }

   bool HasEncoders() const { return m_hasEncoders; }

   const PinPair &Encoder0Pins() const { return m_enc0Pins; }
   const PinPair &Encoder1Pins() const { return m_enc1Pins; }

private:
   bool    m_hasADC[3];
   bool    m_hasEncoders;
   PinPair m_enc0Pins;
   PinPair m_enc1Pins;
};
