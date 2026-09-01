#pragma once

#include "audio/audio_engine.h"
#include <windows.h>
#include <mmsystem.h>

namespace p0::audio {

class WindowsAudioOutput {
 public:
  WindowsAudioOutput() = default;
  ~WindowsAudioOutput();

  bool initialize(AudioEngine& engine, uint32_t sample_rate = 44100, uint16_t channels = 2);
  void shutdown();

  void start();
  void stop();

 private:
  static void CALLBACK wave_out_proc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance,
                                     DWORD_PTR dwParam1, DWORD_PTR dwParam2);

  AudioEngine* engine_ = nullptr;
  HWAVEOUT wave_out_ = nullptr;
  WAVEFORMATEX format_ {};
  bool running_ = false;
};

}
