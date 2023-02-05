//  SuperTux
//  Copyright (C) 2018 Ingo Ruhnke <grumbel@gmail.com>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_SUPERTUX_VIDEO_GL_GL_CONTEXT_HPP
#define HEADER_SUPERTUX_VIDEO_GL_GL_CONTEXT_HPP

#include <stddef.h>
#include <string>

#include "mario/mario_instance.hpp"
#include "video/gl.hpp"

class Color;
class GLTexture;
class Texture;

class GLContext
{
public:
  GLContext() {}
  virtual ~GLContext() {}

  virtual std::string get_name() const = 0;

  virtual void bind() = 0;

  virtual void ortho(float width, float height, bool vflip) = 0;

  virtual void blend_func(GLenum src, GLenum dst) = 0;

  virtual void set_positions(const float* data, size_t size) = 0;

  virtual void set_texcoords(const float* data, size_t size) = 0;
  virtual void set_texcoord(float u, float v) = 0;

  virtual void set_colors(const float* data, size_t size) = 0;
  virtual void set_color(const Color& color) = 0;

  virtual void bind_texture(const Texture& texture, const Texture* displacement_texture) = 0;
  virtual void bind_no_texture() = 0;

  virtual void draw_arrays(GLenum type, GLint first, GLsizei count) = 0;

  virtual bool supports_framebuffer() const = 0;

  virtual void init_sm64_texture(uint8_t* raw_texture, uint32_t* texture, int w, int h, bool linear) = 0;
  virtual void destroy_sm64_texture(uint32_t* texture) = 0;
  virtual void render_mario_instance(const SM64MarioGeometryBuffers* geometry, const Vector& pos, const Vector& camera, const uint32_t cap, const uint32_t& texture, const uint16_t* indices) = 0;
  virtual void render_sm64_texture(const uint32_t& texture, const Vector& pos, const Vector& size, const Vector& texCoord1, const Vector& texCoord2, const Color& color) = 0;

protected:
  float curr_mvp_matrix[9];

private:
  GLContext(const GLContext&) = delete;
  GLContext& operator=(const GLContext&) = delete;
};

#endif

/* EOF */
