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

#ifndef HEADER_SUPERTUX_VIDEO_GL_GL20_CONTEXT_HPP
#define HEADER_SUPERTUX_VIDEO_GL_GL20_CONTEXT_HPP

#include "video/gl/gl_context.hpp"

#ifndef USE_OPENGLES2

class GL20Context final : public GLContext
{
public:
  GL20Context();
  ~GL20Context() override;

  virtual std::string get_name() const override { return "opengl20"; }

  virtual void bind() override;

  virtual void ortho(float width, float height, bool vflip) override;

  virtual void blend_func(GLenum src, GLenum dst) override;

  virtual void set_positions(const float* data, size_t size) override;

  virtual void set_texcoords(const float* data, size_t size) override;
  virtual void set_texcoord(float u, float v) override;

  virtual void set_colors(const float* data, size_t size) override;
  virtual void set_color(const Color& color) override;

  virtual void bind_texture(const Texture& texture, const Texture* displacement_texture) override;
  virtual void bind_no_texture() override;

  virtual void draw_arrays(GLenum type, GLint first, GLsizei count) override;

  virtual bool supports_framebuffer() const override { return false; }

  /** mario only */
  virtual void init_sm64_texture(uint8_t* raw_texture, uint32_t* texture, int w, int h, bool linear) override;
  virtual void destroy_sm64_texture(uint32_t* texture) override;
  virtual void render_mario_instance(const SM64MarioGeometryBuffers* geometry, const Vector& pos, const Vector& camera, const uint32_t cap, const uint32_t& texture, const uint16_t* indices) override;
  virtual void render_sm64_texture(const uint32_t& texture, const Vector& pos, const Vector& size, const Vector& texCoord1, const Vector& texCoord2, const Color& color) override;

private:
  GLfloat last_proj_matrix[16];

private:
  GL20Context(const GL20Context&) = delete;
  GL20Context& operator=(const GL20Context&) = delete;
};

#endif

#endif

/* EOF */
