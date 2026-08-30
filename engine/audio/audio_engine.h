#pragma once

#include "audio/sound.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <functional>

namespace p0::audio {

enum class SoundType : uint8_t {
  ENGINE,
  EXHAUST,
  TIRE_SCREECH,
  WIND,
  COLLISION,
  AMBIENT,
  UI
};

struct SoundInstance {
  int id = -1;
  SoundType type = SoundType::AMBIENT;
  std::shared_ptr<Sound> sound;
  double position = 0.0;
  double volume = 1.0;
  double pitch = 1.0;
  bool playing = false;
  bool looping = false;
  bool spatial = false;
  double pos_x = 0.0;
  double pos_y = 0.0;
  double pos_z = 0.0;
};

struct EngineSoundParams {
  double rpm = 800.0;
  double max_rpm = 8000.0;
  double load = 0.0;
  int cylinders = 8;
  double base_frequency = 60.0;
  double volume = 0.5;
};

class AudioEngine {
 public:
  AudioEngine();
  ~AudioEngine();

  bool initialize(uint32_t sample_rate = 44100, uint16_t channels = 2);
  void shutdown();

  int play_sound(std::shared_ptr<Sound> sound, SoundType type = SoundType::AMBIENT,
                 double volume = 1.0, bool loop = false);
  void stop_sound(int id);
  void stop_all();
  void stop_by_type(SoundType type);

  void set_volume(int id, double volume);
  void set_pitch(int id, double pitch);
  void set_master_volume(double volume) { master_volume_ = volume; }
  double master_volume() const { return master_volume_; }

  void set_engine_params(const EngineSoundParams& params);
  const EngineSoundParams& engine_params() const { return engine_params_; }

  void set_listener_position(double x, double y, double z);
  void set_listener_orientation(double forward_x, double forward_y, double forward_z);

  void mix_audio(int16_t* output_buffer, size_t num_frames);
  void update(double delta_time);

  size_t active_sound_count() const;

  using AudioCallback = std::function<void(int16_t*, size_t)>;
  void set_output_callback(AudioCallback cb) { output_callback_ = cb; }

 private:
  void mix_sound(SoundInstance& instance, int16_t* output, size_t frames, uint16_t channels);
  void synthesize_engine(int16_t* output, size_t frames, uint16_t channels);
  void apply_spatial_attenuation(SoundInstance& instance);

  uint32_t sample_rate_ = 44100;
  uint16_t channels_ = 2;
  double master_volume_ = 1.0;
  bool initialized_ = false;

  std::vector<SoundInstance> sounds_;
  int next_id_ = 1;

  EngineSoundParams engine_params_;
  double engine_phase_ = 0.0;
  bool engine_playing_ = false;

  double listener_x_ = 0.0;
  double listener_y_ = 0.0;
  double listener_z_ = 0.0;
  double listener_fwd_x_ = 0.0;
  double listener_fwd_y_ = 0.0;
  double listener_fwd_z_ = 1.0;

  AudioCallback output_callback_;
  mutable std::mutex mutex_;
};

}
