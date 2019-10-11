// SFML Imports
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

// C++ Imports
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <queue>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

// X11 Setup
#ifdef LINUX
#include <X11/Xlib.h>
#ifdef None
#undef None
#define X11None 0L
#ifdef RevertToNone
#undef RevertToNone
#define RevertToNone (int)X11None
#endif
#endif
#endif

// Charly Imports
#include "value.h"
#include "vm.h"
#include "managedcontext.h"

// Simple SDL2 Graphics library
namespace Graphics {
  static constexpr size_t kGraphicsScale = 20;
  static constexpr uint8_t kIOKeyboardModifierAlt = 0x01;
  static constexpr uint8_t kIOKeyboardModifierControl = 0x02;
  static constexpr uint8_t kIOKeyboardModifierShift = 0x04;
  static constexpr uint8_t kIOKeyboardModifierSystem = 0x08;

  struct Vec {
    uint32_t x;
    uint32_t y;
  };

  struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
  };

  struct ImageBuffer {
    Color* buffer;

    ImageBuffer(const Vec size) {
      this->buffer = (Color*)malloc(sizeof(Color) * size.x * size.y);
    }

    ~ImageBuffer() {
      free(this->buffer);
    }
  };

  struct Window {
    Vec size;
    Color brush_color;

    ImageBuffer buf;

    sf::RenderWindow* window;
    std::thread render_thread;
    std::atomic<bool> shutdown;

    Window(const Vec size, const Color brush_color, sf::RenderWindow* window) : buf(size) {
      this->size = size;
      this->brush_color = brush_color;
      this->window = window;
      this->render_thread = std::thread(&Window::render, this);
    }

    void close() {
      this->shutdown = true;
      this->render_thread.join();
      this->window->close();
      delete this->window;
      this->window = nullptr;
    }

    void render() {
      while (!this->shutdown && this->window->isOpen()) {

        // Iterate over every pixel in VRAM and draw it to the screen
        sf::RectangleShape brush(sf::Vector2f(kGraphicsScale, kGraphicsScale));
        for (uint8_t y = 0; y < this->size.y; y++) {
          for (uint8_t x = 0; x < this->size.x; x++) {

            // Fetch the color from the buffer
            Color c = this->buf.buffer[x + y * this->size.x];
            sf::Color color {c.r, c.g, c.b};

            // Paint the corresponding section of the window
            brush.setFillColor(color);
            brush.setPosition(sf::Vector2f(x * kGraphicsScale, y * kGraphicsScale));
            this->window->draw(brush);
          }
        }

        this->window->display();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
      }
    }

    void set_size(const Vec size);

    sf::Event poll_event() {
      sf::Event event;
      this->window->pollEvent(event);
      return event;
    }

    Vec get_mouse_pos();

    void draw_rectangle(const Vec pos, const Vec size);
    void draw_square(const Vec pos, uint32_t size);
    void draw_line(const Vec pos, const Vec size);
    void draw_dot(const Vec pos);
  };

  class WindowManager {
  private:
    uint32_t next_id = 0;
    std::unordered_map<uint32_t, Window*> windows;

  public:
    uint32_t create_window(const Vec size) {
      sf::VideoMode video_mode(size.x * kGraphicsScale, size.y * kGraphicsScale);
      sf::RenderWindow* render_window = new sf::RenderWindow(video_mode, "SDL2", sf::Style::Titlebar);

      uint32_t id = this->next_id++;
      Window* window = new Window(size, Color{255, 0, 0}, render_window);
      this->windows.insert({id, window});

      return id;
    }

    void close_window(uint32_t window_id) {
      Window* window = this->get_window(window_id);

      if (window) {
        window->close();
      }

      this->windows.erase(window_id);
    }

    Window* get_window(uint32_t window_id) {
      if (!this->windows.count(window_id)) return nullptr;
      return this->windows[window_id];
    }
  };
}

using namespace Charly;

CHARLY_MANIFEST(

  // Graphics API
  F(create_window, 2)
  F(close_window, 1)
  F(sleep, 1)

  // Window interaction
  F(set_resolution, 3)
  F(get_mouse_pos, 1)
  F(poll_event, 1)

  // Drawing
  F(draw_rectangle, 5)
  F(draw_square, 4)
  F(draw_line, 5)
  F(draw_dot, 3)
  F(set_color, 4)
);

Graphics::WindowManager* wm;

CHARLY_API(__charly_constructor) () {
  wm = new Graphics::WindowManager();
  return kNull;
}

CHARLY_API(__charly_destructor) () {
  delete wm;
  wm = nullptr;
  return kNull;
}

