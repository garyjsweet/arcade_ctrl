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

#include "Encoder.h"

#include "quadrature.pio.h"

#include <cstdint>

Encoder::Encoder(uint32_t pioIndex, uint32_t pinA, uint32_t pinB, uint32_t ppr) :
    m_ppr(ppr)
{
    assert(pioIndex < 2);

    m_pio = pioIndex == 0 ? pio0 : pio1;

    // Claim state machine
    uint32_t offset = pio_add_program(m_pio, &quadrature_program);
    m_stateMachine = pio_claim_unused_sm(m_pio, true);
    quadrature_program_init(m_pio, m_stateMachine, offset, pinA, pinB);

    Zero();
}

void Encoder::Zero()
{
    pio_sm_exec(m_pio, m_stateMachine, pio_encode_set(pio_x, 0));
}

int32_t Encoder::ReadRaw() const
{
   pio_sm_exec_wait_blocking(m_pio, m_stateMachine, pio_encode_in(pio_x, 32));
   int32_t x = pio_sm_get_blocking(m_pio, m_stateMachine);
   return x;
}

int32_t Encoder::ReadAngle() const
{
   int32_t angle = ReadRaw() * 360 / m_ppr;
   return angle;
}