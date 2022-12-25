// Super Mario 64 in SuperTux by Headshotnoby

#ifndef HEADER_SUPERTUX_MARIO_MANAGER_HPP
#define HEADER_SUPERTUX_MARIO_MANAGER_HPP

#include <stdint.h>

extern "C" {
#include <libsm64.h>
}

#include "mario/mario_instance.hpp"
#include "util/currenton.hpp"

class MarioManager final : public Currenton<MarioManager>
{
public:
  MarioManager();

  void init_mario(SM64MarioGeometryBuffers* geometry, MarioMesh* mesh);
  void destroy_mario(MarioMesh* mesh);
  //void render_mario(SM64MarioGeometryBuffers* geometry, MarioMesh* mesh, uint32_t cap);

  bool Loaded() const { return loaded; }
  uint16_t* get_indices() { return mario_indices; }
  uint32_t get_texture() { return mario_texture_handle; }
  uint32_t get_shader() { return mario_shader_handle; }

private:
  bool loaded;
  uint8_t *mario_texture;
  uint16_t mario_indices[SM64_GEO_MAX_TRIANGLES * 3];
  uint32_t mario_texture_handle;
  uint32_t mario_shader_handle;

private:
  MarioManager(const MarioManager&) = delete;
  MarioManager& operator=(const MarioManager&) = delete;
};

#endif
