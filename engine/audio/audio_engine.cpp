#include "audio/audio_engine.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace p0::audio {

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
  shutdown();
}

bool AudioEngine::initialize(uint32_t sample_rate, uint16_t channels) {
  sample_rate_ = sample_rate;
  channels_ = channels;
  initialized_ = true;
  return true;
}

void AudioEngine::shutdown() {
  std::lock_guard<std::mutex> lock(mutex_);
  sounds_.clear();
  initialized_ = false;
}

int AudioEngine::play_sound(std::shared_ptr<Sound> sound, SoundType type,
                            double volume, bool loop) {
  if (!sound || !sound->is_valid()) return -1;

  std::lock_guard<std::mutex> lock(mutex_);
  SoundInstance instance;
  instance.id = next_id_++;
  instance.type = type;
  instance.sound = sound;
  instance.position = 0.0;
  instance.volume = volume;
  instance.pitch = 1.0;
  instance.playing = true;
  instance.looping = loop || sound->looping();
  sounds_.push_back(instance);
  return instance.id;
}

void AudioEngine::stop_sound(int id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = std::remove_if(sounds_.begin(), sounds_.end(),
                           [id](const SoundInstance& s) { return s.id == id; });
  sounds_.erase(it, sounds_.end());
}

void AudioEngine::stop_all() {
  std::lock_guard<std::mutex> lock(mutex_);
  sounds_.clear();
}

void AudioEngine::stop_by_type(SoundType type) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = std::remove_if(sounds_.begin(), sounds_.end(),
                           [type](const SoundInstance& s) { return s.type == type; });
  sounds_.erase(it, sounds_.end());
}

void AudioEngine::set_volume(int id, double volume) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& s : sounds_) {
    if (s.id == id) {
      s.volume = std::clamp(volume, 0.0, 2.0);
      break;
    }
  }
}

void AudioEngine::set_pitch(int id, double pitch) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& s : sounds_) {
    if (s.id == id) {
      s.pitch = std::clamp(pitch, 0.1, 4.0);
      break;
    }
  }
}

void AudioEngine::set_engine_params(const EngineSoundParams& params) {
  engine_params_ = params;
  engine_playing_ = params.rpm > 100.0;
}

void AudioEngine::set_listener_position(double x, double y, double z) {
  listener_x_ = x;
  listener_y_ = y;
  listener_z_ = z;
}

void AudioEngine::set_listener_orientation(double forward_x, double forward_y, double forward_z) {
  double len = std::sqrt(forward_x * forward_x + forward_y * forward_y + forward_z * forward_z);
  if (len > 0.001) {
    listener_fwd_x_ = forward_x / len;
    listener_fwd_y_ = forward_y / len;
    listener_fwd_z_ = forward_z / len;
  }
}

void AudioEngine::mix_audio(int16_t* output_buffer, size_t num_frames) {
  if (!initialized_) return;

  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<double> mix_buffer(num_frames * channels_, 0.0);

  for (auto& sound : sounds_) {
    if (!sound.playing || !sound.sound) continue;
    mix_sound(sound, reinterpret_cast<int16_t*>(mix_buffer.data()), num_frames, channels_);
  }

  if (engine_playing_) {
    synthesize_engine(reinterpret_cast<int16_t*>(mix_buffer.data()), num_frames, channels_);
  }

  for (size_t i = 0; i < num_frames * channels_; ++i) {
    double sample = mix_buffer[i] * master_volume_;
    sample = std::clamp(sample, -32768.0, 32767.0);
    output_buffer[i] = static_cast<int16_t>(sample);
  }
}

void AudioEngine::update(double delta_time) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& sound : sounds_) {
    if (!sound.playing || !sound.sound) continue;
    double advance = delta_time * sound.pitch;
    sound.position += advance;
    if (sound.position >= sound.sound->duration()) {
      if (sound.looping) {
        sound.position = std::fmod(sound.position, sound.sound->duration());
      } else {
        sound.playing = false;
      }
    }
  }
  sounds_.erase(std::remove_if(sounds_.begin(), sounds_.end(),
                               [](const SoundInstance& s) { return !s.playing; }),
                sounds_.end());
}

size_t AudioEngine::active_sound_count() const {
  return sounds_.size();
}

void AudioEngine::mix_sound(SoundInstance& instance, int16_t* output, size_t frames, uint16_t channels) {
  if (!instance.sound) return;
  const auto& samples = instance.sound->samples();
  if (samples.empty()) return;

  double sample_rate = instance.sound->sample_rate();
  double pitch_scale = instance.pitch * (sample_rate / static_cast<double>(sample_rate_));

  for (size_t i = 0; i < frames; ++i) {
    double pos = instance.position * sample_rate + i * pitch_scale;
    size_t idx = static_cast<size_t>(pos);
    double frac = pos - idx;

    if (idx >= samples.size()) {
      if (instance.looping) {
        idx = idx % samples.size();
      } else {
        break;
      }
    }

    size_t idx1 = (idx + 1) % samples.size();
    double s0 = static_cast<double>(samples[idx]) / 32768.0;
    double s1 = static_cast<double>(samples[idx1]) / 32768.0;
    double sample = s0 + frac * (s1 - s0);
    sample *= instance.volume;

    for (uint16_t ch = 0; ch < channels; ++ch) {
      double existing = static_cast<double>(output[i * channels + ch]) / 32768.0;
      double mixed = existing + sample;
      output[i * channels + ch] = static_cast<int16_t>(std::clamp(mixed * 32768.0, -32768.0, 32767.0));
    }
  }
}

void AudioEngine::synthesize_engine(int16_t* output, size_t frames, uint16_t channels) {
  double rpm = engine_params_.rpm;
  double max_rpm = engine_params_.max_rpm;
  double load = engine_params_.load;
  int cylinders = engine_params_.cylinders;

  double base_freq = engine_params_.base_frequency * (rpm / 800.0);
  double sample_rate = static_cast<double>(sample_rate_);

  for (size_t i = 0; i < frames; ++i) {
    double t = engine_phase_ / sample_rate;
    double sample = 0.0;

    double fundamental = std::sin(2.0 * M_PI * base_freq * t);
    sample += fundamental * 0.5;

    double harmonic2 = std::sin(2.0 * M_PI * base_freq * 2.0 * t);
    sample += harmonic2 * 0.25;

    double harmonic4 = std::sin(2.0 * M_PI * base_freq * 4.0 * t);
    sample += harmonic4 * 0.125;

    double cylinder_freq = (rpm / 60.0) * (cylinders / 2.0);
    double cylinder_pulse = std::sin(2.0 * M_PI * cylinder_freq * t);
    sample += cylinder_pulse * 0.3 * load;

    double noise = (static_cast<double>(rand()) / RAND_MAX - 0.5) * 0.1;
    sample += noise * (0.5 + load * 0.5);

    sample *= engine_params_.volume;

    for (uint16_t ch = 0; ch < channels; ++ch) {
      double existing = static_cast<double>(output[i * channels + ch]) / 32768.0;
      double mixed = existing + sample;
      output[i * channels + ch] = static_cast<int16_t>(std::clamp(mixed * 32768.0, -32768.0, 32767.0));
    }

    engine_phase_ += 1.0;
  }
}

void AudioEngine::apply_spatial_attenuation(SoundInstance& instance) {
  double dx = instance.pos_x - listener_x_;
  double dy = instance.pos_y - listener_y_;
  double dz = instance.pos_z - listener_z_;
  double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
  double attenuation = 1.0 / (1.0 + dist * 0.1);
  instance.volume *= attenuation;
}

}
