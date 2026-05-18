#include "AudioManager.hpp"
#include "Constants.hpp"
#include <iostream>
#include <algorithm>

AudioManager::AudioManager()
    : masterVolume(50.0f), absorbPlaying(false), fadingFrom(nullptr), fadingTo(nullptr), isCrossfading(false), fadeElapsed(0.0f), fadeDuration(1.0f) {}

bool AudioManager::loadAssets() {
    bool ok = true;
    if (sbufJump.loadFromFile(GameConfig::SND_JUMP)) sndJump.setBuffer(sbufJump);
    else { ok = false; std::cerr << "[WARN] No se cargó sonido: " << GameConfig::SND_JUMP << "\n"; }
    if (sbufAbsorb.loadFromFile(GameConfig::SND_ABSORB)) sndAbsorb.setBuffer(sbufAbsorb);
    else { ok = false; std::cerr << "[WARN] No se cargó sonido: " << GameConfig::SND_ABSORB << "\n"; }
    if (sbufHit.loadFromFile(GameConfig::SND_HIT)) sndHit.setBuffer(sbufHit);
    else { ok = false; std::cerr << "[WARN] No se cargó sonido: " << GameConfig::SND_HIT << "\n"; }
    if (sbufDamage.loadFromFile(GameConfig::SND_DAMAGE)) sndDamage.setBuffer(sbufDamage);
    else { ok = false; std::cerr << "[WARN] No se cargó sonido: " << GameConfig::SND_DAMAGE << "\n"; }
    if (sbufEnemyDie.loadFromFile(GameConfig::SND_ENEMY_DIE)) sndEnemyDie.setBuffer(sbufEnemyDie);
    else { ok = false; std::cerr << "[WARN] No se cargó sonido: " << GameConfig::SND_ENEMY_DIE << "\n"; }
    if (sbufDeath.loadFromFile(GameConfig::SND_DEATH)) sndDeath.setBuffer(sbufDeath);
    else { ok = false; std::cerr << "[WARN] No se cargó sonido: " << GameConfig::SND_DEATH << "\n"; }
    if (sbufDoor.loadFromFile(GameConfig::SND_DOOR)) sndDoor.setBuffer(sbufDoor);
    else { ok = false; std::cerr << "[WARN] No se cargó sonido: " << GameConfig::SND_DOOR << "\n"; }

    if (musMenu.openFromFile(GameConfig::SND_MENU)) {
        musMenu.setLoop(true);
    } else {
        ok = false;
        std::cerr << "[WARN] No se cargó música: " << GameConfig::SND_MENU << "\n";
    }
    if (musLevel.openFromFile(GameConfig::SND_LEVEL)) {
        musLevel.setLoop(true);
    } else {
        ok = false;
        std::cerr << "[WARN] No se cargó música: " << GameConfig::SND_LEVEL << "\n";
    }
    if (musBossBattle.openFromFile(GameConfig::SND_BOSS_BATTLE)) {
        musBossBattle.setLoop(true);
    } else {
        ok = false;
        std::cerr << "[WARN] No se cargó música: " << GameConfig::SND_BOSS_BATTLE << "\n";
    }
    return ok;
}

void AudioManager::applyVolume(float volume) {
    masterVolume = std::clamp(volume, 0.0f, 100.0f);
    sndJump.setVolume(masterVolume);
    sndAbsorb.setVolume(masterVolume);
    sndHit.setVolume(masterVolume);
    sndDamage.setVolume(masterVolume);
    sndEnemyDie.setVolume(masterVolume);
    sndDeath.setVolume(masterVolume);
    sndDoor.setVolume(masterVolume);
    musMenu.setVolume(masterVolume);
    musLevel.setVolume(masterVolume);
    musBossBattle.setVolume(masterVolume);
}

// Internal helper to start crossfade to a target music
void AudioManager::startCrossfade(sf::Music* target) {
    if (!target) return;
    if (fadingTo == target) return;
    // find current playing music
    sf::Music* current = nullptr;
    if (musMenu.getStatus() == sf::SoundSource::Playing) current = &musMenu;
    if (musLevel.getStatus() == sf::SoundSource::Playing) current = &musLevel;
    if (musBossBattle.getStatus() == sf::SoundSource::Playing) current = &musBossBattle;
    if (current == target) return;
    fadingFrom = current;
    fadingTo = target;
    // Begin crossfade process; progressCrossfade() will update volumes over time.
    isCrossfading = true;
    fadeElapsed = 0.0f;
    fadeClock.restart();
    // ensure target is playing at 0 volume
    fadingTo->setVolume(0.0f);
    if (fadingTo->getStatus() != sf::SoundSource::Playing) fadingTo->play();
}

