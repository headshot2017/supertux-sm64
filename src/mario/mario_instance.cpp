// Super Mario 64 in SuperTux by Headshotnoby

#include "mario/mario_instance.hpp"

#include <stdint.h>
#include <string.h>

#define INT_SUBTYPE_BIG_KNOCKBACK 0x00000008
extern "C" {
#include <decomp/include/audio_defines.h>
#include <decomp/include/surface_terrains.h>
}

#include "badguy/badguy.hpp"
#include "badguy/stalactite.hpp"
#include "badguy/spiky.hpp"
#include "badguy/jumpy.hpp"
#include "badguy/igel.hpp"
#include "badguy/livefire.hpp"
#include "mario/mario_manager.hpp"
#include "math/aatriangle.hpp"
#include "math/rect.hpp"
#include "math/rectf.hpp"
#include "object/brick.hpp"
#include "object/camera.hpp"
#include "object/player.hpp"
#include "object/portable.hpp"
#include "object/sprite_particle.hpp"
#include "object/trampoline.hpp"
#include "supertux/console.hpp"
#include "supertux/sector.hpp"
#include "video/canvas.hpp"
#include "video/drawing_context.hpp"

struct Color888
{
	uint8_t r, g, b;
};

const Color888 defaultColors[] = {
	{0  , 0  , 255}, // Overalls
	{255, 0  , 0  }, // Shirt/Hat
	{254, 193, 121}, // Skin
	{115, 6  , 0  }, // Hair
	{255, 255, 255}, // Gloves
	{114, 28 , 14 }, // Shoes
};

Color888 bonusColors[6][6] = {
	{
		// no bonus
		{0,0,0},
		{0,0,0},
		{0,0,0},
		{0,0,0},
		{0,0,0},
		{0,0,0}
	},
	{
		// growup bonus
		{0,0,0},
		{0,0,0},
		{0,0,0},
		{0,0,0},
		{0,0,0},
		{0,0,0}
	},
	{
		// fire bonus
		{255, 0, 0},
		{255, 255, 255},
		{0,0,0},
		{0,0,0},
		{0,0,0},
		{0,0,0}
	},
	{
		// ice bonus
		{192, 0, 0},
		{0, 128, 255},
		{0,0,0},
		{0,0,0},
		{0,0,0},
		{0,0,0}
	},
	{
		// air bonus
		{0,0,0},
		{0,0,0},
		{0,0,0},
		{0,0,0},
		{0,0,0},
		{0,0,0}
	},
	{
		// earth bonus
		{0,0,0},
		{0,0,0},
		{0,0,0},
		{0,0,0},
		{0,0,0},
		{0,0,0}
	},
};


// functions to convert slopes into libsm64 triangle vertices for each face
void get_slope_triangle_up(int& x1, int& y1, int& x2, int& y2, int slope_info)
{
  switch(slope_info)
  {
    case AATriangle::NORTHWEST | AATriangle::DEFORM_RIGHT:
    case AATriangle::NORTHWEST | AATriangle::DEFORM_TOP:
    case AATriangle::NORTHWEST | AATriangle::DEFORM_BOTTOM:
    case AATriangle::NORTHEAST | AATriangle::DEFORM_BOTTOM:
    case AATriangle::NORTHEAST | AATriangle::DEFORM_TOP:
    case AATriangle::NORTHEAST | AATriangle::DEFORM_LEFT:
    case AATriangle::NORTHWEST:
    case AATriangle::NORTHEAST:
      x1 = 0;     y1 = 32;
      x2 = 32;    y2 = 32;
      break;

    case AATriangle::NORTHEAST | AATriangle::DEFORM_RIGHT:
    case AATriangle::SOUTHEAST | AATriangle::DEFORM_LEFT:
      x1 = 16;    y1 = 32;
      x2 = 32;    y2 = 32;
      break;

    case AATriangle::NORTHWEST | AATriangle::DEFORM_LEFT:
    case AATriangle::SOUTHWEST | AATriangle::DEFORM_RIGHT:
      x1 = 0;     y1 = 32;
      x2 = 16;    y2 = 32;
      break;

    case AATriangle::SOUTHWEST | AATriangle::DEFORM_TOP:
      x1 = 0;     y1 = 32;
      x2 = 32;    y2 = 16;
      break;

    case AATriangle::SOUTHWEST | AATriangle::DEFORM_BOTTOM:
      x1 = 0;     y1 = 16;
      x2 = 32;    y2 = 0;
      break;

    case AATriangle::SOUTHEAST | AATriangle::DEFORM_TOP:
      x1 = 0;     y1 = 16;
      x2 = 32;    y2 = 32;
      break;

    case AATriangle::SOUTHEAST | AATriangle::DEFORM_BOTTOM:
      x1 = 0;     y1 = 0;
      x2 = 32;    y2 = 16;
      break;

    case AATriangle::SOUTHWEST | AATriangle::DEFORM_LEFT:
      x1 = 0;     y1 = 32;
      x2 = 16;    y2 = 0;
      break;

    case AATriangle::SOUTHEAST | AATriangle::DEFORM_RIGHT:
      x1 = 16;     y1 = 0;
      x2 = 32;     y2 = 32;
      break;

    case AATriangle::SOUTHWEST:
      x1 = 0;     y1 = 32;
      x2 = 32;    y2 = 0;
      break;

    case AATriangle::SOUTHEAST:
      x1 = 0;     y1 = 0;
      x2 = 32;    y2 = 32;
      break;
  }
}

void get_slope_triangle_left(int& x1, int& y1, int& x2, int& y2, int slope_info)
{
  switch(slope_info)
  {
    case AATriangle::NORTHWEST | AATriangle::DEFORM_BOTTOM:
    case AATriangle::NORTHWEST | AATriangle::DEFORM_LEFT:
    case AATriangle::NORTHWEST | AATriangle::DEFORM_RIGHT:
    case AATriangle::SOUTHWEST | AATriangle::DEFORM_RIGHT:
    case AATriangle::SOUTHWEST | AATriangle::DEFORM_LEFT:
    case AATriangle::SOUTHWEST | AATriangle::DEFORM_TOP:
    case AATriangle::NORTHWEST:
    case AATriangle::SOUTHWEST:
      x1 = 0;     y1 = 0;
      x2 = 0;     y2 = 32;
      break;

    case AATriangle::SOUTHWEST | AATriangle::DEFORM_BOTTOM:
    case AATriangle::SOUTHEAST | AATriangle::DEFORM_TOP:
      x1 = 0;     y1 = 0;
      x2 = 0;     y2 = 16;
      break;

    case AATriangle::NORTHWEST | AATriangle::DEFORM_TOP:
    case AATriangle::NORTHEAST | AATriangle::DEFORM_BOTTOM:
      x1 = 0;     y1 = 16;
      x2 = 0;     y2 = 32;
      break;

    case AATriangle::SOUTHEAST | AATriangle::DEFORM_BOTTOM:
      x1 = 0;     y1 = 0;
      x2 = 32;    y2 = 16;
      break;

    case AATriangle::SOUTHEAST | AATriangle::DEFORM_RIGHT:
      x1 = 16;    y1 = 0;
      x2 = 32;    y2 = 32;
      break;

    case AATriangle::SOUTHEAST | AATriangle::DEFORM_LEFT:
      x1 = 0;     y1 = 0;
      x2 = 16;    y2 = 32;
      break;

    case AATriangle::NORTHEAST | AATriangle::DEFORM_LEFT:
      x1 = 16;    y1 = 0;
      x2 = 0;     y2 = 32;
      break;

    case AATriangle::NORTHEAST | AATriangle::DEFORM_RIGHT:
      x1 = 0;     y1 = 0;
      x2 = 0;     y2 = 0;
      break;

    case AATriangle::NORTHEAST | AATriangle::DEFORM_TOP:
      x1 = 0;     y1 = 0;
      x2 = 0;     y2 = 0;
      break;

    case AATriangle::SOUTHEAST:
      x1 = 0;     y1 = 0;
      x2 = 32;    y2 = 32;
      break;

    case AATriangle::NORTHEAST:
      x1 = 0;     y1 = 32;
      x2 = 32;    y2 = 0;
      break;
  }
}

