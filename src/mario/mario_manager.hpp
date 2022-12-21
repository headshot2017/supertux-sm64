// Super Mario 64 in SuperTux by Headshotnoby

#ifndef HEADER_SUPERTUX_MARIO_MANAGER_HPP
#define HEADER_SUPERTUX_MARIO_MANAGER_HPP

#include <stdint.h>

#include "util/currenton.hpp"

class MarioManager final : public Currenton<MarioManager>
{
public:
  MarioManager();

private:
  uint8_t *mario_texture;

private:
  MarioManager(const MarioManager&) = delete;
  MarioManager& operator=(const MarioManager&) = delete;
};

#endif
