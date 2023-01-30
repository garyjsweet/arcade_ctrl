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

#include "EncoderPio.h"

#include <cstdint>

int32_t Encoder::m_rotation[2];

void Encoder::IRQHandler0()
{
    // test if irq 0 was raised
    if (pio0_hw->irq & 1)
        m_rotation[0] += 1;
    // test if irq 1 was raised
    if (pio0_hw->irq & 2)
        m_rotation[0] -= 1;
    // clear both interrupts
    pio0_hw->irq = 3;
}

void Encoder::IRQHandler1()
{
    // test if irq 0 was raised
    if (pio1_hw->irq & 1)
        m_rotation[1] += 1;
    // test if irq 1 was raised
    if (pio1_hw->irq & 2)
        m_rotation[1] -= 1;
    // clear both interrupts
    pio1_hw->irq = 3;
}

Encoder::Encoder(uint32_t pioIndex, uint32_t pinA, uint32_t pinB, uint32_t ppr) :
    m_pioIndex(pioIndex),
    m_ppr(ppr)
{
    assert(pioIndex < 2);

    m_pio = pioIndex == 0 ? pio0 : pio1;

    pio_gpio_init(m_pio, pinA);
    pio_gpio_init(m_pio, pinB);

    // Claim state machine
    uint32_t offset = pio_add_program(m_pio, &QuadEncoder_program);
    m_stateMachine = pio_claim_unused_sm(m_pio, true);
    EncoderProgramInit(m_pio, m_stateMachine, offset, pinA, pinB);

    // set the IRQ handler
    if (pioIndex == 0)
    {
        irq_set_exclusive_handler(PIO0_IRQ_0, IRQHandler0);
        // enable the IRQ
        irq_set_enabled(PIO0_IRQ_0, true);
        pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS | PIO_IRQ0_INTE_SM1_BITS;
    }
    else
    {
        irq_set_exclusive_handler(PIO1_IRQ_0, IRQHandler1);
        // enable the IRQ
        irq_set_enabled(PIO1_IRQ_0, true);
        pio1_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS | PIO_IRQ0_INTE_SM1_BITS;
    }

    Zero();
}

void Encoder::Zero()
{
    m_rotation[m_pioIndex] = 0;
}

int32_t Encoder::Read() const
{
   return m_rotation[m_pioIndex];
}

