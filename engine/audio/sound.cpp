#include "audio/sound.h"
#include <fstream>
#include <cmath>
#include <algorithm>

namespace p0::audio {

Sound::Sound(const std::string& filename) {
  load_wav(filename);
}

bool Sound::load_wav(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) return false;

  WavHeader header;
  file.read(reinterpret_cast<char*>(&header), sizeof(header));

  if (std::strncmp(header.riff, "RIFF", 4) != 0 ||
      std::strncmp(header.wave, "WAVE", 4) != 0) {
    return false;
  }

  if (header.bits_per_sample != 16) return false;

  sample_rate_ = header.sample_rate;
  channels_ = header.num_channels;
  bits_per_sample_ = header.bits_per_sample;

  file.seekg(header.fmt_size - 16, std::ios::cur);

  char chunk_id[4];
  uint32_t chunk_size;
  while (file.read(chunk_id, 4)) {
    file.read(reinterpret_cast<char*>(&chunk_size), 4);
    if (std::strncmp(chunk_id, "data", 4) == 0) {
      header.data_size = chunk_size;
      break;
    }
    file.seekg(chunk_size, std::ios::cur);
  }

  if (header.data_size == 0) return false;

  size_t num_samples = header.data_size / (bits_per_sample_ / 8);
  samples_.resize(num_samples);
  file.read(reinterpret_cast<char*>(samples_.data()), header.data_size);

  return !samples_.empty();
}

void Sound::create_from_samples(const std::vector<int16_t>& samples, uint32_t sample_rate) {
  samples_ = samples;
  sample_rate_ = sample_rate;
  channels_ = 1;
  bits_per_sample_ = 16;
}

void Sound::create_silence(double duration_seconds, uint32_t sample_rate) {
  sample_rate_ = sample_rate;
  channels_ = 1;
  bits_per_sample_ = 16;
  size_t num_samples = static_cast<size_t>(duration_seconds * sample_rate);
  samples_.resize(num_samples, 0);
}

double Sound::duration() const {
  if (sample_rate_ == 0 || channels_ == 0) return 0.0;
  return static_cast<double>(samples_.size()) / (sample_rate_ * channels_);
}

int16_t Sound::sample_at(size_t index) const {
  if (samples_.empty()) return 0;
  if (looping_) {
    index = index % samples_.size();
  } else if (index >= samples_.size()) {
    return 0;
  }
  return samples_[index];
}

double Sound::sample_at_normalized(double position) const {
  if (samples_.empty()) return 0.0;
  double idx = position * (samples_.size() - 1);
  size_t i = static_cast<size_t>(std::floor(idx));
  size_t j = i + 1;
  double frac = idx - i;

  if (j >= samples_.size()) j = looping_ ? 0 : i;
  double s0 = static_cast<double>(samples_[i]) / 32768.0;
  double s1 = static_cast<double>(samples_[j]) / 32768.0;
  return s0 + frac * (s1 - s0);
}

}
