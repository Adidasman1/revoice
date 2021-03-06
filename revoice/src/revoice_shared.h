#pragma once

#include "rehlds_api.h"

#define MAX_PLAYERS					32

#define LOG_PREFIX					"[REVOICE]: "

#define MAX_SILK_DATA_LEN			650
#define MAX_SPEEX_DATA_LEN			228
#define MAX_SILK_VOICE_RATE			3800
#define MAX_SPEEX_VOICE_RATE		2014

#define SILK_VOICE_QUALITY			5
#define SPEEX_VOICE_QUALITY			5

enum revoice_log_mode {
	rl_none = 0,
	rl_console = 1,
	rl_logfile = 2,
};

enum revoice_codec_type {
	vct_none,
	vct_silk,
	vct_speex,
};

enum svc_messages {
	svc_voiceinit = 52,
	svc_voicedata = 53
};

template <typename T>
T _min(T a, T b) {
	return (a < b) ? a : b;
}

template <typename T>
T _max(T a, T b) {
	return (a < b) ? b : a;
}

template <typename T>
T clamp(T a, T min, T max) {
	return (a > max) ? max : (a < min) ? min : a;
}

extern char* trimbuf(char *str);
extern void LCPrintf(bool critical, const char *fmt, ...);
extern uint32 crc32(const void* buf, unsigned int bufLen);

extern bool Revoice_Load();
extern bool Revoice_Utils_Init();
extern void util_syserror(const char* fmt, ...);
