#ifndef AUDIOMANAGER_HPP
#define AUDIOMANAGER_HPP

#include <SFML/Audio.hpp>
#include <SFML/System.hpp>
#include "SharedState.hpp"

class AudioManager {
public:
    AudioManager();
    bool loadAssets();
    void applyVolume(float volume);
    void stopAllMusic();
    void updateMusic(GameMode mode, int currentLevel, int& lastMusicLevel);
    void processSoundEvents(SharedState& state);
    // Process a snapshot of sound flags (safe to call without holding SharedState mutex).
    void processSoundEventsSnapshot(bool playJump, bool playAbsorb, bool stopAbsorb, bool playHit, bool playDamage, bool playEnemyDie, bool playDeath, bool playDoor);

private:
    float masterVolume;
    bool absorbPlaying;

    sf::SoundBuffer sbufJump;
    sf::SoundBuffer sbufAbsorb;
    sf::SoundBuffer sbufHit;
    sf::SoundBuffer sbufDamage;
    sf::SoundBuffer sbufEnemyDie;
    sf::SoundBuffer sbufDeath;
    sf::SoundBuffer sbufDoor;

    sf::Sound sndJump;
    sf::Sound sndAbsorb;
    sf::Sound sndHit;
    sf::Sound sndDamage;
    sf::Sound sndEnemyDie;
    sf::Sound sndDeath;
    sf::Sound sndDoor;

    sf::Music musMenu;
    sf::Music musLevel;
    sf::Music musBossBattle;
    // Crossfade support
    sf::Music* fadingFrom;
    sf::Music* fadingTo;
    bool isCrossfading;
    float fadeElapsed;
    float fadeDuration;
    sf::Clock fadeClock;
    void startCrossfade(sf::Music* target);
    void progressCrossfade();
    // Crossfade behavior: gradually lower the source music while raising the target
    // over `fadeDuration` seconds. `progressCrossfade()` must be called periodically
    // (e.g., from `updateMusic()`) to advance the transition.
};

#endif
