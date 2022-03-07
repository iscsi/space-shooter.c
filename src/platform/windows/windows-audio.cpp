////////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
// 
// Copyright (c) 2021 Tarek Sherif
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Uses Xaudio2:
// - https://docs.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-introduction
//////////////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xaudio2.h>
#include <stdbool.h>
#include "../../shared/constants.h"
#include "../../shared/data.h"
#include "../../shared/platform-interface.h"
#include "windows-audio.h"

static WAVEFORMATEX AUDIO_SOURCE_FORMAT = {
    .wFormatTag = WAVE_FORMAT_PCM,
    .nChannels = SPACE_SHOOTER_AUDIO_CHANNELS,
    .nSamplesPerSec = SPACE_SHOOTER_AUDIO_SAMPLE_RATE,
    .nAvgBytesPerSec = SPACE_SHOOTER_AUDIO_SAMPLE_RATE * SPACE_SHOOTER_AUDIO_CHANNELS * (SPACE_SHOOTER_AUDIO_BPS / 8),
    .nBlockAlign = 4,
    .wBitsPerSample = SPACE_SHOOTER_AUDIO_BPS,
    .cbSize = 0
};

typedef struct {
    IXAudio2SourceVoice* voice;
    XAUDIO2_BUFFER buffer;
    bool inUse;
} AudioStream;

void WINAPI OnBufferEnd(IXAudio2VoiceCallback* This, void* pBufferContext)    {
    AudioStream* channel = (AudioStream*) pBufferContext;
    channel->inUse = false;
}

class VoiceCallback : public IXAudio2VoiceCallback
{
public:
	HANDLE hBufferEndEvent;
	VoiceCallback() : hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL)) {}
	~VoiceCallback() { CloseHandle(hBufferEndEvent); }

	//Called when the voice has just finished playing a contiguous audio stream.
	void OnStreamEnd() { SetEvent(hBufferEndEvent); }

	//Unused methods are stubs
	void OnVoiceProcessingPassEnd() { }
	void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {    }
	void OnBufferEnd(void* pBufferContext) { }
	void OnBufferStart(void* pBufferContext) {    }
	void OnLoopEnd(void* pBufferContext) {    }
	void OnVoiceError(void* pBufferContext, HRESULT Error) { }
};

static struct {
    IXAudio2* xaudio;
    IXAudio2MasteringVoice* xaudioMasterVoice;
    AudioStream channels[SPACE_SHOOTER_AUDIO_MIXER_CHANNELS];
    VoiceCallback* callbacks;
} audio;

bool windows_initAudio(void) {
    HRESULT comResult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(comResult)) {
        goto ERROR_NO_RESOURCES;
    }
    
    comResult = XAudio2Create(&audio.xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(comResult)) {
        goto ERROR_COM;
    }

	comResult = audio.xaudio->CreateMasteringVoice(&audio.xaudioMasterVoice,
		SPACE_SHOOTER_AUDIO_CHANNELS,
		SPACE_SHOOTER_AUDIO_SAMPLE_RATE,
		0,
		NULL,
		NULL,
		AudioCategory_GameEffects);
    if (FAILED(comResult)) {
        goto ERROR_XAUDIO2;
    }

    audio.callbacks = new VoiceCallback();

    for (int32_t i = 0; i < SPACE_SHOOTER_AUDIO_MIXER_CHANNELS; ++i) {
        audio.channels[i].buffer.Flags = XAUDIO2_END_OF_STREAM;
        audio.channels[i].buffer.pContext = audio.channels + i;

        comResult = audio.xaudio->CreateSourceVoice(
            &audio.channels[i].voice,
            &AUDIO_SOURCE_FORMAT,
            0,
            XAUDIO2_DEFAULT_FREQ_RATIO,
            audio.callbacks,
            NULL,
            NULL
        );

        if (FAILED(comResult)) {
            goto ERROR_XAUDIO2;
        }
    }

    /////////////
    // Success!
    /////////////

    return true;

    ///////////////////
    // Error handling
    ///////////////////

    ERROR_XAUDIO2:
    audio.xaudio->Release();
    audio.xaudio = NULL;

    delete audio.callbacks;
    audio.callbacks = nullptr;

    ERROR_COM:
    CoUninitialize();

    ERROR_NO_RESOURCES:
    return false;
}

void platform_playSound(Data_Buffer* sound, bool loop) {
    if (!audio.xaudio) {
        return;
    }

    for (int32_t i = 0; i < SPACE_SHOOTER_AUDIO_MIXER_CHANNELS; ++i) {
        if (!audio.channels[i].inUse) {
            XAUDIO2_BUFFER* buffer = &audio.channels[i].buffer;
            buffer->LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;
            buffer->AudioBytes = sound->size;
            buffer->pAudioData = sound->data;
            audio.channels[i].voice->Start(0, XAUDIO2_COMMIT_NOW);
            audio.channels[i].voice->SubmitSourceBuffer(buffer, NULL);
            audio.channels[i].inUse = true;
            break;
        }
    }
}

void windows_closeAudio(void) {
    if (!audio.xaudio) {
        return;
    }

    for (int32_t i = 0; i < SPACE_SHOOTER_AUDIO_MIXER_CHANNELS; ++i) {
        if (audio.channels[i].inUse) {
            audio.channels[i].voice->Stop(0, XAUDIO2_COMMIT_NOW);
            audio.channels[i].inUse = false;
            break;
        }
    }

    audio.xaudio->Release();
    audio.xaudio = NULL;
    delete audio.callbacks;
    audio.callbacks = nullptr;
    CoUninitialize();
}