CHARLY_API(sleep) (VM&vm, VALUE ms) {
  uint64_t dt = charly_number_to_uint64(ms);
  std::this_thread::sleep_for(std::chrono::milliseconds(dt));
  return kNull;
}

CHARLY_API(create_window) (VM& vm, VALUE w, VALUE h) {
  WARN_TYPE(number, w);
  WARN_TYPE(number, h);

  Graphics::Vec size = {charly_number_to_uint32(w), charly_number_to_uint32(h)};
  uint32_t window_id = wm->create_window(size);
  return charly_create_number(window_id);
}

CHARLY_API(close_window) (VM& vm, VALUE w) {
  WARN_TYPE(number, w);

  uint32_t window_id = charly_number_to_uint32(w);
  wm->close_window(window_id);

  return kNull;
}

CHARLY_API(set_resolution) (VM& vm, VALUE wid, VALUE w, VALUE h) {
  return kNull;
}

CHARLY_API(get_mouse_pos) (VALUE wid) {
  return kNull;
}

CHARLY_API(poll_event) (VM& vm, VALUE w) {
  WARN_TYPE(number, w);

  uint32_t wid = charly_number_to_uint32(w);
  Graphics::Window* window = wm->get_window(wid);

  if (!window) {
    return kNull;
  }

  sf::Event event = window->poll_event();

  uint8_t modifier_byte = 0x00;

  if (event.key.alt)
    modifier_byte |= Graphics::kIOKeyboardModifierAlt;
  if (event.key.control)
    modifier_byte |= Graphics::kIOKeyboardModifierControl;
  if (event.key.shift)
    modifier_byte |= Graphics::kIOKeyboardModifierShift;
  if (event.key.system)
    modifier_byte |= Graphics::kIOKeyboardModifierSystem;

  Charly::ManagedContext lalloc(vm);
  Object* obj = charly_as_object(lalloc.create_object(3));
  obj->container->insert({vm.context.symtable("type"), charly_create_number(event.type)});
  obj->container->insert({vm.context.symtable("keycode"), charly_create_number(event.key.code)});
  obj->container->insert({vm.context.symtable("modifiers"), charly_create_number(modifier_byte)});

  return charly_create_pointer(obj);
}

CHARLY_API(draw_rectangle) (VM& vm, VALUE wid, VALUE x, VALUE y, VALUE w, VALUE h) {
  CHECK(number, wid);
  CHECK(number, x);
  CHECK(number, y);

  uint32_t window_id = charly_number_to_uint32(wid);
  Graphics::Window* window = wm->get_window(window_id);
  if (!window) return kNull;

  uint32_t _x = charly_number_to_uint32(x);
  uint32_t _y = charly_number_to_uint32(y);
  uint32_t _w = charly_number_to_uint32(w);
  uint32_t _h = charly_number_to_uint32(h);

  for (uint32_t oy = 0; oy < _h; oy++) {
    for (uint32_t ox = 0; ox < _w; ox++) {
      uint32_t offset = (oy + _y) * window->size.x + (ox + _x);
      window->buf.buffer[offset] = window->brush_color;
    }
  }

  return kNull;
}

CHARLY_API(draw_square) (VM& vm, VALUE wid, VALUE x, VALUE y, VALUE s) {
  return kNull;
}

CHARLY_API(draw_line) (VM& vm, VALUE wid, VALUE x1, VALUE y1, VALUE x2, VALUE y2) {
  return kNull;
}

CHARLY_API(draw_dot) (VM& vm, VALUE wid, VALUE x, VALUE y) {
  CHECK(number, wid);
  CHECK(number, x);
  CHECK(number, y);

  uint32_t window_id = charly_number_to_uint32(wid);
  Graphics::Window* window = wm->get_window(window_id);
  if (!window) return kNull;

  uint32_t _x = charly_number_to_uint32(x);
  uint32_t _y = charly_number_to_uint32(y);

  const Graphics::Vec size = window->size;
  uint32_t offset = size.x * _y + _x;

  if (offset >= size.x * size.y) {
    return kNull;
  }

  window->buf.buffer[offset] = window->brush_color;

  return kNull;
}

CHARLY_API(set_color) (VM& vm, VALUE wid, VALUE r, VALUE g, VALUE b) {
  WARN_TYPE(number, wid);
  WARN_TYPE(number, r);
  WARN_TYPE(number, g);
  WARN_TYPE(number, b);

  uint8_t red   = charly_number_to_uint8(r);
  uint8_t green = charly_number_to_uint8(g);
  uint8_t blue  = charly_number_to_uint8(b);

  Graphics::Window* window = wm->get_window(charly_number_to_uint32(wid));

  if (!window) {
    return kNull;
  }

  window->brush_color = Graphics::Color {red, green, blue};

  return kNull;
}