void get_slope_triangle_right(int& x1, int& y1, int& x2, int& y2, int slope_info)
{
  switch(slope_info)
  {
    case AATriangle::SOUTHEAST | AATriangle::DEFORM_TOP:
    case AATriangle::SOUTHEAST | AATriangle::DEFORM_RIGHT:
    case AATriangle::SOUTHEAST | AATriangle::DEFORM_LEFT:
    case AATriangle::NORTHEAST | AATriangle::DEFORM_LEFT:
    case AATriangle::NORTHEAST | AATriangle::DEFORM_RIGHT:
    case AATriangle::NORTHEAST | AATriangle::DEFORM_BOTTOM:
    case AATriangle::SOUTHEAST:
    case AATriangle::NORTHEAST:
      x1 = 32;    y1 = 0;
      x2 = 32;    y2 = 32;
      break;

    case AATriangle::SOUTHEAST | AATriangle::DEFORM_BOTTOM:
    case AATriangle::SOUTHWEST | AATriangle::DEFORM_TOP:
      x1 = 32;    y1 = 0;
      x2 = 32;    y2 = 16;
      break;

    case AATriangle::NORTHEAST | AATriangle::DEFORM_TOP:
    case AATriangle::NORTHWEST | AATriangle::DEFORM_BOTTOM:
      x1 = 32;    y1 = 16;
      x2 = 32;    y2 = 32;
      break;

    case AATriangle::SOUTHWEST | AATriangle::DEFORM_BOTTOM:
      x1 = 0;     y1 = 16;
      x2 = 32;    y2 = 0;
      break;

    case AATriangle::SOUTHWEST | AATriangle::DEFORM_LEFT:
      x1 = 0;     y1 = 32;
      x2 = 16;    y2 = 0;
      break;

    case AATriangle::SOUTHWEST | AATriangle::DEFORM_RIGHT:
      x1 = 16;    y1 = 32;
      x2 = 32;    y2 = 0;
      break;

    case AATriangle::NORTHWEST | AATriangle::DEFORM_RIGHT:
      x1 = 16;    y1 = 0;
      x2 = 32;    y2 = 32;
      break;

    case AATriangle::NORTHWEST | AATriangle::DEFORM_LEFT:
      x1 = 0;     y1 = 0;
      x2 = 0;     y2 = 0;
      break;

    case AATriangle::NORTHWEST | AATriangle::DEFORM_TOP:
      x1 = 0;     y1 = 16;
      x2 = 32;    y2 = 32;
      break;

    case AATriangle::SOUTHWEST:
      x1 = 0;     y1 = 32;
      x2 = 32;    y2 = 0;
      break;

    case AATriangle::NORTHWEST:
      x1 = 0;     y1 = 0;
      x2 = 32;    y2 = 32;
      break;
  }
}

void get_slope_triangle_down(int& x1, int& y1, int& x2, int& y2, int slope_info)
{
  switch(slope_info)
  {
    case AATriangle::SOUTHEAST | AATriangle::DEFORM_LEFT:
    case AATriangle::SOUTHEAST | AATriangle::DEFORM_BOTTOM:
    case AATriangle::SOUTHEAST | AATriangle::DEFORM_TOP:
    case AATriangle::SOUTHWEST | AATriangle::DEFORM_TOP:
    case AATriangle::SOUTHWEST | AATriangle::DEFORM_BOTTOM:
    case AATriangle::SOUTHWEST | AATriangle::DEFORM_RIGHT:
    case AATriangle::SOUTHEAST:
    case AATriangle::SOUTHWEST:
      x1 = 0;    y1 = 0;
      x2 = 32;   y2 = 0;
      break;

    case AATriangle::SOUTHWEST | AATriangle::DEFORM_LEFT:
    case AATriangle::NORTHWEST | AATriangle::DEFORM_RIGHT:
      x1 = 0;    y1 = 0;
      x2 = 16;   y2 = 0;
      break;

    case AATriangle::SOUTHEAST | AATriangle::DEFORM_RIGHT:
    case AATriangle::NORTHEAST | AATriangle::DEFORM_LEFT:
      x1 = 16;    y1 = 0;
      x2 = 32;    y2 = 0;
      break;

    case AATriangle::NORTHEAST | AATriangle::DEFORM_RIGHT:
      x1 = 16;    y1 = 32;
      x2 = 32;    y2 = 0;
      break;

    case AATriangle::NORTHEAST | AATriangle::DEFORM_TOP:
      x1 = 0;     y1 = 32;
      x2 = 32;    y2 = 16;
      break;

    case AATriangle::NORTHEAST | AATriangle::DEFORM_BOTTOM:
      x1 = 0;     y1 = 16;
      x2 = 32;    y2 = 0;
      break;

    case AATriangle::NORTHWEST | AATriangle::DEFORM_BOTTOM:
      x1 = 0;     y1 = 0;
      x2 = 32;    y2 = 16;
      break;

    case AATriangle::NORTHWEST | AATriangle::DEFORM_TOP:
      x1 = 0;     y1 = 16;
      x2 = 32;    y2 = 32;
      break;

    case AATriangle::NORTHWEST | AATriangle::DEFORM_LEFT:
      x1 = 0;     y1 = 0;
      x2 = 16;    y2 = 32;
      break;

    case AATriangle::NORTHEAST:
      x1 = 0;     y1 = 32;
      x2 = 32;    y2 = 0;
      break;

    case AATriangle::NORTHWEST:
      x1 = 0;     y1 = 0;
      x2 = 32;    y2 = 32;
      break;
  }
}


MarioInstance::MarioInstance(Player* player) :
  m_player(player),
  mario_id(-1),
  m_attacked(0)
{
  memset(loaded_surfaces, UINT_MAX, sizeof(uint32_t) * MAX_SURFACES);
  memset(&input, 0, sizeof(input));
  memset(&mesh, 0, sizeof(mesh));
  state.health = MARIO_FULL_HEALTH;
}

MarioInstance::~MarioInstance()
{
  destroy();
}