// Progress an ongoing crossfade — called from updateMusic
void AudioManager::progressCrossfade() {
    if (!isCrossfading) return;
    // Compute delta time since last progress call and lerp volumes.
    float dt = fadeClock.restart().asSeconds();
    fadeElapsed += dt;
    float t = std::min(1.0f, fadeElapsed / fadeDuration);
    float vTo = masterVolume * t;
    float vFrom = masterVolume * (1.0f - t);
    if (fadingTo) fadingTo->setVolume(vTo);
    if (fadingFrom) fadingFrom->setVolume(vFrom);
    if (t >= 1.0f) {
        if (fadingFrom) fadingFrom->stop();
        if (fadingTo) fadingTo->setVolume(masterVolume);
        isCrossfading = false;
        fadingFrom = nullptr;
        fadingTo = nullptr;
    }
}

void AudioManager::stopAllMusic() {
    musMenu.stop();
    musLevel.stop();
    musBossBattle.stop();
}

void AudioManager::updateMusic(GameMode mode, int currentLevel, int& lastMusicLevel) {
    // Progress any active crossfade
    if (isCrossfading) progressCrossfade();

    if (mode == GameMode::PAUSED) {
        return; // keep current music when game is paused
    }

    if (mode == GameMode::MENU || mode == GameMode::INSTRUCTIONS || mode == GameMode::GAME_OVER || mode == GameMode::VICTORY) {
        if (musMenu.getStatus() != sf::SoundSource::Playing && (!isCrossfading || fadingTo != &musMenu)) {
            startCrossfade(&musMenu);
        }
        return;
    }

    if (currentLevel == 3) {
        if (lastMusicLevel != 3 || musBossBattle.getStatus() != sf::SoundSource::Playing) {
            startCrossfade(&musBossBattle);
            lastMusicLevel = 3;
        }
    } else {
        if (lastMusicLevel < 0 || lastMusicLevel == 3 || musLevel.getStatus() != sf::SoundSource::Playing) {
            startCrossfade(&musLevel);
            lastMusicLevel = currentLevel;
        }
    }
}

void AudioManager::processSoundEvents(SharedState& state) {
    if (absorbPlaying && sndAbsorb.getStatus() != sf::SoundSource::Playing) {
        absorbPlaying = false;
    }

    if (state.playSoundJump) {
        sndJump.play();
        state.playSoundJump = false;
    }
    if (state.playSoundAbsorb) {
        if (!absorbPlaying) {
            sndAbsorb.setLoop(true);
            sndAbsorb.play();
            absorbPlaying = true;
        }
        state.playSoundAbsorb = false;
    }
    if (state.stopSoundAbsorb) {
        if (absorbPlaying) {
            sndAbsorb.stop();
            absorbPlaying = false;
        }
        state.stopSoundAbsorb = false;
    }
    if (state.playSoundHit) {
        sndHit.play();
        state.playSoundHit = false;
    }
    if (state.playSoundDamage) {
        sndDamage.play();
        state.playSoundDamage = false;
    }
    if (state.playSoundDeath) {
        sndDeath.play();
        state.playSoundDeath = false;
    }
    if (state.playSoundDoor) {
        sndDoor.play();
        state.playSoundDoor = false;
    }
    if (state.playSoundEnemyDie) {
        sndEnemyDie.play();
        state.playSoundEnemyDie = false;
    }
}

void AudioManager::processSoundEventsSnapshot(bool playJump, bool playAbsorb, bool stopAbsorb, bool playHit, bool playDamage, bool playEnemyDie, bool playDeath, bool playDoor) {
    if (absorbPlaying && sndAbsorb.getStatus() != sf::SoundSource::Playing) {
        absorbPlaying = false;
    }

    if (playJump) {
        sndJump.play();
    }
    if (playAbsorb) {
        if (!absorbPlaying) {
            sndAbsorb.setLoop(true);
            sndAbsorb.play();
            absorbPlaying = true;
        }
    }
    if (stopAbsorb) {
        if (absorbPlaying) {
            sndAbsorb.stop();
            absorbPlaying = false;
        }
    }
    if (playHit) {
        sndHit.play();
    }
    if (playDamage) {
        sndDamage.play();
    }
    if (playDeath) {
        sndDeath.play();
    }
    if (playDoor) {
        sndDoor.play();
    }
    if (playEnemyDie) {
        sndEnemyDie.play();
    }
}
