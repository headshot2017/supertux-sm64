//  SuperTux
//  Copyright (C) 2016 Ingo Ruhnke <grumbel@gmail.com>
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


#include "video/gl/gl33core_context.hpp"

#include <boost/format.hpp>

extern "C" {
#include <decomp/include/sm64shared.h>
}

#include "supertux/console.hpp"
#include "supertux/globals.hpp"
#include "video/color.hpp"
#include "video/gl/gl_program.hpp"
#include "video/gl/gl_texture.hpp"
#include "video/gl/gl_texture_renderer.hpp"
#include "video/gl/gl_vertex_arrays.hpp"
#include "video/gl/gl_video_system.hpp"
#include "video/glutil.hpp"

GL33CoreContext::GL33CoreContext(GLVideoSystem& video_system) :
  m_video_system(video_system),
  m_program(),
  m_vertex_arrays(),
  m_white_texture(),
  m_black_texture(),
  m_grey_texture(),
  m_transparent_texture()
{
  assert_gl();

  m_program.reset(new GLProgram);
  m_vertex_arrays.reset(new GLVertexArrays(*this));
  m_white_texture.reset(new GLTexture(1, 1, Color::WHITE));
  m_black_texture.reset(new GLTexture(1, 1, Color::BLACK));
  m_grey_texture.reset(new GLTexture(1, 1, Color::from_rgba8888(128, 128, 0, 0)));
  m_transparent_texture.reset(new GLTexture(1, 1, Color(1.0f, 0, 0, 0)));

  assert_gl();
}

GL33CoreContext::~GL33CoreContext()
{
}

void
GL33CoreContext::bind()
{
  assert_gl();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);

  m_program->bind();
  m_vertex_arrays->bind();

  GLTextureRenderer* back_renderer = static_cast<GLTextureRenderer*>(m_video_system.get_back_renderer());

  GLTexture* texture;
  if (back_renderer->is_rendering() || !back_renderer->get_texture())
  {
    texture = m_black_texture.get();
    glUniform1f(m_program->get_uniform_location("backbuffer"), 0.0f);
  }
  else
  {
    texture = static_cast<GLTexture*>(back_renderer->get_texture().get());
    glUniform1f(m_program->get_uniform_location("backbuffer"), 1.0f);
  }

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, texture->get_handle());

  const float tsx =
    static_cast<float>(texture->get_image_width()) /
    static_cast<float>(texture->get_texture_width());

  const float tsy =
    static_cast<float>(texture->get_image_height()) /
    static_cast<float>(texture->get_texture_height());

  const Rect& rect = m_video_system.get_viewport().get_rect();

  const float sx = tsx / static_cast<float>(rect.get_width());
  const float sy = tsy / static_cast<float>(rect.get_height());
  const float tx = -static_cast<float>(rect.left) / static_cast<float>(rect.get_width());
  const float ty = -static_cast<float>(rect.top) / static_cast<float>(rect.get_height());

  const float matrix[3*3] = {
    sx, 0.0, 0,
    0.0, sy, 0,
    tx, ty, 1.0,
  };
  glUniformMatrix3fv(m_program->get_uniform_location("fragcoord2uv"),
                     1, false, matrix);

  glUniform1i(m_program->get_uniform_location("diffuse_texture"), 0);
  glUniform1i(m_program->get_uniform_location("displacement_texture"), 1);
  glUniform1i(m_program->get_uniform_location("framebuffer_texture"), 2);

  glUniform1f(m_program->get_uniform_location("game_time"), g_game_time);

  assert_gl();
}

void
GL33CoreContext::ortho(float width, float height, bool vflip)
{
  assert_gl();

  const float sx = 2.0f / static_cast<float>(width);
  const float sy = -2.0f / static_cast<float>(height) * (vflip ? 1.0f : -1.0f);

  const float tx = -1.0f;
  const float ty = 1.0f * (vflip ? 1.0f : -1.0f);

  const float mvp_matrix[] = {
    sx, 0, tx,
    0, sy, ty,
    0, 0, 1
  };

  memcpy(curr_mvp_matrix, mvp_matrix, sizeof(curr_mvp_matrix));

  const GLint mvp_loc = m_program->get_uniform_location("modelviewprojection");
  
  glUniformMatrix3fv(mvp_loc, 1, false, mvp_matrix);

  assert_gl();
}

void
GL33CoreContext::blend_func(GLenum src, GLenum dst)
{
  assert_gl();

  glBlendFunc(src, dst);

  assert_gl();
}

void
GL33CoreContext::set_positions(const float* data, size_t size)
{
  m_vertex_arrays->set_positions(data, size);
}

void
GL33CoreContext::set_texcoords(const float* data, size_t size)
{
  m_vertex_arrays->set_texcoords(data, size);
}

void
GL33CoreContext::set_texcoord(float u, float v)
{
  m_vertex_arrays->set_texcoord(u, v);
}