void MarioInstance::spawn(float x, float y, bool loadCollision)
{
  destroy();
  tick = 0;
  m_attacked = 0;

  // on supertux, up coordinate is Y-, SM64 is Y+. flip the Y coordinate
  // scale conversions:
  //    supertux -> sm64: divide
  //    sm64 -> supertux: multiply
  int spawnX = x / MARIO_SCALE;
  int spawnY = -y / MARIO_SCALE;
  m_last_pos = m_curr_pos = m_pos = Vector(x, y);
  state.position[0] = spawnX;
  state.position[1] = spawnY;
  state.position[2] = 0;

  //exportMap(spawnX, spawnY);

  ConsoleBuffer::output << "Attempt to spawn Mario at " << x << " " << y << std::endl;

  if (loadCollision) load_new_blocks(x/32, y/32);
  mario_id = sm64_mario_create(spawnX, spawnY, 0);

  if (spawned())
  {
    ConsoleBuffer::output << "Mario spawned!" << std::endl;
    geometry.position = new float[9 * SM64_GEO_MAX_TRIANGLES];
    geometry.normal   = new float[9 * SM64_GEO_MAX_TRIANGLES];
    geometry.color    = new float[9 * SM64_GEO_MAX_TRIANGLES];
    geometry.uv       = new float[6 * SM64_GEO_MAX_TRIANGLES];
    geometry.numTrianglesUsed = 0;

    if (loadCollision)
    {
      load_all_movingobjects();
      load_all_path_blocks();
    }

    // no poison gas
    sm64_set_mario_gas_level(mario_id, -(Sector::get().get_height()+256)/MARIO_SCALE + 32);

    // load static surfaces: below the level, and left/right borders to prevent mario from escaping
    uint32_t surfaceCount = 6;
    SM64Surface surfaces[surfaceCount];

    for (uint32_t i=0; i<surfaceCount; i++)
    {
      surfaces[i].type = SURFACE_DEFAULT;
      surfaces[i].force = 0;
      surfaces[i].terrain = TERRAIN_STONE;
    }

    int width = Sector::get().get_width() / MARIO_SCALE;
    int spawnX = width/2;
    int spawnY = (Sector::get().get_height()+256) / -MARIO_SCALE;

    surfaces[0].vertices[0][0] = spawnX + width/2 + 128;	surfaces[0].vertices[0][1] = spawnY;	surfaces[0].vertices[0][2] = +128;
    surfaces[0].vertices[1][0] = spawnX - width/2 - 128;	surfaces[0].vertices[1][1] = spawnY;	surfaces[0].vertices[1][2] = -128;
    surfaces[0].vertices[2][0] = spawnX - width/2 - 128;	surfaces[0].vertices[2][1] = spawnY;	surfaces[0].vertices[2][2] = +128;

    surfaces[1].vertices[0][0] = spawnX - width/2 - 128;	surfaces[1].vertices[0][1] = spawnY;	surfaces[1].vertices[0][2] = -128;
    surfaces[1].vertices[1][0] = spawnX + width/2 + 128;	surfaces[1].vertices[1][1] = spawnY;	surfaces[1].vertices[1][2] = +128;
    surfaces[1].vertices[2][0] = spawnX + width/2 + 128;	surfaces[1].vertices[2][1] = spawnY;	surfaces[1].vertices[2][2] = -128;


    // left side
    int y1 = -Sector::get().get_height() / MARIO_SCALE, y2 = 0;
    surfaces[2].vertices[0][0] = 0;    surfaces[2].vertices[0][1] = y1 / MARIO_SCALE;    surfaces[2].vertices[0][2] = 64 / MARIO_SCALE;
    surfaces[2].vertices[1][0] = 0;    surfaces[2].vertices[1][1] = y2 / MARIO_SCALE;    surfaces[2].vertices[1][2] = -64 / MARIO_SCALE;
    surfaces[2].vertices[2][0] = 0;    surfaces[2].vertices[2][1] = y2 / MARIO_SCALE;    surfaces[2].vertices[2][2] = 64 / MARIO_SCALE;

    surfaces[3].vertices[0][0] = 0;    surfaces[3].vertices[0][1] = y2 / MARIO_SCALE;    surfaces[3].vertices[0][2] = -64 / MARIO_SCALE;
    surfaces[3].vertices[1][0] = 0;    surfaces[3].vertices[1][1] = y1 / MARIO_SCALE;    surfaces[3].vertices[1][2] = 64 / MARIO_SCALE;
    surfaces[3].vertices[2][0] = 0;    surfaces[3].vertices[2][1] = y1 / MARIO_SCALE;    surfaces[3].vertices[2][2] = -64 / MARIO_SCALE;


    // right side
    surfaceCount += 2;
    surfaces[4].vertices[0][0] = Sector::get().get_width() / MARIO_SCALE;    surfaces[4].vertices[0][1] = y1 / MARIO_SCALE;    surfaces[4].vertices[0][2] = -64 / MARIO_SCALE;
    surfaces[4].vertices[1][0] = Sector::get().get_width() / MARIO_SCALE;    surfaces[4].vertices[1][1] = y2 / MARIO_SCALE;    surfaces[4].vertices[1][2] = 64 / MARIO_SCALE;
    surfaces[4].vertices[2][0] = Sector::get().get_width() / MARIO_SCALE;    surfaces[4].vertices[2][1] = y2 / MARIO_SCALE;    surfaces[4].vertices[2][2] = -64 / MARIO_SCALE;

    surfaces[5].vertices[0][0] = Sector::get().get_width() / MARIO_SCALE;    surfaces[5].vertices[0][1] = y2 / MARIO_SCALE;    surfaces[5].vertices[0][2] = 64 / MARIO_SCALE;
    surfaces[5].vertices[1][0] = Sector::get().get_width() / MARIO_SCALE;    surfaces[5].vertices[1][1] = y1 / MARIO_SCALE;    surfaces[5].vertices[1][2] = -64 / MARIO_SCALE;
    surfaces[5].vertices[2][0] = Sector::get().get_width() / MARIO_SCALE;    surfaces[5].vertices[2][1] = y1 / MARIO_SCALE;    surfaces[5].vertices[2][2] = 64 / MARIO_SCALE;

    sm64_static_surfaces_load(surfaces, surfaceCount);

    return;
  }

  ConsoleBuffer::output << "Failed to spawn Mario" << std::endl;
}

void MarioInstance::destroy()
{
  if (spawned())
  {
    delete_blocks();
    delete_all_movingobjects();
    delete_all_path_blocks();

    sm64_mario_delete(mario_id);
    mario_id = -1;

    delete[] geometry.position; geometry.position = 0;
    delete[] geometry.normal; geometry.normal = 0;
    delete[] geometry.color; geometry.color = 0;
    delete[] geometry.uv; geometry.uv = 0;
  }
}

