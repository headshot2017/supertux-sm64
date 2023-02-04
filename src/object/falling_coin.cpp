//  SuperTux
//  Copyright (C) 2006 Ondrej Hosek <ondra.hosek@gmail.com>
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

#include "object/falling_coin.hpp"

#include "mario/mario_manager.hpp"
#include "object/camera.hpp"
#include "object/player.hpp"
#include "sprite/sprite.hpp"
#include "sprite/sprite_manager.hpp"
#include "supertux/globals.hpp"
#include "supertux/sector.hpp"
#include "video/video_system.hpp"
#include "video/viewport.hpp"

FallingCoin::FallingCoin(const Vector& start_position, float vel_x) :
  physic(),
  pos(start_position),
  sprite(SpriteManager::current()->create("images/objects/coin/coin.sprite")),
  m_mario_sprite_frame(0),
  m_mario_sprite_timer(0.f)
{
  physic.set_velocity_y(-800.0f);
  physic.set_velocity_x(vel_x);
}

void
FallingCoin::draw(DrawingContext& context)
{
  if (!Sector::get().get_player().is_mario())
    sprite->draw(context.color(), pos, LAYER_FLOATINGOBJECTS + 5);
  else
    context.color().draw_sm64_texture(MarioManager::current()->coin_texture_handle,
                                      pos - Sector::get().get_camera().get_translation(),
                                      Vector(32, 32),
                                      Vector((m_mario_sprite_frame*32)/(float)coin_atlas_info.atlasWidth, 0),
                                      Vector((m_mario_sprite_frame*32+32)/(float)coin_atlas_info.atlasWidth, 1),
                                      Color::YELLOW,
                                      LAYER_FLOATINGOBJECTS + 5);
}

void
FallingCoin::update(float dt_sec)
{
  if (Sector::get().get_player().is_mario())
  {
    m_mario_sprite_timer += dt_sec;
    while (m_mario_sprite_timer >= 0.1f)
    {
      m_mario_sprite_timer -= 0.1f;
      m_mario_sprite_frame++;
      if (m_mario_sprite_frame > 3) m_mario_sprite_frame = 0;
    }
  }

  pos += physic.get_movement(dt_sec);
  if (pos.y > static_cast<float>(SCREEN_HEIGHT) &&
      physic.get_velocity_y() > 0.0f)
    remove_me();
}

/* EOF */
