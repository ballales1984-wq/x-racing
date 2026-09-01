#include "audio/windows_audio_output.h"
#include <algorithm>

namespace p0::audio {

WindowsAudioOutput::~WindowsAudioOutput() {
  shutdown();
}

bool WindowsAudioOutput::initialize(AudioEngine& engine, uint32_t sample_rate, uint16_t channels) {
  engine_ = &engine;
  format_.wFormatTag = WAVE_FORMAT_PCM;
  format_.nChannels = channels;
  format_.nSamplesPerSec = sample_rate;
  format_.wBitsPerSample = 16;
  format_.nBlockAlign = (format_.nChannels * format_.wBitsPerSample) / 8;
  format_.nAvgBytesPerSec = format_.nSamplesPerSec * format_.nBlockAlign;
  format_.cbSize = 0;

  MMRESULT result = waveOutOpen(&wave_out_, WAVE_MAPPER, &format_, 0, 0, CALLBACK_NULL);
  if (result != MMSYSERR_NOERROR) {
    wave_out_ = nullptr;
    return false;
  }

  return true;
}

void WindowsAudioOutput::shutdown() {
  stop();
  if (wave_out_) {
    waveOutClose(wave_out_);
    wave_out_ = nullptr;
  }
  engine_ = nullptr;
}

void WindowsAudioOutput::start() {
  if (!wave_out_ || !engine_ || running_) return;
  running_ = true;

  constexpr size_t buffer_frames = 4096;
  const size_t buffer_size = buffer_frames * format_.nBlockAlign;
  std::vector<int16_t> buffer(buffer_frames * format_.nChannels);

  while (running_) {
    engine_->mix_audio(buffer.data(), buffer_frames);
    WAVEHDR header {};
    header.lpData = reinterpret_cast<LPSTR>(buffer.data());
    header.dwBufferLength = static_cast<DWORD>(buffer_size);
    header.dwFlags = 0;

    MMRESULT result = waveOutPrepareHeader(wave_out_, &header, sizeof(header));
    if (result != MMSYSERR_NOERROR) break;

    result = waveOutWrite(wave_out_, &header, sizeof(header));
    if (result != MMSYSERR_NOERROR) break;

    while (!(header.dwFlags & WHDR_DONE) && running_) {
      Sleep(1);
    }

    waveOutUnprepareHeader(wave_out_, &header, sizeof(header));
  }
}

void WindowsAudioOutput::stop() {
  running_ = false;
}

void CALLBACK WindowsAudioOutput::wave_out_proc(HWAVEOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR) {
}

}
