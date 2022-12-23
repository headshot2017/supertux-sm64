// Super Mario 64 in SuperTux by Headshotnoby

#include "mario/mario_manager.hpp"

#include <fstream>
#include <string>

#include <boost/format.hpp>

extern "C" {
#include <decomp/include/audio_defines.h>
}

#include "addon/md5.hpp"
#include "gui/dialog.hpp"
#include "supertux/console.hpp"
#include "util/log.hpp"
#include "video/video_system.hpp"


static const char *MARIO_SHADER =
"\n uniform mat4 view;"
"\n uniform mat4 projection;"
"\n uniform sampler2D marioTex;"
"\n uniform int wingCap;"
"\n uniform int metalCap;"
"\n "
"\n v2f vec3 v_color;"
"\n v2f vec3 v_normal;"
"\n v2f vec3 v_light;"
"\n v2f vec2 v_uv;"
"\n "
"\n #ifdef VERTEX"
"\n "
"\n     in vec3 position;"
"\n     in vec3 normal;"
"\n     in vec3 color;"
"\n     in vec2 uv;"
"\n "
"\n     void main()"
"\n     {"
"\n         v_color = color;"
"\n         v_normal = normal;"
"\n         v_light = transpose( mat3( view )) * normalize( vec3( 1 ));"
"\n         v_uv = uv;"
"\n "
"\n         gl_Position = projection * view * vec4( position, 1. );"
"\n     }"
"\n "
"\n #endif"
"\n #ifdef FRAGMENT"
"\n "
"\n     out vec4 color;"
"\n "
"\n     void main() "
"\n     {"
"\n         float light = .5 + .5 * clamp( dot( v_normal, v_light ), 0., 1. );"
"\n         vec4 texColor = vec4(0);"
"\n         if (wingCap == 0 && metalCap == 0) texColor = texture2D(marioTex, v_uv);"
"\n         else if (wingCap == 1)"
"\n         {"
"\n             texColor = texture2D(marioTex, v_uv);"
"\n             if (texColor.a != 1) discard;"
"\n         }"
"\n         else if (metalCap == 1) texColor = texture2D(marioTex, v_uv); // NEED A WAY TO MAKE REFLECTION"
"\n         vec3 mainColor = mix( v_color, texColor.rgb, texColor.a ); // v_uv.x >= 0. ? texColor.a : 0. );"
"\n         color = vec4( mainColor * light, 1 );"
"\n     }"
"\n "
"\n #endif";


MarioManager::MarioManager()
{
  /** initialize libsm64 */
  loaded = false;
  std::ifstream file("sm64.us.z64", std::ios::ate | std::ios::binary);

  if (!file)
  {
    Dialog::show_message("Super Mario 64 US ROM not found!\nPlease provide a ROM with the filename \"sm64.us.z64\"");
  }
  else
  {
    /** load ROM into memory */
    uint8_t *romBuffer;
    size_t romFileLength = file.tellg();

    romBuffer = new uint8_t[romFileLength + 1];
	file.seekg(0);
	file.read((char*)romBuffer, romFileLength);
	romBuffer[romFileLength] = 0;
	file.close();

    /** perform MD5 check to make sure it's the correct ROM */
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
      /** Mario texture is 704x64 RGBA */
      mario_texture = (uint8_t*)malloc(4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT);

      /** load libsm64 */
      sm64_global_init(romBuffer, mario_texture, [](const char *msg) {ConsoleBuffer::output << "libsm64: " << msg << "\n";});
      sm64_play_sound_global(SOUND_MENU_STAR_SOUND);

	  VideoSystem::current()->init_mario(mario_texture, &mario_texture_handle, &mario_shader_handle, MARIO_SHADER);
      for(int i=0; i<3*SM64_GEO_MAX_TRIANGLES; i++) mario_indices[i] = i;

      loaded = true;
    }

    delete[] romBuffer;
  }
}
