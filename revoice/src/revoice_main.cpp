#include "precompiled.h"

cvar_t* pcv_sv_voiceenable = NULL;

void SV_DropClient_hook(IRehldsHook_SV_DropClient* chain, IGameClient* cl, bool crash, const char* msg)
{
	CRevoicePlayer* plr = GetPlayerByClientPtr(cl);

	plr->OnDisconnected();

	chain->callNext(cl, crash, msg);
}

void mm_CvarValue2(const edict_t *pEnt, int requestID, const char *cvarName, const char *cvarValue)
{
	CRevoicePlayer* plr = GetPlayerByEdict(pEnt);

	if (plr->GetRequestId() != requestID) {
		RETURN_META(MRES_IGNORED);
	}

	const char* lastSeparator = strrchr(cvarValue, ',');

	if (lastSeparator)
	{
		int buildNumber = atoi(lastSeparator + 1);

		if (buildNumber > 4554) {
			plr->SetCodecType(vct_silk);
		}
	}

	RETURN_META(MRES_IGNORED);
}

int TranscodeVoice(const char* srcBuf, int srcBufLen, IVoiceCodec* srcCodec, IVoiceCodec* dstCodec, char* dstBuf, int dstBufSize)
{
	char decodedBuf[32768];

	int numDecodedSamples = srcCodec->Decompress(srcBuf, srcBufLen, decodedBuf, sizeof(decodedBuf));

	if (numDecodedSamples <= 0) {
		return 0;
	}

	int compressedSize = dstCodec->Compress(decodedBuf, numDecodedSamples, dstBuf, dstBufSize, false);

	if (compressedSize <= 0) {
		return 0;
	}

	/*
	int numDecodedSamples2 = dstCodec->Decompress(dstBuf, compressedSize, decodedBuf, sizeof(decodedBuf));
	if (numDecodedSamples2 <= 0) {
		return compressedSize;
	}

	FILE* rawSndFile = fopen("d:\\revoice_raw.snd", "ab");
	if (rawSndFile) {
		fwrite(decodedBuf, 2, numDecodedSamples2, rawSndFile);
		fclose(rawSndFile);
	}
	*/

	return compressedSize;
}

void SV_ParseVoiceData_emu(IGameClient* cl)
{
	char chReceived[4096];
	unsigned int nDataLength = g_RehldsFuncs->MSG_ReadShort();

	if (nDataLength > sizeof(chReceived)) {
		g_RehldsFuncs->DropClient(cl, FALSE, "Invalid voice data\n");
		return;
	}

	g_RehldsFuncs->MSG_ReadBuf(nDataLength, chReceived);

	if (pcv_sv_voiceenable->value == 0.0f) {
		return;
	}

	CRevoicePlayer* srcPlayer = GetPlayerByClientPtr(cl);
	srcPlayer->SetLastVoiceTime(g_RehldsSv->GetTime());
	srcPlayer->IncreaseVoiceRate(nDataLength);

	char transcodedBuf[4096];
	char* speexData; int speexDataLen;
	char* silkData; int silkDataLen;

	switch (srcPlayer->GetCodecType())
	{
		case vct_silk:
		{
			if (nDataLength > MAX_SILK_DATA_LEN || srcPlayer->GetVoiceRate() > MAX_SILK_VOICE_RATE)
				return;

			silkData = chReceived; silkDataLen = nDataLength;
			speexData = transcodedBuf;
			speexDataLen = TranscodeVoice(silkData, silkDataLen, srcPlayer->GetSilkCodec(), srcPlayer->GetSpeexCodec(), transcodedBuf, sizeof(transcodedBuf));
			break;
		}

		case vct_speex:
			if (nDataLength > MAX_SPEEX_DATA_LEN || srcPlayer->GetVoiceRate() > MAX_SPEEX_VOICE_RATE)
				return;

			speexData = chReceived; speexDataLen = nDataLength;
			silkData = transcodedBuf;
			silkDataLen = TranscodeVoice(speexData, speexDataLen, srcPlayer->GetSpeexCodec(), srcPlayer->GetSilkCodec(), transcodedBuf, sizeof(transcodedBuf));
			break;

		default:
			return;
	}

	int maxclients = g_RehldsSvs->GetMaxClients();

	for (int i = 0; i < maxclients; i++)
	{
		CRevoicePlayer* dstPlayer = &g_Players[i];
		IGameClient* dstClient = dstPlayer->GetClient();

		if (!((1 << i) & cl->GetVoiceStream(0)) && dstPlayer != srcPlayer)
			continue;

		if (!dstClient->IsActive() && !dstClient->IsConnected() && dstPlayer != srcPlayer)
			continue;

		char* sendBuf; int nSendLen;

		switch (dstPlayer->GetCodecType())
		{
			case vct_silk:
				sendBuf = silkData; nSendLen = silkDataLen;
				break;

			case vct_speex:
				sendBuf = speexData; nSendLen = speexDataLen;
				break;

			default:
				sendBuf = NULL; nSendLen = 0;
				break;
		}

		if (sendBuf == NULL || nSendLen == 0)
			continue;

		if (dstPlayer == srcPlayer && !dstClient->GetLoopback())
			nSendLen = 0;

		sizebuf_t* dstDatagram = dstClient->GetDatagram();
		if (dstDatagram->cursize + nSendLen + 6 < dstDatagram->maxsize) {
			g_RehldsFuncs->MSG_WriteByte(dstDatagram, svc_voicedata); //svc_voicedata
			g_RehldsFuncs->MSG_WriteByte(dstDatagram, cl->GetId());
			g_RehldsFuncs->MSG_WriteShort(dstDatagram, nSendLen);
			g_RehldsFuncs->MSG_WriteBuf(dstDatagram, nSendLen, sendBuf);
		}
	}
}

