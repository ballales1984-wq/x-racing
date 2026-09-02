// Project 0 — Audio engine (synthesized engine + spatial sound mixer)
// Namespace: p0::audio
#pragma once

#include "audio/sound.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <functional>

namespace p0::audio {

// Categories of sound effects managed by the audio engine.
enum class SoundType : uint8_t {
  ENGINE,
  EXHAUST,
  TIRE_SCREECH,
  WIND,
  COLLISION,
  AMBIENT,
  UI
};

// Active sound instance with playback state and spatial parameters.
struct SoundInstance {
  int id = -1;
  SoundType type = SoundType::AMBIENT;
  std::shared_ptr<Sound> sound;
  double position = 0.0;  // playback cursor (s)
  double volume = 1.0;  // [0, 1]
  double pitch = 1.0;  // relative frequency multiplier
  bool playing = false;
  bool looping = false;
  bool spatial = false;
  double pos_x = 0.0;  // world position for spatialization
  double pos_y = 0.0;
  double pos_z = 0.0;
};

// Parameters controlling synthesized engine sound output.
struct EngineSoundParams {
  double rpm = 800.0;
  double max_rpm = 8000.0;
  double load = 0.0;  // throttle position [0, 1]
  int cylinders = 8;
  double base_frequency = 60.0;  // Hz, fundamental engine order
  double volume = 0.5;  // [0, 1]
};

// Real-time audio mixer with synthesized engine sound and spatial playback.
// Manages sound instances, listener position, master volume, and output callback.
class AudioEngine {
 public:
  AudioEngine();
  ~AudioEngine();

  // Initialize audio output at the given sample rate and channel count.
  bool initialize(uint32_t sample_rate = 44100, uint16_t channels = 2);
  void shutdown();

  // Play a sound and return its instance ID (-1 on failure).
  int play_sound(std::shared_ptr<Sound> sound, SoundType type = SoundType::AMBIENT,
                 double volume = 1.0, bool loop = false);
  void stop_sound(int id);
  void stop_all();
  void stop_by_type(SoundType type);

  void set_volume(int id, double volume);  // [0, 1]
  void set_pitch(int id, double pitch);  // relative multiplier
  void set_master_volume(double volume) { master_volume_ = volume; }
  double master_volume() const { return master_volume_; }

  // Update synthesized engine sound parameters (call each frame).
  void set_engine_params(const EngineSoundParams& params);
  const EngineSoundParams& engine_params() const { return engine_params_; }

  // Update listener position for spatial audio (world units).
  void set_listener_position(double x, double y, double z);
  void set_listener_orientation(double forward_x, double forward_y, double forward_z);

  // Mix all active sounds into the interleaved int16 output buffer.
  void mix_audio(int16_t* output_buffer, size_t num_frames);
  // Advance playback timers and process state changes.
  void update(double delta_time);

  // Number of currently active (playing) sound instances.
  size_t active_sound_count() const;

  // Optional callback for writing mixed audio to an external buffer.
  using AudioCallback = std::function<void(int16_t*, size_t)>;
  void set_output_callback(AudioCallback cb) { output_callback_ = cb; }

  private:
   // --- Mixing ---
   void mix_sound(SoundInstance& instance, int16_t* output, size_t frames, uint16_t channels);
   void synthesize_engine(int16_t* output, size_t frames, uint16_t channels);
   void apply_spatial_attenuation(SoundInstance& instance);

  // --- Output config ---
  uint32_t sample_rate_ = 44100;
  uint16_t channels_ = 2;
  double master_volume_ = 1.0;
  bool initialized_ = false;

  // --- Sound instances ---
  std::vector<SoundInstance> sounds_;
  int next_id_ = 1;

  // --- Engine synthesis ---
  EngineSoundParams engine_params_;
  double engine_phase_ = 0.0;  // rad, phase accumulator for oscillator
  bool engine_playing_ = false;

  // --- Listener ---
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
