#include "audio-system.hpp"
#include <iostream>

namespace our {

AudioSystem::AudioSystem() {
    // Constructor
}

AudioSystem::~AudioSystem() {
    destroy();
}

bool AudioSystem::initialize() {
    // Initialize SDL audio
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL audio initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer initialization failed: " << Mix_GetError() << std::endl;
        return false;
    }
    
    // Allocate mixing channels
    Mix_AllocateChannels(32);  // Adjust based on your needs
    
    // Reserve specific channels for voice
    Mix_ReserveChannels(4);    // Reserve first 4 channels for voice
    
    initialized = true;
    return true;
}

void AudioSystem::destroy() {
    if (!initialized) return;
    
    // Free all sound effects
    for (auto& [name, chunk] : soundEffects) {
        Mix_FreeChunk(chunk);
    }
    soundEffects.clear();
    
    // Free all voice clips
    for (auto& [name, chunk] : voices) {
        Mix_FreeChunk(chunk);
    }
    voices.clear();
    
    // Free all music
    for (auto& [name, music_ptr] : music) {
        Mix_FreeMusic(music_ptr);
    }
    music.clear();
    
    // Close SDL_mixer and SDL
    Mix_CloseAudio();
    SDL_Quit();
    
    initialized = false;
}

bool AudioSystem::loadSound(const std::string& name, const std::string& filePath, AudioType type) {
    if (!initialized) {
        std::cerr << "Audio system not initialized" << std::endl;
        return false;
    }
    
    switch (type) {
        case AudioType::SOUND_EFFECT: {
            Mix_Chunk* chunk = Mix_LoadWAV(filePath.c_str());
            if (!chunk) {
                std::cerr << "Failed to load sound effect: " << Mix_GetError() << std::endl;
                return false;
            }
            soundEffects[name] = chunk;
            return true;
        }
        
        case AudioType::VOICE: {
            Mix_Chunk* chunk = Mix_LoadWAV(filePath.c_str());
            if (!chunk) {
                std::cerr << "Failed to load voice clip: " << Mix_GetError() << std::endl;
                return false;
            }
            voices[name] = chunk;
            return true;
        }
        
        case AudioType::MUSIC: {
            Mix_Music* music_ptr = Mix_LoadMUS(filePath.c_str());
            if (!music_ptr) {
                std::cerr << "Failed to load music: " << Mix_GetError() << std::endl;
                return false;
            }
            music[name] = music_ptr;
            return true;
        }
        
        default:
            return false;
    }
}

void AudioSystem::playSound(const std::string& name, int loops) {
    if (!initialized) return;
    
    auto it = soundEffects.find(name);
    if (it != soundEffects.end()) {
        int channel = Mix_PlayChannel(-1, it->second, loops);
        if (channel == -1) {
            std::cerr << "Failed to play sound effect: " << Mix_GetError() << std::endl;
        } else {
            Mix_Volume(channel, sfxVolume);
        }
    }
}

void AudioSystem::playVoice(const std::string& name, int loops) {
    if (!initialized) return;
    
    auto it = voices.find(name);
    if (it != voices.end()) {
        // Use reserved channels for voice (0-3)
        int channel = Mix_PlayChannel(rand() % 4, it->second, loops);
        if (channel == -1) {
            std::cerr << "Failed to play voice clip: " << Mix_GetError() << std::endl;
        } else {
            Mix_Volume(channel, voiceVolume);
        }
    }
}

void AudioSystem::playMusic(const std::string& name, int loops) {
    if (!initialized) return;
    
    auto it = music.find(name);
    if (it != music.end()) {
        if (Mix_PlayMusic(it->second, loops) == -1) {
            std::cerr << "Failed to play music: " << Mix_GetError() << std::endl;
        } else {
            Mix_VolumeMusic(musicVolume);
        }
    }
}

void AudioSystem::stopSound(int channel) {
    if (!initialized) return;
    Mix_HaltChannel(channel);
}

void AudioSystem::stopVoice() {
    if (!initialized) return;
    // Stop all reserved voice channels (0-3)
    for (int i = 0; i < 4; i++) {
        Mix_HaltChannel(i);
    }
}

void AudioSystem::stopMusic() {
    if (!initialized) return;
    Mix_HaltMusic();
}

void AudioSystem::setSfxVolume(int volume) {
    sfxVolume = (volume < 0) ? 0 : (volume > MIX_MAX_VOLUME) ? MIX_MAX_VOLUME : volume;
    // Adjust volume for all non-reserved channels
    for (int i = 4; i < 32; i++) {
        if (Mix_Playing(i)) {
            Mix_Volume(i, sfxVolume);
        }
    }
}

void AudioSystem::setVoiceVolume(int volume) {
    voiceVolume = (volume < 0) ? 0 : (volume > MIX_MAX_VOLUME) ? MIX_MAX_VOLUME : volume;
    // Adjust volume for voice channels
    for (int i = 0; i < 4; i++) {
        if (Mix_Playing(i)) {
            Mix_Volume(i, voiceVolume);
        }
    }
}

void AudioSystem::setMusicVolume(int volume) {
    musicVolume = (volume < 0) ? 0 : (volume > MIX_MAX_VOLUME) ? MIX_MAX_VOLUME : volume;
    Mix_VolumeMusic(musicVolume);
}

bool AudioSystem::isPlaying(int channel) {
    return Mix_Playing(channel) != 0;
}

bool AudioSystem::isMusicPlaying() {
    return Mix_PlayingMusic() != 0;
}

}