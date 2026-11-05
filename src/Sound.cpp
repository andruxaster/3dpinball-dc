#include "pch.h"
#include "loader.h"
#include "Sound.h"
#include "winmain.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include <kos.h>
#include <dc/sound/aica_comm.h>
#include <dc/sound/sound.h>
#include <dc/sound/sfxmgr.h>

static constexpr int AICA_CHANNELS = 64;

int Sound::num_channels;
bool Sound::enabled_flag = false;
int* Sound::TimeStamps = nullptr;

/*
 *    temporary band-aid fix for loading 8-bit mono PCM samples
 *    they appear to not be loaded correctly through KOS' snd_sfx_load() function
 *    most of this code is from snd_sfxmgr.c with some modifications
 *    (https://github.com/KallistiOS/KallistiOS/blob/master/kernel/arch/dreamcast/sound/snd_sfxmgr.c)
 */

struct snd_effect;
LIST_HEAD(selist, snd_effect);

extern struct selist snd_effects;
	
typedef struct snd_effect {
    uint32_t  locl, locr;
    uint32_t  len;
    uint32_t  rate;
    uint32_t  used;
    uint32_t  fmt;
    uint16_t  stereo;

    LIST_ENTRY(snd_effect)  list;
} snd_effect_t;

static snd_effect_t *create_snd_effect(const int16_t *samples, uint32_t frames, uint32_t rate) {
    snd_effect_t *effect;
    uint32_t bytes = frames * sizeof(int16_t);

    effect = (snd_effect_t*)malloc(sizeof(snd_effect_t));
    if(effect == NULL)
        return NULL;

    memset(effect, 0, sizeof(snd_effect_t));

    // Samples are always converted to 16-bit mono before upload
    effect->rate = rate;
    effect->stereo = 0;
    effect->fmt = AICA_SM_16BIT;
    effect->len = frames;

    // spu_memload_sq() transfers in 32-byte blocks
    effect->locl = snd_mem_malloc((bytes + 31) & ~31);
    if(!effect->locl) {
        free(effect);
        return NULL;
    }
    spu_memload_sq(effect->locl, (void*)samples, (bytes + 31) & ~31);

    effect->locr = 0;
    return effect;
}

static sfxhnd_t snd_sfx_load_alt(const char *fn) {
    drwav wavfp;
    snd_effect_t *effect;

    if (!drwav_init_file(&wavfp, fn, NULL))
		return SFXHND_INVALID;

	const uint32_t frames = (uint32_t)wavfp.totalPCMFrameCount;
	const uint32_t channels = wavfp.channels ? wavfp.channels : 1;
	const uint32_t rate = wavfp.sampleRate;
	if (!frames)
	{
		drwav_uninit(&wavfp);
		return SFXHND_INVALID;
	}

	// Read any source format (8-bit, 16-bit...) as signed 16-bit.
	// Padded to a 32-byte multiple for the store-queue upload.
	const uint32_t paddedFrames = (frames + 15) & ~15u;
	int16_t* raw = new int16_t[(size_t)paddedFrames * channels]();
	uint32_t totalRead = (uint32_t)drwav_read_pcm_frames_s16(&wavfp, frames, raw);
	drwav_uninit(&wavfp);
	if (!totalRead)
	{
		delete[] raw;
		return SFXHND_INVALID;
	}

	// Downmix to mono if needed
	int16_t* mono = raw;
	if (channels > 1)
	{
		mono = new int16_t[paddedFrames]();
		for (uint32_t i = 0; i < totalRead; i++)
		{
			int sum = 0;
			for (uint32_t c = 0; c < channels; c++)
				sum += raw[i * channels + c];
			mono[i] = (int16_t)(sum / (int)channels);
		}
		delete[] raw;
	}

    /* Create and initialize sound effect */
    effect = create_snd_effect(mono, totalRead, rate);
	delete[] mono;
    if(!effect)
        return SFXHND_INVALID;

    LIST_INSERT_HEAD(&snd_effects, effect, list);

    return (sfxhnd_t)effect;
}


bool Sound::Init(int channels, bool enableFlag)
{
	snd_init();

	SetChannels(channels);
	Enable(enableFlag);
	return true;
}

void Sound::Enable(bool enableFlag)
{
	enabled_flag = enableFlag;
}

void Sound::Activate()
{
	
}

void Sound::Deactivate()
{
	
}

void Sound::Close()
{
	snd_sfx_unload_all();
	snd_shutdown();

	delete[] TimeStamps;
	TimeStamps = nullptr;
}

void Sound::PlaySound(uint8_t* buf, int time, int size, int samplerate)
{
	if (!enabled_flag || !buf) return;
	int channel = snd_sfx_play((sfxhnd_t)buf, 255, 128);
	// KOS returns an AICA channel number (0..63), not an index in [0, num_channels).
	// Writing TimeStamps[channel] used to overflow the heap buffer and corrupt
	// nearby objects (e.g. text box layout), see "Known issues" in README.
	if (channel >= 0 && channel < AICA_CHANNELS)
		TimeStamps[channel] = time;
}

uint8_t* Sound::LoadWaveFile(const std::string& lpName)
{
	sfxhnd_t snd = snd_sfx_load_alt(lpName.c_str());
	if (!snd)
		printf("Failed to load sound '%s'\n", lpName.c_str());
	return (uint8_t*)snd;
}

void Sound::FreeSound(uint8_t* wave)
{
	if (wave)
		snd_sfx_unload((sfxhnd_t)wave);
}

void Sound::SetChannels(int channels)
{
	if (channels <= 0)
		channels = 8;

	num_channels = channels;
	delete[] TimeStamps;
	TimeStamps = new int[AICA_CHANNELS]();
}
