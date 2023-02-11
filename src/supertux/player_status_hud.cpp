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

#include "supertux/player_status_hud.hpp"

#include <sstream>

#include "mario/mario_manager.hpp"
#include "math/easing.hpp"
#include "math/util.hpp"
#include "object/player.hpp"
#include "supertux/game_object.hpp"
#include "supertux/gameconfig.hpp"
#include "supertux/globals.hpp"
#include "supertux/player_status.hpp"
#include "supertux/resources.hpp"
#include "video/drawing_context.hpp"
#include "video/surface.hpp"
#include "video/video_system.hpp"
#include "video/viewport.hpp"
#include "editor/editor.hpp"

static const int DISPLAYED_COINS_UNSET = -1;

PlayerStatusHUD::PlayerStatusHUD(PlayerStatus& player_status) :
  m_player_status(player_status),
  displayed_coins(DISPLAYED_COINS_UNSET),
  displayed_coins_frame(0),
  coin_surface(Surface::from_file("images/engine/hud/coins-0.png")),
  fire_surface(Surface::from_file("images/objects/bullets/fire-hud.png")),
  ice_surface(Surface::from_file("images/objects/bullets/ice-hud.png")),
  fire_surface_mario(Surface::from_file("images/objects/bullets/fire_bullet-1.png")),
  ice_surface_mario(Surface::from_file("images/objects/bullets/ice_bullet.png")),
  mario_health_y(-128.f),
  mario_health_state(0),
  mario_health_state_add(0),
  mario_health_state_timer(0.f)
{
}

void
PlayerStatusHUD::reset()
{
  displayed_coins = DISPLAYED_COINS_UNSET;
}

void
PlayerStatusHUD::draw_sm64_number(DrawingContext& context, Vector& pos, int num)
{
  while (num > 0)
  {
    int digit = num % 10;
	num /= 10;
    context.color().draw_sm64_texture(MarioManager::current()->ui_texture_handle,
                                      pos,
                                      Vector(16, 16),
                                      Vector((digit*16-1)/(float)ui_atlas_info.atlasWidth, 0),
                                      Vector((digit*16+15)/(float)ui_atlas_info.atlasWidth, 1),
                                      Color::WHITE,
                                      LAYER_HUD);
    pos.x -= 14;
  }

  // draws "x"
  pos.x -= 2;
  context.color().draw_sm64_texture(MarioManager::current()->ui_texture_handle,
                                    pos,
                                    Vector(16, 16),
                                    Vector((10*16-1)/(float)ui_atlas_info.atlasWidth, 0),
                                    Vector((11*16-1)/(float)ui_atlas_info.atlasWidth, 1),
                                    Color::WHITE,
                                    LAYER_HUD);
  pos.x -= 16;
}

void
PlayerStatusHUD::update(float dt_sec)
{
  if (!Sector::current()) return; // don't run this code if on worldmap

  Player& tux = Sector::get().get_player();
  if (!tux.is_mario()) return;

  int16_t healthSlices = (tux.m_mario_obj->state.health >> 8);
  if (healthSlices < 8 || (tux.m_mario_obj->state.action & ACT_GROUP_MASK) == ACT_GROUP_SUBMERGED)
  {
    mario_health_state_add = 1;
    mario_health_state_timer = 1.5f;
  }

  if (mario_health_state_timer > 0)
  {
    mario_health_state_timer -= dt_sec;
    if (mario_health_state_timer <= 0)
    {
      mario_health_state_timer = 0;
      mario_health_state_add = -1;
    }
  }

  mario_health_state = math::clamp(mario_health_state + mario_health_state_add, 0, 15);
  mario_health_y = 128*QuadraticEaseOut(mario_health_state/15.f) - 128;
}

