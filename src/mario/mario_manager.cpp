// Super Mario 64 in SuperTux by Headshotnoby

#include "mario/mario_manager.hpp"

#include <fstream>
#include <string>

#include <boost/format.hpp>

extern "C" {
#include <libsm64.h>
#include <decomp/include/audio_defines.h>
}

#include "addon/md5.hpp"
#include "gui/dialog.hpp"

MarioManager::MarioManager()
{
  // initialize libsm64
  std::ifstream file("sm64.us.z64", std::ios::ate | std::ios::binary);

  if (!file)
  {
    Dialog::show_message("Super Mario 64 US ROM not found!\nPlease provide a ROM with the filename \"sm64.us.z64\"");
  }
  else
  {
    // load ROM into memory
    uint8_t *romBuffer;
    size_t romFileLength = file.tellg();

    romBuffer = new uint8_t[romFileLength + 1];
	file.seekg(0);
	file.read((char*)romBuffer, romFileLength);
	romBuffer[romFileLength] = 0;
	file.close();

    // perform MD5 check to make sure it's the correct ROM
    MD5 ctxt;
	ctxt.update(romBuffer, romFileLength);
    std::string hexResult = ctxt.hex_digest();

    std::string SM64_MD5 = "20b854b239203baf6c961b850a4a51a2";
    if (hexResult.compare(SM64_MD5)) // mismatch
    {
      std::string text = str(boost::format("Super Mario 64 US ROM MD5 mismatch!\nExpected: %s\nYour copy: %s\nPlease provide the correct ROM")
                         % SM64_MD5 % hexResult);

      Dialog::show_message(text);
    }
    else
    {
      // Mario texture is 704x64 RGBA
      mario_texture = (uint8_t*)malloc(4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT);

      // load libsm64
      sm64_global_init(romBuffer, mario_texture, 0 /*[](const char *msg) {dbg_msg("libsm64", "%s", msg);}*/);
      //dbg_msg("libsm64", "Super Mario 64 US ROM loaded!");
      sm64_play_sound_global(SOUND_MENU_STAR_SOUND);

      //Graphics()->firstInitMario(&m_MarioShaderHandle, &m_MarioTexHandle, m_MarioTexture, MARIO_SHADER);

      //for(int i=0; i<3*SM64_GEO_MAX_TRIANGLES; i++) m_MarioIndices[i] = i;
    }

    delete[] romBuffer;
  }
}
