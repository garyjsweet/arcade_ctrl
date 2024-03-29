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
#include "ArcadeCtrl.h"

#include "bsp/board.h"

// BOARD CONFIG - chosen based on the DIP of the connected board
// One config per dip option (00, 01, 10, 11)
ArcadeCtrl::BoardConfig ArcadeCtrl::s_boardConfigs[4] =
{
   // Analogs  Encoders  Gain
   0,          2,        10.0f,    // Player 1 with low PPR trackball
   0,          1,       -1.0f,     // Player 2 with high PPR spinner (reversed)
   0,          0,        1.0f,
   0,          0,        1.0f
};

// PIN CONFIG

// First 0-15 & pin 20 are button GPIO inputs.
// 16 through 19 are either encoder inputs or can be used for a second joystick.
// 21 & 22 are DIP switch inputs.
// 26, 27 & 28 are fixed analog inputs (hardcoded in Analog.cpp)

constexpr uint32_t INPUT_MASK = ((1 << 16) - 1) | (1 << 20);

constexpr uint32_t DIP_MASK   = (1 << 21) | (1 << 22);
constexpr uint32_t DIP_SHIFT  = 21;

constexpr uint32_t ENCODER0_A_PIN = 16;
constexpr uint32_t ENCODER0_B_PIN = 17;

constexpr uint32_t ENCODER1_A_PIN = 18;
constexpr uint32_t ENCODER1_B_PIN = 19;

constexpr uint32_t PIO_MASK = (1 << ENCODER0_A_PIN) | (1 << ENCODER0_B_PIN) |
                              (1 << ENCODER1_A_PIN) | (1 << ENCODER1_B_PIN);

constexpr uint32_t GPIO_MASK = INPUT_MASK | DIP_MASK | PIO_MASK;

constexpr uint32_t POLL_INTERVAL_MS = 1;

enum
{
   BLINK_NOT_MOUNTED = 250,
   BLINK_MOUNTED     = 1000,
   BLINK_SUSPENDED   = 2500,
};

ArcadeCtrl::ArcadeCtrl()
{
   board_init();
   InitGPIO();

   // Read the PID - this will also control the spinner/trackball gain
   // since we know which device is connected to which board.
   uint32_t pidDip = ((~gpio_get_all()) & DIP_MASK) >> DIP_SHIFT;

   m_boardCfg = s_boardConfigs[pidDip];

   m_usb = USB(pidDip, m_boardCfg.numAnalogs, m_boardCfg.numEncoders);

   // Create our encoder inputs
   if (m_boardCfg.numEncoders > 0)
      m_encoders[0] = Encoder(0, ENCODER0_A_PIN, ENCODER0_B_PIN, m_boardCfg.encoderGain);
   if (m_boardCfg.numEncoders > 1)
      m_encoders[1] = Encoder(1, ENCODER1_A_PIN, ENCODER1_B_PIN, m_boardCfg.encoderGain);

   // Create our analog inputs
   for (uint32_t i = 0; i < m_boardCfg.numAnalogs; i++)
      m_analogs[i] = Analog(i);

   // The tiny-usb code is essentially a singleton, so register our
   // class to interact with it
   RegisterUSBHandler(&m_usb);
}

void ArcadeCtrl::UpdateBlinker()
{
   uint32_t blinkInterval = 0;

   if (m_usb.IsMounted())
      blinkInterval = BLINK_MOUNTED;
   else if (m_usb.IsSuspended())
      blinkInterval = BLINK_SUSPENDED;
   else
      blinkInterval = BLINK_NOT_MOUNTED;

   m_blinker.SetBlinkInterval(blinkInterval);
   m_blinker.Process();
}

int ArcadeCtrl::Run()
{
   uint32_t startMS = 0;

   do
   {
      // We do these two every time in the loop, regardless of polling interval
      m_usb.Process();
      UpdateBlinker();

      uint32_t now = to_ms_since_boot(get_absolute_time());

      // Poll inputs at defined interval
      if (now - startMS < POLL_INTERVAL_MS)
         continue;

      startMS = now;

      const InputData &lastSent = m_usb.LastSentData();
      InputData        inputs;

      ReadInputs(&inputs, lastSent);

      if (NeedsSending(inputs, lastSent))
         m_usb.SendData(inputs);
   }
   while (true);

   return 0;
}

void ArcadeCtrl::ReadInputs(InputData *inputs, const InputData &lastSent)
{
   *inputs = {};

   // Our input pins are pulled-up, so we need to invert to get the up/down state
   uint32_t buttons = (~gpio_get_all()) & INPUT_MASK;

   // We also want to debounce the buttons. We'll pass the press immediately,
   // but wait for 5ms for bouncing on release. This will debounce both the press
   // and release, but won't affect the latency of the press reaching the device.
   // Essentially, every sampling period we overwrite the oldest entry in a circular
   // buffer with the current state of the buttons, one bit per button (0=up, 1=down).
   // We simply bitwise OR every entry in the buffer together to get the input to
   // send to the device. So, any button registered as down will be seen as down
   // immediately, but will take at least 5ms to clear the ring buffer and become
   // UP again. Any bouncing in that 5ms period is then essentially ignored.
   m_buttonDebounceArray[m_buttonDebouncePos++] = buttons;
   if (m_buttonDebouncePos >= m_buttonDebounceArray.size())
      m_buttonDebouncePos = 0;

   for (uint32_t i = 0; i < m_buttonDebounceArray.size(); i++)
      inputs->buttons |= m_buttonDebounceArray[i];

   for (uint32_t i = 0; i < m_boardCfg.numAnalogs; i++)
      inputs->analog[i] = m_analogs[i].Read();

   for (uint32_t i = 0; i < m_boardCfg.numEncoders; i++)
   {
      inputs->angle[i] = m_encoders[i].Read();

      // Update the delta angle
      inputs->angleDelta[i] = inputs->angle[i] - lastSent.angle[i];
   }
}

bool ArcadeCtrl::NeedsSending(const InputData &cur, const InputData &prev)
{
   if (cur.buttons != prev.buttons)
      return true;

   for (uint32_t i = 0; i < m_boardCfg.numAnalogs; i++)
      if (cur.analog[i] != prev.analog[i])
         return true;

   for (uint32_t i = 0; i < m_boardCfg.numEncoders; i++)
      if (cur.angle[i] != prev.angle[i] || cur.angleDelta != 0)
         return true;

   return false;
}

void ArcadeCtrl::InitGPIO()
{
   // All button, analog & encoder pins must be internally pulled up, and marked as input
   gpio_init_mask(GPIO_MASK);
   gpio_set_dir_in_masked(GPIO_MASK);

   for (uint8_t i = 0; i < 32; i++)
      if (GPIO_MASK & (1 << i))
         gpio_pull_up(i);

    gpio_set_dir_out_masked(LED_MASK);
}