void MarioInstance::update(float tickspeed)
{
  if (!spawned())
    return;

  tick += tickspeed;
  while (tick >= 1.f/30)
  {
    tick -= 1.f/30;

    if (m_attacked) m_attacked--;

    // handle path blocks (solid tilemaps with a moving path, like the platforms in The Crystal Mine)
    for (int i=0; i<MAX_SURFACES; i++)
    {
      MarioPathBlock* sm64obj = &loaded_path_surfaces[i];
      if (sm64obj->ID == UINT_MAX) continue;

      float x = sm64obj->tilemap->get_offset().x / MARIO_SCALE;
      float y = -sm64obj->tilemap->get_offset().y / MARIO_SCALE;
      if (sm64obj->transform.position[0] != x || sm64obj->transform.position[1] != y)
      {
        sm64obj->transform.position[0] = x;
        sm64obj->transform.position[1] = y;
        sm64_surface_object_move(sm64obj->ID, &sm64obj->transform);
      }
    }

    // handle MovingObjects
    for (int i=0; i<MAX_MOVINGOBJECTS; i++)
    {
      MarioMovingObject* sm64obj = &loaded_movingobjects[i];
      if (sm64obj->ID == UINT_MAX || !sm64obj->obj) continue;

      if (!sm64obj->obj->is_valid() || !sm64obj->obj->get_uid())
      {
        sm64obj->obj = nullptr;
        sm64obj->transform.position[2] = 256;
        sm64_surface_object_move(sm64obj->ID, &sm64obj->transform);
        continue;
      }

      Brick* brick = dynamic_cast<Brick*>(sm64obj->obj);
      Portable* portable = dynamic_cast<Portable*>(sm64obj->obj);

      if (sm64obj->obj->get_group() == COLGROUP_DISABLED || (portable && portable->is_grabbed()) || (brick && state.action == ACT_GROUND_POUND))
        sm64obj->transform.position[2] = 256; // make it out of reach
      else
      {
        Vector pos = sm64obj->obj->get_pos();
        Vector bbox = sm64obj->obj->get_bbox().get_size().as_vector();
        sm64obj->transform.position[0] = pos.x / MARIO_SCALE;
        sm64obj->transform.position[1] = (-pos.y-16 - (bbox.y-32)) / MARIO_SCALE;
        sm64obj->transform.position[2] = 0;
      }
      sm64_surface_object_move(sm64obj->ID, &sm64obj->transform);
    }

    // water
    int yadd = 0;
    int waterY = (Sector::get().get_height()+256)/32;
    for (const auto& solids : Sector::get().get_solid_tilemaps())
    {
      bool get_out = false;

      while (m_curr_pos.y/32 - yadd < waterY && m_curr_pos.y - yadd*32 >= solids->get_offset().y && solids->get_tile(m_curr_pos.x/32, m_curr_pos.y/32 - yadd).get_attributes() & Tile::WATER)
      {
        waterY = m_curr_pos.y/32 - yadd;
        yadd++;
        get_out = true;
      }

      if (get_out)
        break;
    }
    sm64_set_mario_water_level(mario_id, -waterY*32/MARIO_SCALE + 32);

    // attack enemies with punches, kicks or dives
    if (state.action & ACT_FLAG_ATTACKING)
    {
      for (MovingObject* obj : Sector::get().get_nearby_objects(m_curr_pos, 40))
      {
        BadGuy* enemy = dynamic_cast<BadGuy*>(obj);
        if (!enemy) continue;

        // do not let mario attack these objects
        Stalactite* ignore1 = dynamic_cast<Stalactite*>(enemy);
        LiveFire* ignore2 = dynamic_cast<LiveFire*>(enemy);
        if (ignore1 || ignore2) continue;

        // do not let mario jump on these enemies' heads
        // ugly solution but it'll do...
        Spiky* ignore3 = dynamic_cast<Spiky*>(enemy);
        Jumpy* ignore4 = dynamic_cast<Jumpy*>(enemy);
        Igel* ignore5 = dynamic_cast<Igel*>(enemy);
        if ((ignore3 || ignore4 || ignore5) &&
            !(state.action & ACT_FLAG_BUTT_OR_STOMACH_SLIDE) && state.action != ACT_DIVE_SLIDE && state.action != ACT_PUNCHING &&
              state.action != ACT_MOVE_PUNCHING && state.action != ACT_CROUCH_SLIDE && state.action != ACT_SLIDE_KICK_SLIDE &&
              state.action != ACT_DIVE && state.action != ACT_SLIDE_KICK && state.action != ACT_JUMP_KICK)
          continue;

        if (sm64_mario_attack(mario_id, enemy->get_pos().x / MARIO_SCALE, enemy->get_pos().y / -MARIO_SCALE, 0, 0))
          enemy->kill_fall();
      }
    }

    // particles
	if (state.particleFlags & PARTICLE_FIRE)
    {
      Vector ppos = get_pos() + Vector(0, -16);
      Vector pspeed = Vector(0, -50);
      Vector paccel = Vector(0);
      Sector::get().add<SpriteParticle>("images/particles/smoke.sprite",
                                         "default", ppos, ANCHOR_MIDDLE,
                                         pspeed, paccel,
                                         LAYER_BACKGROUNDTILES+2);
    }

    m_last_pos = m_curr_pos;
    memcpy(m_last_geometry_pos, m_curr_geometry_pos, sizeof(m_curr_geometry_pos));

    sm64_reset_mario_z(mario_id);
    if (state.action & ACT_FLAG_SWIMMING_OR_FLYING) input.stickX *= -1;
    sm64_mario_tick(mario_id, &input, &state, &geometry);

    Vector new_pos(state.position[0]*MARIO_SCALE, -state.position[1]*MARIO_SCALE);
    if ((int)(new_pos.x/32) != (int)(m_pos.x/32) || (int)(new_pos.y/32) != (int)(m_pos.y/32))
      load_new_blocks(new_pos.x/32, new_pos.y/32);

    new_pos.y += 16;
    m_curr_pos = new_pos;

    BonusType bonus = m_player->get_status().bonus;
    for (int i=0; i<geometry.numTrianglesUsed*3; i++)
    {
      geometry.normal[i*3+1] *= -1;

      m_curr_geometry_pos[i*3+0] = geometry.position[i*3+0]*MARIO_SCALE;
      m_curr_geometry_pos[i*3+1] = geometry.position[i*3+1]*-MARIO_SCALE + 16;
      m_curr_geometry_pos[i*3+2] = geometry.position[i*3+2]*MARIO_SCALE;

      uint8_t r = geometry.color[i*3+0]*255;
      uint8_t g = geometry.color[i*3+1]*255;
      uint8_t b = geometry.color[i*3+2]*255;
      for (int c = 0; c < 6; c++) {
        if (r == defaultColors[c].r && g == defaultColors[c].g && b == defaultColors[c].b &&
           (bonusColors[bonus][c].r || bonusColors[bonus][c].g || bonusColors[bonus][c].b)) {
          geometry.color[i*3+0] = bonusColors[bonus][c].r/255.f;
          geometry.color[i*3+1] = bonusColors[bonus][c].g/255.f;
          geometry.color[i*3+2] = bonusColors[bonus][c].b/255.f;
          break;
        }
      }
    }
  }

  m_pos = mix(m_last_pos, m_curr_pos, tick / (1.f/30));
  for (int i=0; i<geometry.numTrianglesUsed*3; i++)
  {
    geometry.position[i*3+0] = m_last_geometry_pos[i*3+0] + (m_curr_geometry_pos[i*3+0] - m_last_geometry_pos[i*3+0]) * (tick / (1.f/30));
    geometry.position[i*3+1] = m_last_geometry_pos[i*3+1] + (m_curr_geometry_pos[i*3+1] - m_last_geometry_pos[i*3+1]) * (tick / (1.f/30));
    geometry.position[i*3+2] = m_last_geometry_pos[i*3+2] + (m_curr_geometry_pos[i*3+2] - m_last_geometry_pos[i*3+2]) * (tick / (1.f/30));
  }
}

void MarioInstance::draw(Canvas& canvas, Vector camera)
{
  if (!geometry.numTrianglesUsed || !spawned()) return;
  auto mariomanager = MarioManager::current();

  DrawingContext& context = canvas.get_context();
  context.push_transform();

  canvas.draw_mario(&geometry,
                    m_pos,
                    camera,
                    state.flags,
                    mariomanager->mario_texture_handle,
                    mariomanager->mario_indices);

  context.pop_transform();
}

