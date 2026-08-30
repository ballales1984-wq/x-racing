#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

namespace p0::audio {

struct WavHeader {
  char riff[4] = {'R', 'I', 'F', 'F'};
  uint32_t file_size;
  char wave[4] = {'W', 'A', 'V', 'E'};
  char fmt[4] = {'f', 'm', 't', ' '};
  uint32_t fmt_size = 16;
  uint16_t audio_format = 1;
  uint16_t num_channels = 1;
  uint32_t sample_rate = 44100;
  uint32_t byte_rate;
  uint16_t block_align;
  uint16_t bits_per_sample = 16;
  char data[4] = {'d', 'a', 't', 'a'};
  uint32_t data_size;
};

class Sound {
 public:
  Sound() = default;
  explicit Sound(const std::string& filename);
  ~Sound() = default;

  bool load_wav(const std::string& filename);
  void create_from_samples(const std::vector<int16_t>& samples, uint32_t sample_rate = 44100);
  void create_silence(double duration_seconds, uint32_t sample_rate = 44100);

  const std::vector<int16_t>& samples() const { return samples_; }
  std::vector<int16_t>& samples() { return samples_; }
  uint32_t sample_rate() const { return sample_rate_; }
  uint16_t channels() const { return channels_; }
  uint16_t bits_per_sample() const { return bits_per_sample_; }
  double duration() const;
  bool is_valid() const { return !samples_.empty(); }

  void set_looping(bool loop) { looping_ = loop; }
  bool looping() const { return looping_; }

  int16_t sample_at(size_t index) const;
  double sample_at_normalized(double position) const;

 private:
  std::vector<int16_t> samples_;
  uint32_t sample_rate_ = 44100;
  uint16_t channels_ = 1;
  uint16_t bits_per_sample_ = 16;
  bool looping_ = false;
};

}
