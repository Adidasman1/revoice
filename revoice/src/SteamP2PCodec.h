#pragma once

#include "revoice_shared.h"
#include "VoiceEncoder_Silk.h"

class CSteamP2PCodec : public IVoiceCodec {
public:
	CSteamP2PCodec(VoiceEncoder_Silk* backend);
	virtual bool Init(int quality);
	virtual void Release();
	virtual int Compress(const char *pUncompressedBytes, int nSamples, char *pCompressed, int maxCompressedBytes, bool bFinal);
	virtual int Decompress(const char *pCompressed, int compressedBytes, char *pUncompressed, int maxUncompressedBytes);
	virtual bool ResetState();

	VoiceEncoder_Silk* GetSilkCodec() const { return m_BackendCodec; }

private:
	int StreamDecode(const char *pCompressed, int compressedBytes, char *pUncompressed, int maxUncompressedBytes) const;
	int StreamEncode(const char *pUncompressedBytes, int nSamples, char *pCompressed, int maxCompressedBytes, bool bFinal) const;

private:
	VoiceEncoder_Silk* m_BackendCodec;
};