void MarioInstance::bounce(bool jump)
{
  if (!spawned()) return;
  state.velocity[1] = 10; // for badguy squish check
  m_attacked = 2;
  sm64_mario_attack(mario_id, state.position[0], state.position[1]-8, state.position[2], 0);

  if (state.action == ACT_GROUND_POUND)
  {
    sm64_set_mario_action(mario_id, ACT_TRIPLE_JUMP);
    sm64_play_sound_global(SOUND_ACTION_HIT);
  }

  if (jump && state.action != ACT_GROUND_POUND && !(state.action & ACT_FLAG_INVULNERABLE))
    sm64_set_mario_velocity(mario_id, state.velocity[0], 50, 0);
}

void MarioInstance::hurt(uint32_t damage, Vector& src)
{
  if (!spawned() || state.action & ACT_FLAG_INVULNERABLE || m_attacked) return;

  PlayerStatus& status = m_player->get_status();
  if (status.bonus == FIRE_BONUS
    || status.bonus == ICE_BONUS
    || status.bonus == AIR_BONUS
    || status.bonus == EARTH_BONUS) {
    m_player->set_bonus(GROWUP_BONUS, true);
  }

  uint32_t subtype = (damage >= 3) ? INT_SUBTYPE_BIG_KNOCKBACK : 0;
  sm64_mario_take_damage(mario_id, damage, subtype, src.x/MARIO_SCALE, src.y/MARIO_SCALE, 0);
}

void MarioInstance::kill(bool falling_sfx)
{
  if (dead()) return;
  if (falling_sfx)
  {
    sm64_play_sound_global(SOUND_MARIO_WAAAOOOW);
	destroy();
  }
  else
    sm64_mario_kill(mario_id);

  state.health = MARIO_DEAD_HEALTH;
}

void MarioInstance::heal(uint8_t amount)
{
  if (!spawned()) return;
  sm64_mario_heal(mario_id, amount);
}

void MarioInstance::burn()
{
  if (!spawned() || state.action & ACT_FLAG_INVULNERABLE || m_attacked) return;

  PlayerStatus& status = m_player->get_status();
  if (status.bonus == FIRE_BONUS
    || status.bonus == ICE_BONUS
    || status.bonus == AIR_BONUS
    || status.bonus == EARTH_BONUS) {
    m_player->set_bonus(GROWUP_BONUS, true);
  }

  m_player->ungrab_object();
  sm64_set_mario_action_arg(mario_id, ACT_BURNING_JUMP, 1);
  sm64_play_sound_global(SOUND_MARIO_ON_FIRE);
}

void MarioInstance::reload_collision()
{
  if (!spawned()) return;

  delete_blocks();
  delete_all_movingobjects();
  delete_all_path_blocks();

  load_new_blocks(m_pos.x/32, m_pos.y/32);
  load_all_movingobjects();
  load_all_path_blocks();
}

void MarioInstance::set_pos(const Vector& pos)
{
  if (!spawned() || pos.x <= 0 || pos.y <= 0 || pos.x >= Sector::get().get_width() || pos.y >= Sector::get().get_height())
    return;

  sm64_set_mario_position(mario_id, pos.x/MARIO_SCALE, -pos.y/MARIO_SCALE, 0);
}

void MarioInstance::set_velocity(const Vector& vel)
{
  if (!spawned())
    return;

  sm64_set_mario_velocity(mario_id, vel.x/MARIO_SCALE, -vel.y/MARIO_SCALE, 0);
  state.velocity[0] = vel.x/MARIO_SCALE;
  state.velocity[1] = -vel.y/MARIO_SCALE;
}

void MarioInstance::delete_all_movingobjects()
{
  for (int i=0; i<MAX_MOVINGOBJECTS; i++)
  {
    if (loaded_movingobjects[i].ID == UINT_MAX) continue;
    sm64_surface_object_delete(loaded_movingobjects[i].ID);
    loaded_movingobjects[i].ID = UINT_MAX;
    loaded_movingobjects[i].obj = nullptr;
  }
}