void
GL33CoreContext::set_colors(const float* data, size_t size)
{
  m_vertex_arrays->set_colors(data, size);
}

void
GL33CoreContext::set_color(const Color& color)
{
  m_vertex_arrays->set_color(color);
}

void
GL33CoreContext::bind_texture(const Texture& texture, const Texture* displacement_texture)
{
  assert_gl();

  GLTextureRenderer* back_renderer = static_cast<GLTextureRenderer*>(m_video_system.get_back_renderer());

  if (displacement_texture && back_renderer->is_rendering())
  {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_transparent_texture->get_handle());
  }
  else
  {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, static_cast<const GLTexture&>(texture).get_handle());

    Vector animate = static_cast<const GLTexture&>(texture).get_sampler().get_animate();

    animate.x /= static_cast<float>(texture.get_image_width());
    animate.y /= static_cast<float>(texture.get_image_height());

    glUniform2f(m_program->get_uniform_location("animate"), animate.x, animate.y);
  }

  if (displacement_texture)
  {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, static_cast<const GLTexture&>(*displacement_texture).get_handle());

    Vector animate = static_cast<const GLTexture&>(*displacement_texture).get_sampler().get_animate();

    animate.x /= static_cast<float>(displacement_texture->get_image_width());
    animate.y /= static_cast<float>(displacement_texture->get_image_height());

    glUniform2f(m_program->get_uniform_location("displacement_animate"), animate.x, animate.y);
  }
  else
  {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_grey_texture->get_handle());
  }

  assert_gl();
}

void
GL33CoreContext::bind_no_texture()
{
  assert_gl();

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_white_texture->get_handle());

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, m_grey_texture->get_handle());

  assert_gl();
}

void
GL33CoreContext::draw_arrays(GLenum type, GLint first, GLsizei count)
{
  assert_gl();

  glDrawArrays(type, first, count);

  assert_gl();
}

/** mario stuff below */
typedef float VEC2[2];
typedef float VEC3[3];

GLuint
shader_compile( const char *shaderContents, size_t shaderContentsLength, GLenum shaderType )
{
  const GLchar *shaderDefine = shaderType == GL_VERTEX_SHADER 
    ? "\n#version 330 core\n#define VERTEX  \n#define v2f out\n" 
    : "\n#version 330 core\n#define FRAGMENT\n#define v2f in \n";

  const GLchar *shaderStrings[2] = { shaderDefine, shaderContents };
  GLint shaderStringLengths[2] = { (GLint)strlen( shaderDefine ), (GLint)shaderContentsLength };

  GLuint shader = glCreateShader( shaderType );
  glShaderSource( shader, 2, shaderStrings, shaderStringLengths );
  glCompileShader( shader );

  GLint isCompiled = 0;
  glGetShaderiv( shader, GL_COMPILE_STATUS, &isCompiled );
  if( isCompiled == GL_FALSE ) 
  {
    GLint maxLength = 0;
    glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &maxLength );
    char *log = (char*)malloc( maxLength );
    glGetShaderInfoLog( shader, maxLength, &maxLength, log );

	ConsoleBuffer::output << "Mario " << ((shaderType == GL_VERTEX_SHADER) ? "vertex" : "fragment") << " shader compilation failure: " << log << std::endl;
  }

  return shader;
}

