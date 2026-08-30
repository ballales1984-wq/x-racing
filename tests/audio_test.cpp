#include <gtest/gtest.h>
#include "audio/sound.h"
#include "audio/audio_engine.h"

using namespace p0::audio;

TEST(Sound, DefaultInvalid) {
  Sound sound;
  EXPECT_FALSE(sound.is_valid());
  EXPECT_EQ(sound.samples().size(), 0u);
}

TEST(Sound, CreateSilence) {
  Sound sound;
  sound.create_silence(1.0, 44100);
  EXPECT_TRUE(sound.is_valid());
  EXPECT_EQ(sound.samples().size(), 44100u);
  EXPECT_DOUBLE_EQ(sound.duration(), 1.0);
}

TEST(Sound, CreateFromSamples) {
  std::vector<int16_t> samples = {0, 16384, 32767, 16384, 0, -16384, -32768, -16384};
  Sound sound;
  sound.create_from_samples(samples, 8000);
  EXPECT_TRUE(sound.is_valid());
  EXPECT_EQ(sound.samples().size(), samples.size());
  EXPECT_EQ(sound.sample_rate(), 8000u);
}

TEST(Sound, SampleAt) {
  std::vector<int16_t> samples = {0, 1000, 2000, 3000};
  Sound sound;
  sound.create_from_samples(samples, 44100);
  EXPECT_EQ(sound.sample_at(0), 0);
  EXPECT_EQ(sound.sample_at(1), 1000);
  EXPECT_EQ(sound.sample_at(3), 3000);
  EXPECT_EQ(sound.sample_at(100), 0);
}

TEST(Sound, SampleAtLooping) {
  std::vector<int16_t> samples = {1000, 2000, 3000};
  Sound sound;
  sound.create_from_samples(samples, 44100);
  sound.set_looping(true);
  EXPECT_EQ(sound.sample_at(0), 1000);
  EXPECT_EQ(sound.sample_at(3), 1000);
  EXPECT_EQ(sound.sample_at(5), 3000);
}

TEST(AudioEngine, InitializeShutdown) {
  AudioEngine engine;
  EXPECT_TRUE(engine.initialize(44100, 2));
  engine.shutdown();
}

TEST(AudioEngine, PlaySound) {
  AudioEngine engine;
  engine.initialize(44100, 2);

  auto sound = std::make_shared<Sound>();
  sound->create_silence(0.1, 44100);

  int id = engine.play_sound(sound, SoundType::AMBIENT, 0.5, false);
  EXPECT_GE(id, 0);
  EXPECT_EQ(engine.active_sound_count(), 1u);

  engine.stop_sound(id);
}

TEST(AudioEngine, StopAll) {
  AudioEngine engine;
  engine.initialize(44100, 2);

  auto sound1 = std::make_shared<Sound>();
  sound1->create_silence(1.0, 44100);
  auto sound2 = std::make_shared<Sound>();
  sound2->create_silence(1.0, 44100);

  engine.play_sound(sound1, SoundType::AMBIENT);
  engine.play_sound(sound2, SoundType::UI);
  EXPECT_EQ(engine.active_sound_count(), 2u);

  engine.stop_all();
  EXPECT_EQ(engine.active_sound_count(), 0u);
}

TEST(AudioEngine, StopByType) {
  AudioEngine engine;
  engine.initialize(44100, 2);

  auto ambient = std::make_shared<Sound>();
  ambient->create_silence(1.0, 44100);
  auto ui = std::make_shared<Sound>();
  ui->create_silence(1.0, 44100);

  engine.play_sound(ambient, SoundType::AMBIENT);
  engine.play_sound(ui, SoundType::UI);
  EXPECT_EQ(engine.active_sound_count(), 2u);

  engine.stop_by_type(SoundType::AMBIENT);
}

TEST(AudioEngine, MasterVolume) {
  AudioEngine engine;
  engine.initialize(44100, 2);
  engine.set_master_volume(0.5);
  EXPECT_DOUBLE_EQ(engine.master_volume(), 0.5);
}

TEST(AudioEngine, EngineSoundParams) {
  AudioEngine engine;
  engine.initialize(44100, 2);

  EngineSoundParams params;
  params.rpm = 3000.0;
  params.max_rpm = 8000.0;
  params.load = 0.5;
  params.cylinders = 8;
  params.volume = 0.7;

  engine.set_engine_params(params);
  EXPECT_DOUBLE_EQ(engine.engine_params().rpm, 3000.0);
  EXPECT_DOUBLE_EQ(engine.engine_params().volume, 0.7);
}

TEST(AudioEngine, MixAudio) {
  AudioEngine engine;
  engine.initialize(44100, 2);

  auto sound = std::make_shared<Sound>();
  std::vector<int16_t> samples(4410);
  for (size_t i = 0; i < samples.size(); ++i) {
    samples[i] = static_cast<int16_t>(std::sin(i * 0.1) * 16000);
  }
  sound->create_from_samples(samples, 44100);

  engine.play_sound(sound, SoundType::AMBIENT, 0.5, false);

  std::vector<int16_t> buffer(44100 * 2, 0);
  engine.mix_audio(buffer.data(), 44100);

  bool has_nonzero = false;
  for (auto s : buffer) {
    if (s != 0) {
      has_nonzero = true;
      break;
    }
  }
  EXPECT_TRUE(has_nonzero);
}

TEST(AudioEngine, UpdateAdvancesPosition) {
  AudioEngine engine;
  engine.initialize(44100, 2);

  auto sound = std::make_shared<Sound>();
  sound->create_silence(0.5, 44100);

  engine.play_sound(sound, SoundType::AMBIENT, 1.0, false);
  EXPECT_EQ(engine.active_sound_count(), 1u);

  for (int i = 0; i < 10; ++i) {
    engine.update(0.1);
  }

  EXPECT_EQ(engine.active_sound_count(), 0u);
}

TEST(AudioEngine, InvalidSoundRejected) {
  AudioEngine engine;
  engine.initialize(44100, 2);

  auto sound = std::make_shared<Sound>();
  int id = engine.play_sound(sound, SoundType::AMBIENT);
  EXPECT_EQ(id, -1);
  EXPECT_EQ(engine.active_sound_count(), 0u);
}
