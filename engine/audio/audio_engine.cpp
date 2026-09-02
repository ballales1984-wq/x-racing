#include "audio/audio_engine.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace p0::audio {

//! @brief Default constructor.
AudioEngine::AudioEngine() = default;

//! @brief Destructor. Ensures clean shutdown of audio system.
AudioEngine::~AudioEngine() {
  shutdown();
}

//! @brief Initializes the audio engine with specified sample rate and channel count.
//! @param sample_rate Output sample rate in Hz (default: 44100).
//! @param channels Number of audio channels (default: 2 for stereo).
//! @return true if initialization succeeded.
bool AudioEngine::initialize(uint32_t sample_rate, uint16_t channels) {
  sample_rate_ = sample_rate;
  channels_ = channels;
  initialized_ = true;
  return true;
}

//! @brief Shuts down the audio engine and clears all active sounds.
void AudioEngine::shutdown() {
  std::lock_guard<std::mutex> lock(mutex_);
  sounds_.clear();
  initialized_ = false;
}

//! @brief Plays a sound and returns its instance ID.
//! @param sound The sound resource to play.
//! @param type The sound category for grouping.
//! @param volume Playback volume (0.0 to 2.0).
//! @param loop Whether the sound should loop.
//! @return Instance ID for controlling the sound, or -1 on failure.
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

//! @brief Stops a specific sound instance by ID.
//! @param id The instance ID returned by play_sound.
void AudioEngine::stop_sound(int id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = std::remove_if(sounds_.begin(), sounds_.end(),
                           [id](const SoundInstance& s) { return s.id == id; });
  sounds_.erase(it, sounds_.end());
}

//! @brief Stops all currently playing sounds.
void AudioEngine::stop_all() {
  std::lock_guard<std::mutex> lock(mutex_);
  sounds_.clear();
}

//! @brief Stops all sounds of a specific type.
//! @param type The sound type to stop.
void AudioEngine::stop_by_type(SoundType type) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = std::remove_if(sounds_.begin(), sounds_.end(),
                           [type](const SoundInstance& s) { return s.type == type; });
  sounds_.erase(it, sounds_.end());
}

//! @brief Sets the volume for a specific sound instance.
//! @param id The instance ID.
//! @param volume New volume level (clamped to 0.0-2.0).
void AudioEngine::set_volume(int id, double volume) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& s : sounds_) {
    if (s.id == id) {
      s.volume = std::clamp(volume, 0.0, 2.0);
      break;
    }
  }
}

//! @brief Sets the pitch for a specific sound instance.
//! @param id The instance ID.
//! @param pitch New pitch multiplier (clamped to 0.1-4.0).
void AudioEngine::set_pitch(int id, double pitch) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& s : sounds_) {
    if (s.id == id) {
      s.pitch = std::clamp(pitch, 0.1, 4.0);
      break;
    }
  }
}

//! @brief Updates engine sound parameters from vehicle state.
//! @param params Engine sound parameters including RPM and load.
void AudioEngine::set_engine_params(const EngineSoundParams& params) {
  engine_params_ = params;
  engine_playing_ = params.rpm > 100.0;
}

//! @brief Sets the listener position for 3D audio spatialization.
//! @param x X coordinate.
//! @param y Y coordinate.
//! @param z Z coordinate.
void AudioEngine::set_listener_position(double x, double y, double z) {
  listener_x_ = x;
  listener_y_ = y;
  listener_z_ = z;
}

//! @brief Sets the listener forward direction for 3D audio.
//! @param forward_x X component of forward vector.
//! @param forward_y Y component of forward vector.
//! @param forward_z Z component of forward vector.
void AudioEngine::set_listener_orientation(double forward_x, double forward_y, double forward_z) {
  double len = std::sqrt(forward_x * forward_x + forward_y * forward_y + forward_z * forward_z);
  if (len > 0.001) {
    listener_fwd_x_ = forward_x / len;
    listener_fwd_y_ = forward_y / len;
    listener_fwd_z_ = forward_z / len;
  }
}

//! @brief Main audio mixing callback. Mixes all active sounds into the output buffer.
//! @param output_buffer Output PCM buffer (interleaved channels).
//! @param num_frames Number of audio frames to generate.
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

//! @brief Updates sound instance positions and removes finished sounds.
//! @param delta_time Time elapsed since last update in seconds.
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

//! @brief Returns the number of currently active sound instances.
//! @return Count of active sounds.
size_t AudioEngine::active_sound_count() const {
  return sounds_.size();
}

//! @brief Mixes a single sound instance into the output buffer with linear interpolation.
//! @param instance The sound instance to mix.
//! @param output Output PCM buffer.
//! @param frames Number of frames to process.
//! @param channels Number of audio channels.
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

//! @brief Synthesizes engine sound using additive synthesis.
//!        Generates fundamental, harmonics, cylinder pulse, and noise.
//! @param output Output PCM buffer.
//! @param frames Number of frames to generate.
//! @param channels Number of audio channels.
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

//! @brief Applies distance-based volume attenuation for 3D spatial audio.
//! @param instance The sound instance to modify.
void AudioEngine::apply_spatial_attenuation(SoundInstance& instance) {
  double dx = instance.pos_x - listener_x_;
  double dy = instance.pos_y - listener_y_;
  double dz = instance.pos_z - listener_z_;
  double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
  double attenuation = 1.0 / (1.0 + dist * 0.1);
  instance.volume *= attenuation;
}

}