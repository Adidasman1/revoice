#pragma once

#include "revoice_shared.h"
#include "VoiceEncoder_Silk.h"
#include "SteamP2PCodec.h"
#include "VoiceEncoder_Speex.h"
#include "voice_codec_frame.h"

class CRevoicePlayer {
private:
	IGameClient* m_RehldsClient;
	revoice_codec_type m_CodecType;
	CSteamP2PCodec* m_SilkCodec;
	VoiceCodec_Frame* m_SpeexCodec;
	int m_Protocol;
	int m_VoiceRate;
	int m_RequestId;
	bool m_Connected;

public:
	CRevoicePlayer();
	void Initialize(IGameClient* cl);
	void OnConnected();
	void OnDisconnected();

	void SetLastVoiceTime(double time);
	void UpdateVoiceRate(double delta);
	void IncreaseVoiceRate(int dataLength);

	int GetVoiceRate() const { return m_VoiceRate; }
	int GetRequestId() const { return m_RequestId; }
	bool IsConnected() const { return m_Connected; }

	void SetCodecType(revoice_codec_type codecType)		{ m_CodecType = codecType; };
	revoice_codec_type GetCodecType() const				{ return m_CodecType; }
	CSteamP2PCodec* GetSilkCodec() const				{ return m_SilkCodec; }
	VoiceCodec_Frame* GetSpeexCodec() const				{ return m_SpeexCodec;  }
	IGameClient* GetClient() const						{ return m_RehldsClient; }
};

extern CRevoicePlayer g_Players[MAX_PLAYERS];

extern void Revoice_Init_Players();
extern CRevoicePlayer* GetPlayerByClientPtr(IGameClient* cl);
extern CRevoicePlayer* GetPlayerByEdict(const edict_t* ed);