void Rehlds_HandleNetCommand(IRehldsHook_HandleNetCommand* chain, IGameClient* cl, int8 opcode)
{
	if (opcode == 8) { //clc_voicedata
		SV_ParseVoiceData_emu(cl);
		return;
	}

	chain->callNext(cl, opcode);
}

qboolean mm_ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128]) {

	CRevoicePlayer* plr = GetPlayerByEdict(pEntity);

	plr->OnConnected();

	RETURN_META_VALUE(MRES_IGNORED, TRUE);
}

void SV_WriteVoiceCodec_hooked(IRehldsHook_SV_WriteVoiceCodec* chain, sizebuf_t* sb)
{
	IGameClient* cl = g_RehldsFuncs->GetHostClient();
	CRevoicePlayer* plr = GetPlayerByClientPtr(cl);

	switch (plr->GetCodecType())
	{
		case vct_silk:
			g_RehldsFuncs->MSG_WriteByte(sb, svc_voiceinit); //svc_voiceinit
			g_RehldsFuncs->MSG_WriteString(sb, ""); //codec id
			g_RehldsFuncs->MSG_WriteByte(sb, 0); //quality
			break;

		case vct_speex:
			g_RehldsFuncs->MSG_WriteByte(sb, svc_voiceinit); //svc_voiceinit
			g_RehldsFuncs->MSG_WriteString(sb, "voice_speex"); //codec id
			g_RehldsFuncs->MSG_WriteByte(sb, 5); //quality
			break;

		default:
			LCPrintf(true, "SV_WriteVoiceCodec() called on client(%d) with unknown voice codec\n", cl->GetId());
			break;
	}
}

bool Revoice_Main_Init()
{
	g_RehldsHookchains->SV_DropClient()->registerHook(SV_DropClient_hook, HC_PRIORITY_DEFAULT + 1);
	g_RehldsHookchains->HandleNetCommand()->registerHook(Rehlds_HandleNetCommand, HC_PRIORITY_DEFAULT + 1);
	g_RehldsHookchains->SV_WriteVoiceCodec()->registerHook(SV_WriteVoiceCodec_hooked, HC_PRIORITY_DEFAULT + 1);

	pcv_sv_voiceenable = g_engfuncs.pfnCVarGetPointer("sv_voiceenable");

	return true;
}
