//  SuperTux - End Sequence: Tux walks right
//  Copyright (C) 2007 Christoph Sommer <christoph.sommer@2006.expires.deltadevelopment.de>
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

#include "object/endsequence_fireworks.hpp"

extern "C" {
#include <libsm64.h>
#include <decomp/include/sm64shared.h>
}

#include "object/fireworks.hpp"
#include "object/player.hpp"
#include "supertux/screen_manager.hpp"
#include "supertux/sector.hpp"

EndSequenceFireworks::EndSequenceFireworks() :
  EndSequence(),
  endsequence_timer()
{
}

EndSequenceFireworks::~EndSequenceFireworks()
{
}

void
EndSequenceFireworks::draw(DrawingContext& /*context*/)
{
}

void
EndSequenceFireworks::starting()
{
  EndSequence::starting();
  endsequence_timer.start(7.3f * ScreenManager::current()->get_speed());
  Sector::get().add<Fireworks>();
}

void
EndSequenceFireworks::running(float dt_sec)
{
  EndSequence::running(dt_sec);
  Player& tux = Sector::get().get_player();

  if (tux_may_walk) {
    if (!tux.is_mario())
      end_sequence_controller->press(Control::JUMP);
    else if (tux.m_mario_obj->state.action != ACT_STAR_DANCE_EXIT)
      sm64_set_mario_action(tux.m_mario_obj->ID(), ACT_STAR_DANCE_EXIT);
  }

  if (endsequence_timer.check()) isdone = true;
}

void
EndSequenceFireworks::stopping()
{
  EndSequence::stopping();
}

/* EOF */
