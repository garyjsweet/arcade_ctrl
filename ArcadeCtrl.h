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

#pragma once

#include "Encoder.h"
#include "Analog.h"
#include "InputData.h"
#include "USB.h"
#include "BlinkLED.h"

#include <cstdint>
#include <array>

class ArcadeCtrl
{
public:
    ArcadeCtrl();

    int Run();

private:
    void InitGPIO();
    void ReadInputs(InputData *inputs, const InputData &curInputs);
    void UpdateBlinker();

    bool NeedsSending(const InputData &data1, const InputData &data2);

private:
    struct BoardConfig
    {
        uint32_t numAnalogs  = 0;
        uint32_t numEncoders = 0;
        float    encoderGain = 1.0f;
    };

    static BoardConfig s_boardConfigs[4];

    BoardConfig m_boardCfg;
    Encoder     m_encoders[2];
    Analog      m_analogs[3];
    USB         m_usb;
    BlinkLED    m_blinker;

    std::array<uint32_t, 5> m_buttonDebounceArray {};
    uint32_t                m_buttonDebouncePos = 0;
};