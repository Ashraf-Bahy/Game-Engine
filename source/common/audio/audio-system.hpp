#pragma once

#include <SDL.h>
#include <SDL_mixer.h>
#include <unordered_map>
#include <string>
#include <memory>

namespace our {

enum class AudioType {
    SOUND_EFFECT,  // Short sounds (bullets, impacts)
    VOICE,         // Voice clips and narration
    MUSIC          // Background music
};

class AudioSystem {
private:
    std::unordered_map<std::string, Mix_Chunk*> soundEffects;
    std::unordered_map<std::string, Mix_Chunk*> voices;
    std::unordered_map<std::string, Mix_Music*> music;
    bool initialized = false;
    
    // Volume levels (0-128)
    int sfxVolume = MIX_MAX_VOLUME / 8;
    int voiceVolume = MIX_MAX_VOLUME * 4;
    int musicVolume = MIX_MAX_VOLUME / 16;

public:
    AudioSystem();
    ~AudioSystem();
    
    bool initialize();
    void destroy();
    
    // Load audio files
    bool loadSound(const std::string& name, const std::string& filePath, AudioType type);
    
    // Play functions
    void playSound(const std::string& name, int loops = 0);
    void playVoice(const std::string& name, int loops = 0);
    void playMusic(const std::string& name, int loops = -1);
    
    // Stop functions
    void stopSound(int channel = -1);
    void stopVoice();
    void stopMusic();
    
    // Volume control
    void setSfxVolume(int volume);
    void setVoiceVolume(int volume);
    void setMusicVolume(int volume);
    
    // Get current volume levels
    int getSfxVolume() const { return sfxVolume; }
    int getVoiceVolume() const { return voiceVolume; }
    int getMusicVolume() const { return musicVolume; }
    
    // Utility
    bool isPlaying(int channel);
    bool isMusicPlaying();
};

}