void MarioInstance::load_all_movingobjects()
{
  delete_all_movingobjects();

  for (auto& object_ptr : Sector::get().get_objects())
  {
    MovingObject* object = dynamic_cast<MovingObject*>(object_ptr.get());
    Trampoline* ignore1 = dynamic_cast<Trampoline*>(object);
    if (!object || ignore1 || (object->get_group() != COLGROUP_STATIC && object->get_group() != COLGROUP_MOVING_STATIC))
      continue;

    Vector pos = object->get_pos();
    Vector bbox = object->get_bbox().get_size().as_vector();

    SM64SurfaceObject sm64obj;
    memset(&sm64obj.transform, 0, sizeof(struct SM64ObjectTransform));
    sm64obj.transform.position[0] = pos.x / MARIO_SCALE;
    sm64obj.transform.position[1] = (-pos.y-16 - (bbox.y-32)) / MARIO_SCALE;
    sm64obj.transform.position[2] = 0;
    sm64obj.surfaceCount = 4*2;
    sm64obj.surfaces = (struct SM64Surface*)malloc(sizeof(struct SM64Surface) * sm64obj.surfaceCount);

    // block ground face
    sm64obj.surfaces[0].vertices[0][0] = bbox.x / MARIO_SCALE;	sm64obj.surfaces[0].vertices[0][1] = bbox.y / MARIO_SCALE;	sm64obj.surfaces[0].vertices[0][2] = 64 / MARIO_SCALE;
    sm64obj.surfaces[0].vertices[1][0] = 0 / MARIO_SCALE;		sm64obj.surfaces[0].vertices[1][1] = bbox.y / MARIO_SCALE;	sm64obj.surfaces[0].vertices[1][2] = -64 / MARIO_SCALE;
    sm64obj.surfaces[0].vertices[2][0] = 0 / MARIO_SCALE;		sm64obj.surfaces[0].vertices[2][1] = bbox.y / MARIO_SCALE;	sm64obj.surfaces[0].vertices[2][2] = 64 / MARIO_SCALE;

    sm64obj.surfaces[1].vertices[0][0] = 0 / MARIO_SCALE; 		sm64obj.surfaces[1].vertices[0][1] = bbox.y / MARIO_SCALE;	sm64obj.surfaces[1].vertices[0][2] = -64 / MARIO_SCALE;
    sm64obj.surfaces[1].vertices[1][0] = bbox.x / MARIO_SCALE;	sm64obj.surfaces[1].vertices[1][1] = bbox.y / MARIO_SCALE;	sm64obj.surfaces[1].vertices[1][2] = 64 / MARIO_SCALE;
    sm64obj.surfaces[1].vertices[2][0] = bbox.x / MARIO_SCALE;	sm64obj.surfaces[1].vertices[2][1] = bbox.y / MARIO_SCALE;	sm64obj.surfaces[1].vertices[2][2] = -64 / MARIO_SCALE;

	// left (Z+)
    sm64obj.surfaces[2].vertices[0][2] = -64 / MARIO_SCALE;		sm64obj.surfaces[2].vertices[0][1] = 0 / MARIO_SCALE;		sm64obj.surfaces[2].vertices[0][0] = 0 / MARIO_SCALE;
    sm64obj.surfaces[2].vertices[1][2] = 64 / MARIO_SCALE;		sm64obj.surfaces[2].vertices[1][1] = bbox.y / MARIO_SCALE;	sm64obj.surfaces[2].vertices[1][0] = 0 / MARIO_SCALE;
    sm64obj.surfaces[2].vertices[2][2] = -64 / MARIO_SCALE;		sm64obj.surfaces[2].vertices[2][1] = bbox.y / MARIO_SCALE;	sm64obj.surfaces[2].vertices[2][0] = 0 / MARIO_SCALE;

    sm64obj.surfaces[3].vertices[0][2] = 64 / MARIO_SCALE;		sm64obj.surfaces[3].vertices[0][1] = bbox.y / MARIO_SCALE;	sm64obj.surfaces[3].vertices[0][0] = 0 / MARIO_SCALE;
    sm64obj.surfaces[3].vertices[1][2] = -64 / MARIO_SCALE;		sm64obj.surfaces[3].vertices[1][1] = 0 / MARIO_SCALE;		sm64obj.surfaces[3].vertices[1][0] = 0 / MARIO_SCALE;
    sm64obj.surfaces[3].vertices[2][2] = 64 / MARIO_SCALE;		sm64obj.surfaces[3].vertices[2][1] = 0 / MARIO_SCALE;		sm64obj.surfaces[3].vertices[2][0] = 0 / MARIO_SCALE;

	// right (Z-)
    sm64obj.surfaces[4].vertices[0][2] = 64 / MARIO_SCALE;		sm64obj.surfaces[4].vertices[0][1] = 0 / MARIO_SCALE;		sm64obj.surfaces[4].vertices[0][0] = bbox.x / MARIO_SCALE;
    sm64obj.surfaces[4].vertices[1][2] = -64 / MARIO_SCALE;		sm64obj.surfaces[4].vertices[1][1] = bbox.y / MARIO_SCALE;	sm64obj.surfaces[4].vertices[1][0] = bbox.x / MARIO_SCALE;
    sm64obj.surfaces[4].vertices[2][2] = 64 / MARIO_SCALE;		sm64obj.surfaces[4].vertices[2][1] = bbox.y / MARIO_SCALE;	sm64obj.surfaces[4].vertices[2][0] = bbox.x / MARIO_SCALE;

    sm64obj.surfaces[5].vertices[0][2] = -64 / MARIO_SCALE;		sm64obj.surfaces[5].vertices[0][1] = bbox.y / MARIO_SCALE;	sm64obj.surfaces[5].vertices[0][0] = bbox.x / MARIO_SCALE;
    sm64obj.surfaces[5].vertices[1][2] = 64 / MARIO_SCALE;		sm64obj.surfaces[5].vertices[1][1] = 0 / MARIO_SCALE;		sm64obj.surfaces[5].vertices[1][0] = bbox.x / MARIO_SCALE;
    sm64obj.surfaces[5].vertices[2][2] = -64 / MARIO_SCALE;		sm64obj.surfaces[5].vertices[2][1] = 0 / MARIO_SCALE;		sm64obj.surfaces[5].vertices[2][0] = bbox.x / MARIO_SCALE;

	// block bottom face
    sm64obj.surfaces[6].vertices[0][0] = 0 / MARIO_SCALE;		sm64obj.surfaces[6].vertices[0][1] = 0 / MARIO_SCALE;		sm64obj.surfaces[6].vertices[0][2] = 64 / MARIO_SCALE;
    sm64obj.surfaces[6].vertices[1][0] = 0 / MARIO_SCALE;		sm64obj.surfaces[6].vertices[1][1] = 0 / MARIO_SCALE;		sm64obj.surfaces[6].vertices[1][2] = -64 / MARIO_SCALE;
    sm64obj.surfaces[6].vertices[2][0] = bbox.x / MARIO_SCALE;	sm64obj.surfaces[6].vertices[2][1] = 0 / MARIO_SCALE;		sm64obj.surfaces[6].vertices[2][2] = 64 / MARIO_SCALE;

    sm64obj.surfaces[7].vertices[0][0] = bbox.x / MARIO_SCALE;	sm64obj.surfaces[7].vertices[0][1] = 0 / MARIO_SCALE;		sm64obj.surfaces[7].vertices[0][2] = -64 / MARIO_SCALE;
    sm64obj.surfaces[7].vertices[1][0] = bbox.x / MARIO_SCALE;	sm64obj.surfaces[7].vertices[1][1] = 0 / MARIO_SCALE;		sm64obj.surfaces[7].vertices[1][2] = 64 / MARIO_SCALE;
    sm64obj.surfaces[7].vertices[2][0] = 0 / MARIO_SCALE;		sm64obj.surfaces[7].vertices[2][1] = 0 / MARIO_SCALE;		sm64obj.surfaces[7].vertices[2][2] = -64 / MARIO_SCALE;

    for (uint32_t ind=0; ind<sm64obj.surfaceCount; ind++)
    {
      sm64obj.surfaces[ind].type = SURFACE_DEFAULT;
      sm64obj.surfaces[ind].force = 0;
      sm64obj.surfaces[ind].terrain = TERRAIN_STONE;
    }

    for (int ind=0; ind<MAX_MOVINGOBJECTS; ind++)
    {
      if (loaded_movingobjects[ind].ID != UINT_MAX) continue;

      loaded_movingobjects[ind].ID = sm64_surface_object_create(&sm64obj);
      loaded_movingobjects[ind].obj = object;
      loaded_movingobjects[ind].transform = sm64obj.transform;
      break;
    }

    free(sm64obj.surfaces);
  }
}

void MarioInstance::delete_all_path_blocks()
{
  for (int i=0; i<MAX_SURFACES; i++)
  {
    if (loaded_path_surfaces[i].ID == UINT_MAX) continue;
    sm64_surface_object_delete(loaded_path_surfaces[i].ID);
    loaded_path_surfaces[i].ID = UINT_MAX;
    loaded_path_surfaces[i].tilemap = nullptr;
  }
}

