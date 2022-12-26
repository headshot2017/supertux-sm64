// Super Mario 64 in SuperTux by Headshotnoby

#include "mario/mario_instance.hpp"

#include <stdint.h>
#include <string.h>
#include <limits.h>

#define INT_SUBTYPE_BIG_KNOCKBACK 0x00000008
extern "C" {
#include <decomp/include/surface_terrains.h>
}

#include "mario/mario_manager.hpp"
#include "math/rect.hpp"
#include "math/rectf.hpp"
#include "object/camera.hpp"
#include "supertux/console.hpp"
#include "supertux/sector.hpp"
#include "video/canvas.hpp"
#include "video/drawing_context.hpp"

MarioInstance::MarioInstance()
{
  memset(loaded_surfaces, UINT_MAX, sizeof(uint32_t) * MAX_SURFACES);
  memset(&input, 0, sizeof(input));
  memset(&mesh, 0, sizeof(mesh));
  mario_id = -1;
}

MarioInstance::~MarioInstance()
{
  ConsoleBuffer::output << "call mario destructor" << std::endl;
  destroy();
}

void MarioInstance::spawn(float x, float y)
{
  destroy();
  tick = 0;

  // on supertux, up coordinate is Y-, SM64 is Y+. flip the Y coordinate
  // scale conversions:
  //    supertux -> sm64: divide
  //    sm64 -> supertux: multiply
  int spawnX = x / MARIO_SCALE;
  int spawnY = -y / MARIO_SCALE;

  //exportMap(spawnX, spawnY);

  ConsoleBuffer::output << "Attempt to spawn Mario at " << x << " " << y << std::endl;

  load_new_blocks(x/32, y/32);
  mario_id = sm64_mario_create(spawnX, spawnY, 0, 0,0,0,0);

  if (spawned())
  {
    ConsoleBuffer::output << "Mario spawned!" << std::endl;
    geometry.position = new float[9 * SM64_GEO_MAX_TRIANGLES];
    geometry.normal   = new float[9 * SM64_GEO_MAX_TRIANGLES];
    geometry.color    = new float[9 * SM64_GEO_MAX_TRIANGLES];
    geometry.uv       = new float[6 * SM64_GEO_MAX_TRIANGLES];
    geometry.numTrianglesUsed = 0;
    MarioManager::current()->init_mario(&geometry, &mesh);

    // load a static surface way below the level
    uint32_t surfaceCount = 2;
	SM64Surface surfaces[surfaceCount];

	for (uint32_t i=0; i<surfaceCount; i++)
	{
		surfaces[i].type = SURFACE_DEFAULT;
		surfaces[i].force = 0;
		surfaces[i].terrain = TERRAIN_STONE;
	}
	
	int width = Sector::get().get_width()/2 / MARIO_SCALE;
	int spawnX = width;
	int spawnY = (Sector::get().get_height()+256) / -MARIO_SCALE;
	
	surfaces[surfaceCount-2].vertices[0][0] = spawnX + width + 128;		surfaces[surfaceCount-2].vertices[0][1] = spawnY;	surfaces[surfaceCount-2].vertices[0][2] = +128;
	surfaces[surfaceCount-2].vertices[1][0] = spawnX - width - 128;		surfaces[surfaceCount-2].vertices[1][1] = spawnY;	surfaces[surfaceCount-2].vertices[1][2] = -128;
	surfaces[surfaceCount-2].vertices[2][0] = spawnX - width - 128;		surfaces[surfaceCount-2].vertices[2][1] = spawnY;	surfaces[surfaceCount-2].vertices[2][2] = +128;

	surfaces[surfaceCount-1].vertices[0][0] = spawnX - width - 128;		surfaces[surfaceCount-1].vertices[0][1] = spawnY;	surfaces[surfaceCount-1].vertices[0][2] = -128;
	surfaces[surfaceCount-1].vertices[1][0] = spawnX + width + 128;		surfaces[surfaceCount-1].vertices[1][1] = spawnY;	surfaces[surfaceCount-1].vertices[1][2] = +128;
	surfaces[surfaceCount-1].vertices[2][0] = spawnX + width + 128;		surfaces[surfaceCount-1].vertices[2][1] = spawnY;	surfaces[surfaceCount-1].vertices[2][2] = -128;

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

    sm64_mario_delete(mario_id);
    mario_id = -1;

    MarioManager::current()->destroy_mario(&mesh);

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

    for (int i=0; i<geometry.numTrianglesUsed*3; i++)
    {
      geometry.normal[i*3+1] *= -1;

      m_curr_geometry_pos[i*3+0] = geometry.position[i*3+0]*MARIO_SCALE;
      m_curr_geometry_pos[i*3+1] = geometry.position[i*3+1]*-MARIO_SCALE + 16;
      m_curr_geometry_pos[i*3+2] = geometry.position[i*3+2]*MARIO_SCALE;
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
  if (!geometry.numTrianglesUsed) return;
  auto mariomanager = MarioManager::current();

  DrawingContext& context = canvas.get_context();
  context.push_transform();

  canvas.draw_mario(&geometry,
                    &mesh,
                    m_pos,
                    camera,
                    state.flags,
                    mariomanager->get_texture(),
                    mariomanager->get_shader(),
                    mariomanager->get_indices());

  context.pop_transform();
}

void MarioInstance::bounce(bool jump)
{
  sm64_mario_attack(mario_id, state.position[0], state.position[1]-8, state.position[2], 0);
  if (jump && !(state.action & ACT_FLAG_INVULNERABLE)) sm64_set_mario_velocity(mario_id, state.velocity[0], 50, 0);
}

void MarioInstance::hurt(uint32_t damage, Vector& src)
{
  if (state.action & ACT_FLAG_INVULNERABLE) return;
  uint32_t subtype = (damage >= 3) ? INT_SUBTYPE_BIG_KNOCKBACK : 0;
  sm64_mario_take_damage(mario_id, damage, subtype, src.x/MARIO_SCALE, src.y/MARIO_SCALE, 0);
}

void MarioInstance::heal(uint8_t amount)
{
  sm64_mario_heal(mario_id, amount);
}

void MarioInstance::set_pos(const Vector& pos)
{
  if (!spawned() || pos.x <= 0 || pos.y <= 0)
    return;

  sm64_set_mario_position(mario_id, pos.x/MARIO_SCALE, -pos.y/MARIO_SCALE, 0);
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
	if ((*i) >= MAX_SURFACES) return false;
	const Tile& block = solids->get_tile(x, y);
	if (!block.is_solid()) return false;

	struct SM64SurfaceObject obj;
	memset(&obj.transform, 0, sizeof(struct SM64ObjectTransform));
	obj.transform.position[0] = x*32 / MARIO_SCALE;
	obj.transform.position[1] = (-y*32-16) / MARIO_SCALE;
	obj.transform.position[2] = 0;
	obj.surfaceCount = 0;
	obj.surfaces = (struct SM64Surface*)malloc(sizeof(struct SM64Surface) * 4*2);

	bool up =		solids->get_tile(x, y-1).is_solid();
	bool down =		solids->get_tile(x, y+1).is_solid();
	bool left =		solids->get_tile(x-1, y).is_solid();
	bool right =	solids->get_tile(x+1, y).is_solid();

	// block ground face
	if (!up)
	{
		obj.surfaces[obj.surfaceCount+0].vertices[0][0] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[0][1] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[0][2] = 64 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+0].vertices[1][0] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[1][1] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[1][2] = -64 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+0].vertices[2][0] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[2][1] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[2][2] = 64 / MARIO_SCALE;

		obj.surfaces[obj.surfaceCount+1].vertices[0][0] = 0 / MARIO_SCALE; 		obj.surfaces[obj.surfaceCount+1].vertices[0][1] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[0][2] = -64 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+1].vertices[1][0] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[1][1] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[1][2] = 64 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+1].vertices[2][0] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[2][1] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[2][2] = -64 / MARIO_SCALE;

		obj.surfaceCount += 2;
	}

	// left (Z+)
	if (!left)
	{
		obj.surfaces[obj.surfaceCount+0].vertices[0][2] = -64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount+0].vertices[0][1] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[0][0] = 0 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+0].vertices[1][2] = 64 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[1][1] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[1][0] = 0 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+0].vertices[2][2] = -64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount+0].vertices[2][1] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[2][0] = 0 / MARIO_SCALE;

		obj.surfaces[obj.surfaceCount+1].vertices[0][2] = 64 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[0][1] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[0][0] = 0 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+1].vertices[1][2] = -64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount+1].vertices[1][1] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[1][0] = 0 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+1].vertices[2][2] = 64 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[2][1] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[2][0] = 0 / MARIO_SCALE;

		obj.surfaceCount += 2;
	}

	// right (Z-)
	if (!right)
	{
		obj.surfaces[obj.surfaceCount+0].vertices[0][2] = 64 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[0][1] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[0][0] = 32 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+0].vertices[1][2] = -64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount+0].vertices[1][1] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[1][0] = 32 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+0].vertices[2][2] = 64 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[2][1] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[2][0] = 32 / MARIO_SCALE;

		obj.surfaces[obj.surfaceCount+1].vertices[0][2] = -64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount+1].vertices[0][1] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[0][0] = 32 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+1].vertices[1][2] = 64 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[1][1] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[1][0] = 32 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+1].vertices[2][2] = -64 / MARIO_SCALE;	obj.surfaces[obj.surfaceCount+1].vertices[2][1] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[2][0] = 32 / MARIO_SCALE;

		obj.surfaceCount += 2;
	}

	// block bottom face
	if (!down)
	{
		obj.surfaces[obj.surfaceCount+0].vertices[0][0] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[0][1] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[0][2] = 64 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+0].vertices[1][0] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[1][1] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[1][2] = -64 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+0].vertices[2][0] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[2][1] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+0].vertices[2][2] = 64 / MARIO_SCALE;

		obj.surfaces[obj.surfaceCount+1].vertices[0][0] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[0][1] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[0][2] = -64 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+1].vertices[1][0] = 32 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[1][1] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[1][2] = 64 / MARIO_SCALE;
		obj.surfaces[obj.surfaceCount+1].vertices[2][0] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[2][1] = 0 / MARIO_SCALE;		obj.surfaces[obj.surfaceCount+1].vertices[2][2] = -64 / MARIO_SCALE;

		obj.surfaceCount += 2;
	}

	for (uint32_t ind=0; ind<obj.surfaceCount; ind++)
	{
		obj.surfaces[ind].type = SURFACE_DEFAULT;
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

  /*
  for (const auto& solids : Sector::get().get_solid_tilemaps()) {
    // test with all tiles in this rectangle
    const Rect test_tiles = solids->get_tiles_overlapping(rect);

    for (int x = test_tiles.left; x < test_tiles.right; ++x) {
      for (int y = test_tiles.top; y < test_tiles.bottom; ++y) {
        const Tile& tile = solids->get_tile(x, y);

        if (!(tile.get_attributes() & tiletype))
          continue;
        if (tile.is_unisolid () && ignoreUnisolid)
          continue;
        if (tile.is_slope ()) {
          AATriangle triangle;
          const Rectf tbbox = solids->get_tile_bbox(x, y);
          triangle = AATriangle(tbbox, tile.get_data());
          Constraints constraints;
          if (!collision::rectangle_aatriangle(&constraints, rect, triangle))
            continue;
        }
        // We have a solid tile that overlaps the given rectangle.
        return false;
      }
    }
  }
  */

}