// Super Mario 64 in SuperTux by Headshotnoby

#include "mario/mario_manager.hpp"

#include <fstream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

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
"\n uniform mat3 modelviewprojection;"
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
//"\n         gl_Position = vec4( position * modelviewprojection, 1. );"
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
"\n         if (wingCap == 0 && metalCap == 0) texColor = texture(marioTex, v_uv);"
"\n         else if (wingCap == 1)"
"\n         {"
"\n             texColor = texture(marioTex, v_uv);"
"\n             if (texColor.a != 1) discard;"
"\n         }"
"\n         else if (metalCap == 1) texColor = texture(marioTex, v_uv); // NEED A WAY TO MAKE REFLECTION"
"\n         vec3 mainColor = mix( v_color, texColor.rgb, texColor.a ); // v_uv.x >= 0. ? texColor.a : 0. );"
"\n         color = vec4( mainColor * light, 1 );"
"\n     }"
"\n "
"\n #endif";


static SDL_AudioDeviceID dev;
pthread_t gSoundThread;

long long timeInMilliseconds(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return(((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

void* audio_thread(void* keepAlive)
{
	// from https://github.com/ckosmic/libsm64/blob/audio/src/libsm64.c#L535-L555
	// except keepAlive is a null pointer here, so don't use it

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

	long long currentTime = timeInMilliseconds();
	long long targetTime = 0;
	while(1)
	{
		//if(!*((bool*)keepAlive)) return NULL;

		int16_t audioBuffer[544 * 2 * 2];
		uint32_t numSamples = sm64_audio_tick(SDL_GetQueuedAudioSize(dev)/4, 1100, audioBuffer);
		if (SDL_GetQueuedAudioSize(dev)/4 < 6000)
			SDL_QueueAudio(dev, audioBuffer, numSamples * 2 * 4);

		targetTime = currentTime + 33;
		while (timeInMilliseconds() < targetTime)
		{
			usleep(100);
			//if(!*((bool*)keepAlive)) return NULL;
		}
		currentTime = timeInMilliseconds();
	}
}

void audio_init()
{
	if (!SDL_WasInit(SDL_INIT_AUDIO)) {
		if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
			fprintf(stderr, "SDL_InitSubSystem(SDL_INIT_AUDIO) failed: %s\n", SDL_GetError());
			return;
		}
	}

	SDL_AudioSpec want, have;
	SDL_zero(want);
	want.freq = 32000;
	want.format = AUDIO_S16;
	want.channels = 2;
	want.samples = 512;
	want.callback = NULL;
	dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if (dev == 0) {
		fprintf(stderr, "SDL_OpenAudio error: %s\n", SDL_GetError());
		return;
	}
	SDL_PauseAudioDevice(dev, 0);

	// it's best to run audio in a separate thread
	pthread_create(&gSoundThread, NULL, audio_thread, NULL);
}


MarioManager::MarioManager()
{
  /** initialize libsm64 */
  loaded = false;
  std::ifstream file("sm64.us.z64", std::ios::ate | std::ios::binary);

  if (!file)
  {
    error_message = "Super Mario 64 US ROM not found!\nPlease provide a ROM with the filename \"sm64.us.z64\"";
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
      error_message = str(boost::format("Super Mario 64 US ROM MD5 mismatch!\nExpected: %s\nYour copy: %s\nPlease provide the correct ROM")
                      % SM64_MD5 % hexResult);
    }
    else
    {
      /** Mario texture is 704x64 RGBA */
      mario_texture = (uint8_t*)malloc(4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT);

      /** load libsm64 */
      sm64_global_init(romBuffer, mario_texture);
      //sm64_register_debug_print_function( [](const char *msg) {ConsoleBuffer::output << "libsm64: " << msg << std::endl;} );

      sm64_audio_init(romBuffer);
      audio_init();
      sm64_play_sound_global(SOUND_MENU_STAR_SOUND);

	  VideoSystem::current()->init_mario(mario_texture, &mario_texture_handle, &mario_shader_handle, MARIO_SHADER);
      for(int i=0; i<3*SM64_GEO_MAX_TRIANGLES; i++) mario_indices[i] = i;

      loaded = true;
    }

    delete[] romBuffer;
  }
}

void MarioManager::init_mario(SM64MarioGeometryBuffers* geometry, MarioMesh* mesh)
{
  VideoSystem::current()->init_mario_instance(geometry, mesh);
}

void MarioManager::destroy_mario(MarioMesh* mesh)
{
  VideoSystem::current()->destroy_mario_instance(mesh);
}

/* EOF */