void MarioInstance::load_all_path_blocks()
{
  delete_all_path_blocks();

  for (const auto& solids : Sector::get().get_solid_tilemaps())
  {
    if (!solids->get_path()) continue;

    struct SM64SurfaceObject obj;
    memset(&obj.transform, 0, sizeof(struct SM64ObjectTransform));
    obj.transform.position[0] = solids->get_offset().x / MARIO_SCALE;
    obj.transform.position[1] = -solids->get_offset().y / MARIO_SCALE;
	obj.surfaceCount = 0;
	obj.surfaces = nullptr;

    for (int x=0; x<solids->get_width(); x++)
    {
      for (int y=0; y<solids->get_height(); y++)
      {
        const Tile& block = solids->get_tile(x, y);
        if (!block.is_solid()) continue;

        bool up =      y-1 >= 0                   && solids->get_tile(x, y-1).is_solid();
        bool down =    y+1 < solids->get_height() && (solids->get_tile(x, y+1).is_solid() || block.is_unisolid());
        bool left =    x-1 >= 0                   && (solids->get_tile(x-1, y).is_solid() || block.is_unisolid());
        bool right =   x+1 < solids->get_width()  && (solids->get_tile(x+1, y).is_solid() || block.is_unisolid());
        if (up && down && left && right) continue; // at least one side must be free

        int X = (solids->get_offset().x + (x*32 - solids->get_offset().x)) / MARIO_SCALE;
        int Y = -(solids->get_offset().y + (y*32+16 - solids->get_offset().y)) / MARIO_SCALE;

        // block ground face
        if (!up)
        {
          obj.surfaceCount += 2;
          obj.surfaces = (struct SM64Surface*)realloc(obj.surfaces, sizeof(struct SM64Surface) * obj.surfaceCount);

          obj.surfaces[obj.surfaceCount-2].vertices[0][0] = X + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-2].vertices[0][1] = Y + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-2].vertices[0][2] = 64 / MARIO_SCALE;
          obj.surfaces[obj.surfaceCount-2].vertices[1][0] = X;					obj.surfaces[obj.surfaceCount-2].vertices[1][1] = Y + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-2].vertices[1][2] = -64 / MARIO_SCALE;
          obj.surfaces[obj.surfaceCount-2].vertices[2][0] = X;					obj.surfaces[obj.surfaceCount-2].vertices[2][1] = Y + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-2].vertices[2][2] = 64 / MARIO_SCALE;

          obj.surfaces[obj.surfaceCount-1].vertices[0][0] = X; 					obj.surfaces[obj.surfaceCount-1].vertices[0][1] = Y + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-1].vertices[0][2] = -64 / MARIO_SCALE;
          obj.surfaces[obj.surfaceCount-1].vertices[1][0] = X + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-1].vertices[1][1] = Y + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-1].vertices[1][2] = 64 / MARIO_SCALE;
          obj.surfaces[obj.surfaceCount-1].vertices[2][0] = X + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-1].vertices[2][1] = Y + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-1].vertices[2][2] = -64 / MARIO_SCALE;
        }

        // left (Z+)
        if (!left)
        {
          obj.surfaceCount += 2;
          obj.surfaces = (struct SM64Surface*)realloc(obj.surfaces, sizeof(struct SM64Surface) * obj.surfaceCount);

          obj.surfaces[obj.surfaceCount-2].vertices[0][2] = -64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount-2].vertices[0][1] = Y;					obj.surfaces[obj.surfaceCount-2].vertices[0][0] = X;
          obj.surfaces[obj.surfaceCount-2].vertices[1][2] = 64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount-2].vertices[1][1] = Y + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-2].vertices[1][0] = X;
          obj.surfaces[obj.surfaceCount-2].vertices[2][2] = -64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount-2].vertices[2][1] = Y + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-2].vertices[2][0] = X;

          obj.surfaces[obj.surfaceCount-1].vertices[0][2] = 64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount-1].vertices[0][1] = Y + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-1].vertices[0][0] = X;
          obj.surfaces[obj.surfaceCount-1].vertices[1][2] = -64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount-1].vertices[1][1] = Y;					obj.surfaces[obj.surfaceCount-1].vertices[1][0] = X;
          obj.surfaces[obj.surfaceCount-1].vertices[2][2] = 64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount-1].vertices[2][1] = Y;					obj.surfaces[obj.surfaceCount-1].vertices[2][0] = X;
        }

        // right (Z-)
        if (!right)
        {
          obj.surfaceCount += 2;
          obj.surfaces = (struct SM64Surface*)realloc(obj.surfaces, sizeof(struct SM64Surface) * obj.surfaceCount);

          obj.surfaces[obj.surfaceCount-2].vertices[0][2] = 64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount-2].vertices[0][1] = Y;					obj.surfaces[obj.surfaceCount-2].vertices[0][0] = X + 32/MARIO_SCALE;
          obj.surfaces[obj.surfaceCount-2].vertices[1][2] = -64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount-2].vertices[1][1] = Y + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-2].vertices[1][0] = X + 32/MARIO_SCALE;
          obj.surfaces[obj.surfaceCount-2].vertices[2][2] = 64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount-2].vertices[2][1] = Y + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-2].vertices[2][0] = X + 32/MARIO_SCALE;

          obj.surfaces[obj.surfaceCount-1].vertices[0][2] = -64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount-1].vertices[0][1] = Y + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-1].vertices[0][0] = X + 32/MARIO_SCALE;
          obj.surfaces[obj.surfaceCount-1].vertices[1][2] = 64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount-1].vertices[1][1] = Y;					obj.surfaces[obj.surfaceCount-1].vertices[1][0] = X + 32/MARIO_SCALE;
          obj.surfaces[obj.surfaceCount-1].vertices[2][2] = -64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount-1].vertices[2][1] = Y;					obj.surfaces[obj.surfaceCount-1].vertices[2][0] = X + 32/MARIO_SCALE;
        }

        // block bottom face
        if (!down)
        {
          obj.surfaceCount += 2;
          obj.surfaces = (struct SM64Surface*)realloc(obj.surfaces, sizeof(struct SM64Surface) * obj.surfaceCount);

          obj.surfaces[obj.surfaceCount-2].vertices[0][0] = X;					obj.surfaces[obj.surfaceCount-2].vertices[0][1] = Y;	obj.surfaces[obj.surfaceCount-2].vertices[0][2] = 64 / MARIO_SCALE;
          obj.surfaces[obj.surfaceCount-2].vertices[1][0] = X;					obj.surfaces[obj.surfaceCount-2].vertices[1][1] = Y;	obj.surfaces[obj.surfaceCount-2].vertices[1][2] = -64 / MARIO_SCALE;
          obj.surfaces[obj.surfaceCount-2].vertices[2][0] = X + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-2].vertices[2][1] = Y;	obj.surfaces[obj.surfaceCount-2].vertices[2][2] = 64 / MARIO_SCALE;

          obj.surfaces[obj.surfaceCount-1].vertices[0][0] = X + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-1].vertices[0][1] = Y;	obj.surfaces[obj.surfaceCount-1].vertices[0][2] = -64 / MARIO_SCALE;
          obj.surfaces[obj.surfaceCount-1].vertices[1][0] = X + 32/MARIO_SCALE;	obj.surfaces[obj.surfaceCount-1].vertices[1][1] = Y;	obj.surfaces[obj.surfaceCount-1].vertices[1][2] = 64 / MARIO_SCALE;
          obj.surfaces[obj.surfaceCount-1].vertices[2][0] = X;					obj.surfaces[obj.surfaceCount-1].vertices[2][1] = Y;	obj.surfaces[obj.surfaceCount-1].vertices[2][2] = -64 / MARIO_SCALE;
        }
      }
    }

    for (uint32_t ind=0; ind<obj.surfaceCount; ind++)
    {
      obj.surfaces[ind].type = SURFACE_DEFAULT;
      obj.surfaces[ind].force = 0;
      obj.surfaces[ind].terrain = TERRAIN_STONE;
    }

    for (int ind=0; ind<MAX_SURFACES; ind++)
    {
      if (loaded_path_surfaces[ind].ID != UINT_MAX) continue;

      loaded_path_surfaces[ind].ID = sm64_surface_object_create(&obj);
      loaded_path_surfaces[ind].transform = obj.transform;
      loaded_path_surfaces[ind].tilemap = solids;
      break;
    }

    if (obj.surfaces) free(obj.surfaces);
  }
}

void MarioInstance::delete_blocks()
{
  for (int i=0; i<MAX_SURFACES; i++)
  {
    if (loaded_surfaces[i] == UINT_MAX) continue;
    sm64_surface_object_delete(loaded_surfaces[i]);
    loaded_surfaces[i] = UINT_MAX;
  }
}

