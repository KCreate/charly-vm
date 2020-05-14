/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

const gfx = import "../libraries/sfml.lib"

const type = {
  closed:                 0x00,
  resized:                0x01,

  lostfocus:              0x02,
  gainedfocus:            0x03,

  textentered:            0x04,
  keypressed:             0x05,
  keyreleased:            0x06,

  mousewheelmoved:        0x07,
  mousewheelscrolled:     0x08,

  mousebuttonpressed:     0x09,
  mousebuttonreleased:    0x0A,

  mousemoved:             0x0B,
  mouseentered:           0x0C,
  mouseleft:              0x0D,

  joystickbuttonpressed:  0x0E,
  joystickbuttonreleased: 0x0F,
  joystickmoved:          0x10,
  joystickconnected:      0x11,
  joystickdisconnected:   0x12,

  touchbegan:             0x13,
  touchmoved:             0x14,
  touchended:             0x15,

  sensorchanged:          0x16
}

const w = gfx.create_window(16, 16)

const chars = [
  "a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
  "k", "l", "m", "n", "o", "p", "q", "r", "s", "t",
  "u", "v", "w", "x", "y", "z", "0", "1", "2", "3",
  "4", "5", "6", "7", "8", "9"
]

func character_from_keycode(keycode) {
  print(keycode)
  return chars[keycode]
}

const pos = {
  x: 0,
  y: 0
}

loop {

  // Poll for an event
  const e = gfx.poll_event(w);
  if (!e) break
  if (e.type == type.closed) {
    gfx.close_window(w)
    break
  }

  // Interact
  switch e.type {
    case type.keypressed {
      switch (e.keycode) {
        case chars.index("w") {
          print("up")
          pos.y -= 1
        }
        case chars.index("a") {
          print("left")
          pos.x -= 1
        }
        case chars.index("s") {
          print("down")
          pos.y += 1
        }
        case chars.index("d") {
          print("right")
          pos.x += 1
        }
      }
    }
  }

  // Render the screen black
  gfx.set_color(w, 0, 0, 0)
  gfx.draw_rectangle(w, 0, 0, 16, 16)
  gfx.set_color(w, 0xFF, 0xCE, 0x42)

  // Draw the player to the screen
  gfx.draw_dot(w, pos.x, pos.y)
}
