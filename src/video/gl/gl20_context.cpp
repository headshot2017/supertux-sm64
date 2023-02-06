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

#include "video/gl/gl20_context.hpp"

#include "supertux/console.hpp"
#include "supertux/globals.hpp"
#include "video/glutil.hpp"
#include "video/color.hpp"
#include "video/gl/gl_texture.hpp"
#include "video/gl/gl_video_system.hpp"

#ifndef USE_OPENGLES2

GL20Context::GL20Context()
{
  assert_gl();
}

GL20Context::~GL20Context()
{
}

void
GL20Context::bind()
{
  assert_gl();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);

  assert_gl();
}

void
GL20Context::ortho(float width, float height, bool vflip)
{
  assert_gl();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  if (vflip)
  {
    glOrtho(0, static_cast<double>(width),
            static_cast<double>(height), 0,
            -1, 1);
  }
  else
  {
    glOrtho(0, static_cast<double>(width),
            0, static_cast<double>(height),
            -1, 1);
  }

  glGetFloatv(GL_PROJECTION_MATRIX, last_proj_matrix);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  assert_gl();
}

void
GL20Context::blend_func(GLenum src, GLenum dst)
{
  assert_gl();

  glBlendFunc(src, dst);

  assert_gl();
}

void
GL20Context::set_positions(const float* data, size_t size)
{
  assert_gl();

  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, data);

  assert_gl();
}

void
GL20Context::set_texcoords(const float* data, size_t size)
{
  assert_gl();

  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glTexCoordPointer(2, GL_FLOAT, 0, data);

  assert_gl();
}

void
GL20Context::set_texcoord(float u, float v)
{
  assert_gl();

  glDisableClientState(GL_TEXTURE_COORD_ARRAY);

  assert_gl();
}

void
GL20Context::set_colors(const float* data, size_t size)
{
  assert_gl();

  glEnableClientState(GL_COLOR_ARRAY);
  glColorPointer(4, GL_FLOAT, 0, data);

  assert_gl();
}

void
GL20Context::set_color(const Color& color)
{
  assert_gl();

  glDisableClientState(GL_COLOR_ARRAY);
  glColor4f(color.red, color.green, color.blue, color.alpha);

  assert_gl();
}

void
GL20Context::bind_texture(const Texture& texture, const Texture* displacement_texture)
{
  assert_gl();

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, static_cast<const GLTexture&>(texture).get_handle());

  assert_gl();

  Vector animate = static_cast<const GLTexture&>(texture).get_sampler().get_animate();
  if (animate.x == 0.0f && animate.y == 0.0f)
  {
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
  }
  else
  {
    animate.x /= static_cast<float>(texture.get_image_width());
    animate.y /= static_cast<float>(texture.get_image_height());

    animate *= g_game_time;

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glTranslatef(animate.x, animate.y, 0.0f);
    glMatrixMode(GL_MODELVIEW);
  }

  assert_gl();
}

void
GL20Context::bind_no_texture()
{
  assert_gl();

  glDisable(GL_TEXTURE_2D);

  assert_gl();
}

void
GL20Context::draw_arrays(GLenum type, GLint first, GLsizei count)
{
  assert_gl();

  glDrawArrays(type, first, count);

  assert_gl();
}

void
GL20Context::init_sm64_texture(uint8_t* raw_texture, uint32_t* texture, int w, int h, bool linear)
{
  glGenTextures(1, texture);
  glBindTexture(GL_TEXTURE_2D, *texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, raw_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void
GL20Context::destroy_sm64_texture(uint32_t* texture)
{
  glDeleteTextures(1, texture);
}

void
GL20Context::render_mario_instance(const SM64MarioGeometryBuffers* geometry, const Vector& pos, const Vector& camera, const uint32_t cap, const uint32_t& texture, const uint16_t* indices)
{
  uint32_t triangleSize = geometry->numTrianglesUsed*3;

  const Viewport& viewport = VideoSystem::current()->get_viewport();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(camera.x, viewport.get_screen_width() + camera.x,
          viewport.get_screen_height() + camera.y, camera.y,
          -1000.f, 10000.f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glColor4f(1,1,1,1);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  // give mario some lighting instead of being flat colors
  GLfloat light_position[] = { pos.x, pos.y-48, 192, 1 };
  GLfloat light_diffuse[] = { 0.6f, 0.6f, 0.6f, 1 };
  GLfloat light_model[] = { 0.5f, 0.5f, 0.5f, 1 };
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light_model);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_DEPTH_BUFFER_BIT);

  glVertexPointer(3, GL_FLOAT, 0, geometry->position);
  glNormalPointer(GL_FLOAT, 0, geometry->normal);
  glColorPointer(3, GL_FLOAT, 0, geometry->color);
  glTexCoordPointer(2, GL_FLOAT, 0, geometry->uv);

  // first, draw geometry without Mario's texture.
  glDisable(GL_TEXTURE_2D);
  glDrawElements(GL_TRIANGLES, triangleSize, GL_UNSIGNED_SHORT, indices);

  // now disable the color array and enable the texture.
  glDisableClientState(GL_COLOR_ARRAY);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();

  glDrawElements(GL_TRIANGLES, triangleSize, GL_UNSIGNED_SHORT, indices);

  glDisableClientState(GL_NORMAL_ARRAY);
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(last_proj_matrix);
}

void
GL20Context::render_sm64_texture(const uint32_t& texture, const Vector& pos, const Vector& size, const Vector& texCoord1, const Vector& texCoord2, const Color& color)
{
  glColor4f(color.red, color.green, color.blue, color.alpha);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  float fpos[8] = {
    pos.x, pos.y,
    pos.x+size.x, pos.y,
    pos.x+size.x, pos.y+size.y,
    pos.x, pos.y+size.y,
  };
  float uv[8] = {
    texCoord1.x, texCoord1.y,
    texCoord2.x, texCoord1.y,
    texCoord2.x, texCoord2.y,
    texCoord1.x, texCoord2.y,
  };

  glVertexPointer(2, GL_FLOAT, 0, fpos);
  glTexCoordPointer(2, GL_FLOAT, 0, uv);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();

  glDrawArrays(GL_QUADS, 0, 8);
}

#endif

/* EOF */