bool MarioInstance::add_block(int x, int y, int *i, TileMap* solids)
{
  if ((*i) >= MAX_SURFACES || solids->is_outside_bounds(Vector(x*32, y*32))) return false;
  int offsetX = solids->get_offset().x/32;
  int offsetY = solids->get_offset().y/32;
  const Tile& block = solids->get_tile(x - offsetX, y - offsetY);
  bool lava = (block.get_attributes() & Tile::WATER) && (block.get_attributes() & Tile::HURTS) && (block.get_attributes() & Tile::FIRE);
  if (!block.is_solid() && !lava) return false;

  struct SM64SurfaceObject obj;
  memset(&obj.transform, 0, sizeof(struct SM64ObjectTransform));
  obj.transform.position[0] = (solids->get_offset().x + (x*32 - solids->get_offset().x)) / MARIO_SCALE;
  obj.transform.position[1] = -(solids->get_offset().y + (y*32+16 - solids->get_offset().y)) / MARIO_SCALE;
  obj.transform.position[2] = 0;
  obj.surfaceCount = 0;
  obj.surfaces = (struct SM64Surface*)malloc(sizeof(struct SM64Surface) * 4*2);

  bool up =      !solids->is_outside_bounds(Vector(x*32, y*32-32)) && solids->get_tile(x - offsetX, y-1 - offsetY).is_solid();
  bool down =    !solids->is_outside_bounds(Vector(x*32, y*32+32)) && (solids->get_tile(x - offsetX, y+1 - offsetY).is_solid() || block.is_unisolid());
  bool left =    !solids->is_outside_bounds(Vector(x*32-32, y*32)) && (solids->get_tile(x-1 - offsetX, y - offsetY).is_solid() || block.is_unisolid());
  bool right =   !solids->is_outside_bounds(Vector(x*32+32, y*32)) && (solids->get_tile(x+1 - offsetX, y - offsetY).is_solid() || block.is_unisolid());

  int x1 = 0, y1 = 0;
  int x2 = 0, y2 = 0;

  // block ground face
  if (!up)
  {
    if (block.is_slope())
      get_slope_triangle_up(x1, y1, x2, y2, block.get_data() & (AATriangle::DIRECTION_MASK | AATriangle::DEFORM_MASK));
    else
    {
      x1 = 0;    y1 = 32;
      x2 = 32;   y2 = 32;
    }

    if (x1 || y1 || x2 || y2)
    {
      obj.surfaces[obj.surfaceCount+0].vertices[0][0] = x1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[0][1] = y1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[0][2] = -64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+0].vertices[1][0] = x2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[1][1] = y2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[1][2] = 64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+0].vertices[2][0] = x2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[2][1] = y2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[2][2] = -64 / MARIO_SCALE;

      obj.surfaces[obj.surfaceCount+1].vertices[0][0] = x2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[0][1] = y2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[0][2] = 64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+1].vertices[1][0] = x1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[1][1] = y1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[1][2] = -64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+1].vertices[2][0] = x1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[2][1] = y1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[2][2] = 64 / MARIO_SCALE;

      obj.surfaceCount += 2;
    }
  }

  // left (Z+)
  if (!left)
  {
    if (block.is_slope())
      get_slope_triangle_left(x1, y1, x2, y2, block.get_data() & (AATriangle::DIRECTION_MASK | AATriangle::DEFORM_MASK));
    else
    {
      x1 = 0;    y1 = 0;
      x2 = 0;    y2 = 32;
    }

    if (x1 || y1 || x2 || y2)
    {
      obj.surfaces[obj.surfaceCount+0].vertices[0][0] = x1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[0][1] = y1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[0][2] = -64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+0].vertices[1][0] = x2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[1][1] = y2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[1][2] = 64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+0].vertices[2][0] = x2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[2][1] = y2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[2][2] = -64 / MARIO_SCALE;

      obj.surfaces[obj.surfaceCount+1].vertices[0][0] = x2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[0][1] = y2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[0][2] = 64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+1].vertices[1][0] = x1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[1][1] = y1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[1][2] = -64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+1].vertices[2][0] = x1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[2][1] = y1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[2][2] = 64 / MARIO_SCALE;

      obj.surfaceCount += 2;
    }
  }

  // right (Z-)
  if (!right)
  {
    if (block.is_slope())
      get_slope_triangle_right(x1, y1, x2, y2, block.get_data() & (AATriangle::DIRECTION_MASK | AATriangle::DEFORM_MASK));
    else
    {
      x1 = 32;   y1 = 0;
      x2 = 32;   y2 = 32;
    }

    if (x1 || y1 || x2 || y2)
    {
      obj.surfaces[obj.surfaceCount+0].vertices[0][0] = x1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[0][1] = y1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[0][2] = 64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+0].vertices[1][0] = x2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[1][1] = y2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[1][2] = -64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+0].vertices[2][0] = x2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[2][1] = y2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[2][2] = 64 / MARIO_SCALE;

      obj.surfaces[obj.surfaceCount+1].vertices[0][0] = x2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[0][1] = y2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[0][2] = -64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+1].vertices[1][0] = x1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[1][1] = y1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[1][2] = 64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+1].vertices[2][0] = x1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[2][1] = y1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[2][2] = -64 / MARIO_SCALE;

      obj.surfaceCount += 2;
    }
  }

  // block bottom face
  if (!down)
  {
    if (block.is_slope())
      get_slope_triangle_down(x1, y1, x2, y2, block.get_data() & (AATriangle::DIRECTION_MASK | AATriangle::DEFORM_MASK));
    else
    {
      x1 = 0;    y1 = 0;
      x2 = 32;   y2 = 0;
    }

    if (x1 || y1 || x2 || y2)
    {
      obj.surfaces[obj.surfaceCount+0].vertices[0][0] = x1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[0][1] = y1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[0][2] = 64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+0].vertices[1][0] = x2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[1][1] = y2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[1][2] = -64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+0].vertices[2][0] = x2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[2][1] = y2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+0].vertices[2][2] = 64 / MARIO_SCALE;

      obj.surfaces[obj.surfaceCount+1].vertices[0][0] = x2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[0][1] = y2 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[0][2] = -64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+1].vertices[1][0] = x1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[1][1] = y1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[1][2] = 64 / MARIO_SCALE;
      obj.surfaces[obj.surfaceCount+1].vertices[2][0] = x1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[2][1] = y1 / MARIO_SCALE;    obj.surfaces[obj.surfaceCount+1].vertices[2][2] = -64 / MARIO_SCALE;

      obj.surfaceCount += 2;
    }
  }

  for (uint32_t ind=0; ind<obj.surfaceCount; ind++)
  {
    obj.surfaces[ind].type = lava ? SURFACE_BURNING : SURFACE_DEFAULT;
    obj.surfaces[ind].force = 0;
    obj.surfaces[ind].terrain = TERRAIN_STONE;
  }

  if (obj.surfaceCount)
    loaded_surfaces[(*i)++] = sm64_surface_object_create(&obj);

  free(obj.surfaces);
  return true;
}

void MarioInstance::load_new_blocks(int x, int y)
{
  delete_blocks();
  int yadd = 0;
  int arrayInd = 0;

  for (const auto& solids : Sector::get().get_solid_tilemaps())
  {
    if (solids->get_path()) continue;

    for (int xadd=-7; xadd<=7; xadd++)
    {
      // get block at floor
      for (yadd=0; y+yadd<=solids->get_height(); yadd++)
      {
        if (add_block(x+xadd, y+yadd, &arrayInd, solids)) break;
      }

      for (yadd=6; yadd>=0; yadd--)
      {
        add_block(x+xadd, y-yadd, &arrayInd, solids);
      }
    }
  }
}