void
PlayerStatusHUD::draw(DrawingContext& context)
{
  if (!g_config->show_hud) return;
  int player_id = 0;

  if ((displayed_coins == DISPLAYED_COINS_UNSET) ||
      (std::abs(displayed_coins - m_player_status.coins) > 100)) {
    displayed_coins = m_player_status.coins;
    displayed_coins_frame = 0;
  }
  if (++displayed_coins_frame > 2) {
    displayed_coins_frame = 0;
    if (displayed_coins < m_player_status.coins) displayed_coins++;
    if (displayed_coins > m_player_status.coins) displayed_coins--;
  }
  displayed_coins = std::min(std::max(displayed_coins, 0), m_player_status.get_max_coins());

  std::ostringstream ss;
  ss << displayed_coins;
  std::string coins_text = ss.str();

  context.push_transform();
  context.set_translation(Vector(0, 0));
  if (!Editor::is_active())
  {
    Vector pos(static_cast<float>(context.get_width()) - BORDER_X - Resources::fixed_font->get_text_width(coins_text),
               BORDER_Y + (Resources::fixed_font->get_text_height(coins_text) + 5.0f) * static_cast<float>(player_id));

    if (g_config->mario)
    {
      pos.x += Resources::fixed_font->get_text_width(coins_text) - 16;
      draw_sm64_number(context, pos, displayed_coins);

      context.color().draw_sm64_texture(MarioManager::current()->ui_texture_handle,
                                        pos,
                                        Vector(16, 16),
                                        Vector((11*16-1)/(float)ui_atlas_info.atlasWidth, 0),
                                        Vector((12*16-1)/(float)ui_atlas_info.atlasWidth, 1),
                                        Color::WHITE,
                                        LAYER_HUD);
    }
    else
    {
      if (coin_surface)
      {
        context.color().draw_surface(coin_surface,
                                    Vector(static_cast<float>(context.get_width()) - BORDER_X - static_cast<float>(coin_surface->get_width()) - Resources::fixed_font->get_text_width(coins_text),
                                            BORDER_Y + 1.0f + (Resources::fixed_font->get_text_height(coins_text) + 5) * static_cast<float>(player_id)),
                                    LAYER_HUD);
      }

      context.color().draw_text(Resources::fixed_font,
                                coins_text,
                                pos,
                                ALIGN_LEFT,
                                LAYER_HUD,
                                PlayerStatusHUD::text_color);
    }
  }
  std::string ammo_text;

  if (m_player_status.bonus == FIRE_BONUS) {

    ammo_text = std::to_string(m_player_status.max_fire_bullets);
    Vector pos(static_cast<float>(context.get_width())
                   - BORDER_X
                   - Resources::fixed_font->get_text_width(ammo_text),
               BORDER_Y
                   + (Resources::fixed_font->get_text_height(coins_text) + 5.0f)
                   + (Resources::fixed_font->get_text_height(ammo_text) + 5.0f)
                   * static_cast<float>(player_id));

    if (g_config->mario)
    {
      pos.x += Resources::fixed_font->get_text_width(ammo_text) - 16;
      draw_sm64_number(context, pos, m_player_status.max_fire_bullets);

      if (fire_surface_mario)
        context.color().draw_surface(fire_surface_mario, pos - Vector(0, 1), LAYER_HUD);
    }
    else
    {
      if (fire_surface) {
        context.color().draw_surface(fire_surface,
                                     Vector(static_cast<float>(context.get_width())
                                                - BORDER_X
                                                - static_cast<float>(fire_surface->get_width())
                                                - Resources::fixed_font->get_text_width(ammo_text),
                                            BORDER_Y
                                                + 1.0f
                                                + (Resources::fixed_font->get_text_height(coins_text) + 5)
                                                + (Resources::fixed_font->get_text_height(ammo_text) + 5)
                                                * static_cast<float>(player_id)),
                                     LAYER_HUD);
      }

      context.color().draw_text(Resources::fixed_font,
                                ammo_text,
                                pos,
                                ALIGN_LEFT,
                                LAYER_HUD,
                                PlayerStatusHUD::text_color);
    }
  }

  if (m_player_status.bonus == ICE_BONUS) {

    ammo_text = std::to_string(m_player_status.max_ice_bullets);
    Vector pos(static_cast<float>(context.get_width())
                   - BORDER_X
                   - Resources::fixed_font->get_text_width(ammo_text),
               BORDER_Y
                   + (Resources::fixed_font->get_text_height(coins_text) + 5.0f)
                   + (Resources::fixed_font->get_text_height(ammo_text) + 5.0f)
                   * static_cast<float>(player_id));

    if (g_config->mario)
    {
      pos.x += Resources::fixed_font->get_text_width(ammo_text) - 16;
      draw_sm64_number(context, pos, m_player_status.max_ice_bullets);

      if (ice_surface_mario)
        context.color().draw_surface(ice_surface_mario, pos + Vector(0, 1), LAYER_HUD);
    }
    else
    {
      if (ice_surface) {
        context.color().draw_surface(ice_surface,
                                     Vector(static_cast<float>(context.get_width())
                                                - BORDER_X
                                                - static_cast<float>(ice_surface->get_width())
                                                - Resources::fixed_font->get_text_width(ammo_text),
                                            BORDER_Y
                                                + 1.0f
                                                + (Resources::fixed_font->get_text_height(coins_text) + 5)
                                                + (Resources::fixed_font->get_text_height(ammo_text) + 5)
                                                * static_cast<float>(player_id)),
                                     LAYER_HUD);
      }

      context.color().draw_text(Resources::fixed_font,
                                ammo_text,
                                pos,
                                ALIGN_LEFT,
                                LAYER_HUD,
                                PlayerStatusHUD::text_color);
    }
  }

  if (Sector::current()) // don't run this code if on worldmap
  {
    Player& tux = Sector::get().get_player();
    if (tux.is_mario() && !tux.is_deactivated())
    {
      context.color().draw_sm64_texture(MarioManager::current()->health_texture_handle,
                                        Vector(SCREEN_WIDTH/2.f-62, mario_health_y),
                                        Vector(64, 128),
                                        Vector((0*32)/(float)health_atlas_info.atlasWidth, 0),
                                        Vector((1*32)/(float)health_atlas_info.atlasWidth, 1),
                                        Color::WHITE,
                                        LAYER_HUD);

      context.color().draw_sm64_texture(MarioManager::current()->health_texture_handle,
                                        Vector(SCREEN_WIDTH/2.f, mario_health_y),
                                        Vector(64, 128),
                                        Vector((2*32)/(float)health_atlas_info.atlasWidth, 0),
                                        Vector((3*32)/(float)health_atlas_info.atlasWidth, 1),
                                        Color::WHITE,
                                        LAYER_HUD);

      int16_t healthSlices = (tux.m_mario_obj->state.health >> 8);
      if (healthSlices > 0)
      {
        int uCoord = (20 - healthSlices*2);
        context.color().draw_sm64_texture(MarioManager::current()->health_texture_handle,
                                          Vector(SCREEN_WIDTH/2.f-32, mario_health_y+32),
                                          Vector(64, 128),
                                          Vector((uCoord*32)/(float)health_atlas_info.atlasWidth, 0),
                                          Vector((uCoord*32+32)/(float)health_atlas_info.atlasWidth, 1),
                                          Color::WHITE,
                                          LAYER_HUD);
      }
    }
  }

  context.pop_transform();
}

/* EOF */