void
GL33CoreContext::init_mario(uint8_t* raw_texture, uint32_t* texture, uint32_t* shader, const char* shader_code)
{
  assert_gl();

  GLuint vert = shader_compile(shader_code, strlen(shader_code), GL_VERTEX_SHADER);
  GLuint frag = shader_compile(shader_code, strlen(shader_code), GL_FRAGMENT_SHADER);

  *shader = glCreateProgram();
  glAttachShader(*shader, vert);
  glAttachShader(*shader, frag);

  const GLchar *attribs[] = {"position", "normal", "color", "uv"};
  for (int i=6; i<10; i++) glBindAttribLocation(*shader, i, attribs[i-6]);

  glLinkProgram(*shader);
  glDetachShader(*shader, vert);
  glDetachShader(*shader, frag);

  // initialize texture
  glGenTextures(1, texture);
  glBindTexture(GL_TEXTURE_2D, *texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SM64_TEXTURE_WIDTH, SM64_TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, raw_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void
GL33CoreContext::init_mario_instance(SM64MarioGeometryBuffers* geometry, MarioMesh* mesh)
{
  glGenVertexArrays( 1, &mesh->vao );
  glBindVertexArray( mesh->vao );

  #define X( loc, buff, arr, type ) do { \
    glGenBuffers( 1, &buff ); \
    glBindBuffer( GL_ARRAY_BUFFER, buff ); \
    glBufferData( GL_ARRAY_BUFFER, sizeof( type ) * 3 * SM64_GEO_MAX_TRIANGLES, arr, GL_DYNAMIC_DRAW ); \
    glEnableVertexAttribArray( loc ); \
    glVertexAttribPointer( loc, sizeof( type ) / sizeof( float ), GL_FLOAT, GL_FALSE, sizeof( type ), NULL ); \
  } while( 0 )

    X( 6, mesh->position_buffer, geometry->position,     VEC3 );
    X( 7, mesh->normal_buffer,   geometry->normal,       VEC3 );
    X( 8, mesh->color_buffer,    geometry->color,        VEC3 );
    X( 9, mesh->uv_buffer,       geometry->uv,           VEC2 );

  #undef X

  ConsoleBuffer::output << mesh->position_buffer << " " << mesh->normal_buffer << " " << mesh->color_buffer << " " << mesh->uv_buffer << " " << mesh->vao << std::endl;
}

void
GL33CoreContext::destroy_mario_instance(MarioMesh* mesh)
{
  glDeleteVertexArrays(1, &mesh->vao);
  glDeleteBuffers(1, &mesh->position_buffer);
  glDeleteBuffers(1, &mesh->normal_buffer);
  glDeleteBuffers(1, &mesh->color_buffer);
  glDeleteBuffers(1, &mesh->uv_buffer);
}

void
GL33CoreContext::render_mario_instance(const SM64MarioGeometryBuffers* geometry, const MarioMesh* mesh, const Vector& pos, const Vector& camera, const uint32_t cap, const uint32_t& texture, const uint32_t& shader, const uint16_t* indices)
{
  glBindBuffer(GL_ARRAY_BUFFER, mesh->position_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(VEC3) * 3 * SM64_GEO_MAX_TRIANGLES, geometry->position, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, mesh->normal_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(VEC3) * 3 * SM64_GEO_MAX_TRIANGLES, geometry->normal, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, mesh->color_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(VEC3) * 3 * SM64_GEO_MAX_TRIANGLES, geometry->color, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, mesh->uv_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(VEC2) * 3 * SM64_GEO_MAX_TRIANGLES, geometry->uv, GL_DYNAMIC_DRAW);

  const Viewport& viewport = m_video_system.get_viewport();
  const Rect& rect = viewport.get_rect();

  ConsoleBuffer::output << camera.x << " " << (rect.get_width() + camera.x) << " " << (rect.get_height() + camera.y) << " " << camera.y << std::endl;

  glm::mat4 projection = glm::ortho(camera.x, rect.get_width() - camera.x,
                         rect.get_height() - camera.y, -camera.y,
                         -1000.f, 10000.f);

  GLfloat view[16];//, projection[16];
  //glGetFloatv(GL_PROJECTION_MATRIX, projection);
  glGetFloatv(GL_MODELVIEW_MATRIX, view);

  //ConsoleBuffer::output << (boost::format("%f %f %f") % geometry->position[0] % geometry->position[1] % geometry->position[2]);
  //ConsoleBuffer::output << "proj ";
  //for (int i=0; i<16; i++) ConsoleBuffer::output << (boost::format("%f ") % projection[i]);
  //ConsoleBuffer::output << std::endl << "view ";
  //for (int i=0; i<16; i++) ConsoleBuffer::output << (boost::format("%f ") % view[i]);
  //ConsoleBuffer::output << "mvp ";
  //for (int i=0; i<9; i++) ConsoleBuffer::output << (boost::format("%f ") % curr_mvp_matrix[i]);
  //ConsoleBuffer::output << std::endl;

  // adjust farZ and nearZ in projection matrix to avoid mario model from getting clipped
  /*
  float nearZ = -1000.f, farZ = 10000.f;
  float fan = farZ + nearZ;
  float fsn = farZ - nearZ;
  projection[10] = -2.0f / fsn;
  projection[14] = -fan / fsn;
  */

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);

  glUseProgram(shader);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glBindVertexArray(mesh->vao);
  glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, (GLfloat*)view);
  glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, (GLfloat*)glm::value_ptr(projection));
  glUniformMatrix3fv(glGetUniformLocation(shader, "modelviewprojection"), 1, GL_FALSE, (GLfloat*)curr_mvp_matrix);
  glUniform1i(glGetUniformLocation(shader, "marioTex"), 0);

  uint32_t triangleSize = geometry->numTrianglesUsed*3;
  if (cap & MARIO_WING_CAP)
  {
    // skip the white rectangles from the wings (hacky solution because glBlend stuff does nothing)
    triangleSize = geometry->numTrianglesUsed*3-24;
    glUniform1i(glGetUniformLocation(shader, "wingCap"), 1);
    glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, indices + triangleSize);
  }
  glUniform1i(glGetUniformLocation(shader, "wingCap"), 0);
  glUniform1i(glGetUniformLocation(shader, "metalCap"), (cap & MARIO_METAL_CAP) ? 1 : 0);

  glDrawElements(GL_TRIANGLES, triangleSize, GL_UNSIGNED_SHORT, indices);
  glUseProgram(0);
  glBindVertexArray(0);

  glDisable(GL_DEPTH_TEST);

  bind();
}

/* EOF */
