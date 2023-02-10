// Super Mario 64 in SuperTux by Headshotnoby

#ifndef HEADER_SUPERTUX_MARIO_INSTANCE_HPP
#define HEADER_SUPERTUX_MARIO_INSTANCE_HPP

#define MAX_SURFACES 256
#define MAX_MOVINGOBJECTS 1024
#define MARIO_SCALE (50/100.f)

#include <stdint.h>
#include <limits.h>

extern "C" {
#include <libsm64.h>
#include <decomp/include/sm64shared.h>
}

#include "math/vector.hpp"
#include "object/tilemap.hpp"
#include "supertux/moving_object.hpp"

class Canvas;
class Player;

struct MarioMesh
{
  uint32_t position_buffer;
  uint32_t normal_buffer;
  uint32_t color_buffer;
  uint32_t uv_buffer;
  uint32_t vao;
};

struct MarioPathBlock
{
  MarioPathBlock() :
    ID(UINT_MAX),
    tilemap(nullptr)
  {}

  uint32_t ID;
  SM64ObjectTransform transform;
  TileMap *tilemap;
};

struct MarioMovingObject
{
  MarioMovingObject() :
    ID(UINT_MAX),
    obj(nullptr)
  {}

  uint32_t ID;
  SM64ObjectTransform transform;
  MovingObject* obj;
};

class MarioInstance
{
  Player* m_player;
  int mario_id;
  float tick;
  uint32_t loaded_surfaces[MAX_SURFACES];
  MarioPathBlock loaded_path_surfaces[MAX_SURFACES];
  MarioMovingObject loaded_movingobjects[MAX_MOVINGOBJECTS];
  MarioMesh mesh;
  int m_attacked;

  /** for mesh interpolation */
  Vector m_pos, m_last_pos, m_curr_pos;
  float m_last_geometry_pos[SM64_GEO_MAX_TRIANGLES * 9], m_curr_geometry_pos[SM64_GEO_MAX_TRIANGLES * 9];

  /** sm64 surface objects */
  bool add_block(int x, int y, int *i, TileMap* solids);
  void load_new_blocks(int x, int y);
  void load_all_movingobjects();
  void load_all_path_blocks();

public:
  MarioInstance(Player* player);
  ~MarioInstance();

  void spawn(float x, float y, bool loadCollision=true);
  void destroy();
  void update(float tickspeed);
  void draw(Canvas& canvas, Vector camera);

  void bounce(bool jump);
  void hurt(uint32_t damage, Vector& src);
  void kill(bool falling_sfx = false);
  void heal(uint8_t amount);
  void burn();

  void reload_collision();
  void delete_blocks();
  void delete_all_movingobjects();
  void delete_all_path_blocks();
  void set_pos(const Vector& pos);
  void set_velocity(const Vector& vel);

  bool dead() const { return state.health <= MARIO_DEAD_HEALTH; }
  bool spawned() const { return mario_id != -1; }
  bool attacked() const { return m_attacked; }
  int ID() const { return mario_id; }
  Vector get_pos() const { return m_pos; }

  SM64MarioState state;
  SM64MarioInputs input;
  SM64MarioGeometryBuffers geometry;
};

#endif
