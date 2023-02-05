// Super Mario 64 in SuperTux by Headshotnoby

#include "mario/mario_manager.hpp"

#include <stdio.h>
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
#include "supertux/gameconfig.hpp"
#include "supertux/globals.hpp"
#include "util/log.hpp"
#include "video/video_system.hpp"


/**
	SM64 texture atlases
	https://github.com/ckosmic/libsm64-gmod/blob/master/g64/utils.cpp#L123
*/

/** Mario's texture */
SM64TextureAtlasInfo mario_atlas_info = {
    .offset = 0x114750,
    .numUsedTextures = 11,
    .atlasWidth = 11 * 64,
    .atlasHeight = 64,
    .texInfos = {
        {.offset = 144, .width = 64, .height = 32, .format = FORMAT_RGBA },
        {.offset = 4240, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 6288, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 8336, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 10384, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 12432, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 14480, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 16528, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 30864, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 32912, .width = 32, .height = 64, .format = FORMAT_RGBA },
        {.offset = 37008, .width = 32, .height = 64, .format = FORMAT_RGBA },
    }
};

/** Power Meter texture */
SM64TextureAtlasInfo health_atlas_info = {
    .offset = 0x201410,
    .numUsedTextures = 11,
    .atlasWidth = 11 * 64,
    .atlasHeight = 64,
    .texInfos = {
        {.offset = 0x233E0, .width = 32, .height = 64, .format = FORMAT_RGBA },
        {.offset = 0x243E0, .width = 32, .height = 64, .format = FORMAT_RGBA },
        {.offset = 0x253E0, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 0x25BE0, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 0x263E0, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 0x26BE0, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 0x273E0, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 0x27BE0, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 0x283E0, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 0x28BE0, .width = 32, .height = 32, .format = FORMAT_RGBA },
        {.offset = 0x29628, .width = 32, .height = 64, .format = FORMAT_RGBA },
    }
};

/** UI texture (only using Mario's head for world map) */
SM64TextureAtlasInfo ui_atlas_info = {
    .offset = 0x108A40,
    .numUsedTextures = 14,
    .atlasWidth = 14 * 16,
    .atlasHeight = 16,
    .texInfos = {
        {.offset = 0x0000, .width = 16, .height = 16, .format = FORMAT_RGBA },
        {.offset = 0x0200, .width = 16, .height = 16, .format = FORMAT_RGBA },
        {.offset = 0x0400, .width = 16, .height = 16, .format = FORMAT_RGBA },
        {.offset = 0x0600, .width = 16, .height = 16, .format = FORMAT_RGBA },
        {.offset = 0x0800, .width = 16, .height = 16, .format = FORMAT_RGBA },
        {.offset = 0x0A00, .width = 16, .height = 16, .format = FORMAT_RGBA },
        {.offset = 0x0C00, .width = 16, .height = 16, .format = FORMAT_RGBA },
        {.offset = 0x0E00, .width = 16, .height = 16, .format = FORMAT_RGBA },
        {.offset = 0x1000, .width = 16, .height = 16, .format = FORMAT_RGBA },
        {.offset = 0x1200, .width = 16, .height = 16, .format = FORMAT_RGBA },
        {.offset = 0x4200, .width = 16, .height = 16, .format = FORMAT_RGBA },
        {.offset = 0x4400, .width = 16, .height = 16, .format = FORMAT_RGBA },
        {.offset = 0x4600, .width = 16, .height = 16, .format = FORMAT_RGBA },
        {.offset = 0x4800, .width = 16, .height = 16, .format = FORMAT_RGBA },
    }
};

/** Coin texture */
SM64TextureAtlasInfo coin_atlas_info = {
    .offset = 0x201410,
    .numUsedTextures = 4,
    .atlasWidth = 4 * 32,
    .atlasHeight = 32,
    .texInfos = {
        {.offset = 0x5780, .width = 32, .height = 32, .format = FORMAT_IA },
        {.offset = 0x5F80, .width = 32, .height = 32, .format = FORMAT_IA },
        {.offset = 0x6780, .width = 32, .height = 32, .format = FORMAT_IA },
        {.offset = 0x6F80, .width = 32, .height = 32, .format = FORMAT_IA },
    }
};


/**
	SM64 audio thread
*/

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
      mario_texture = new uint8_t[4 * mario_atlas_info.atlasWidth * mario_atlas_info.atlasHeight];
      health_texture = new uint8_t[4 * health_atlas_info.atlasWidth * health_atlas_info.atlasHeight];
      ui_texture = new uint8_t[4 * ui_atlas_info.atlasWidth * ui_atlas_info.atlasHeight];
      coin_texture = new uint8_t[4 * coin_atlas_info.atlasWidth * coin_atlas_info.atlasHeight];

      /** load libsm64 */
      sm64_global_init(romBuffer);

      /** load textures from ROM */
      sm64_texture_load(romBuffer, &mario_atlas_info, mario_texture);
      sm64_texture_load(romBuffer, &health_atlas_info, health_texture);
      sm64_texture_load(romBuffer, &ui_atlas_info, ui_texture);
      sm64_texture_load(romBuffer, &coin_atlas_info, coin_texture);

      /** init SM64 audio */
      sm64_audio_init(romBuffer);
      audio_init();
      sm64_set_sound_volume(g_config->sound_volume / 100.f);
      sm64_play_sound_global(SOUND_MENU_STAR_SOUND);

      /** load textures into opengl */
	  VideoSystem::current()->init_sm64_texture(mario_texture, &mario_texture_handle, mario_atlas_info.atlasWidth, mario_atlas_info.atlasHeight, true);
	  VideoSystem::current()->init_sm64_texture(health_texture, &health_texture_handle, health_atlas_info.atlasWidth, health_atlas_info.atlasHeight, false);
	  VideoSystem::current()->init_sm64_texture(ui_texture, &ui_texture_handle, ui_atlas_info.atlasWidth, ui_atlas_info.atlasHeight, false);
	  VideoSystem::current()->init_sm64_texture(coin_texture, &coin_texture_handle, coin_atlas_info.atlasWidth, coin_atlas_info.atlasHeight, true);
      for(int i=0; i<3*SM64_GEO_MAX_TRIANGLES; i++) mario_indices[i] = i;

      loaded = true;
    }

    delete[] romBuffer;
  }
}

MarioManager::~MarioManager()
{
  sm64_global_terminate();

  VideoSystem::current()->destroy_sm64_texture(&mario_texture_handle);
  VideoSystem::current()->destroy_sm64_texture(&health_texture_handle);
  VideoSystem::current()->destroy_sm64_texture(&ui_texture_handle);
  VideoSystem::current()->destroy_sm64_texture(&coin_texture_handle);

  delete[] mario_texture;
  delete[] health_texture;
  delete[] ui_texture;
  delete[] coin_texture;
}

/* EOF */
