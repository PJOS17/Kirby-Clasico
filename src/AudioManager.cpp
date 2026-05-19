#include "AudioManager.hpp"
#include "Constants.hpp"
#include <iostream>
#include <algorithm>

//Implementación de AudioManager
AudioManager::AudioManager()
    : masterVolume(50.0f), absorbPlaying(false), fadingFrom(nullptr), fadingTo(nullptr), isCrossfading(false), fadeElapsed(0.0f), fadeDuration(1.0f) {}

// Cargar todos los assets de audio (sonidos y música). Retorna true si todos se cargaron correctamente.
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

// Aplicar el volumen maestro a todos los sonidos y música. El volumen se clampa entre 0 y 100.
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

// Detener toda la música (usada al pausar o al volver al menú para evitar que varias pistas se superpongan).
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

// Implementar la lógica de crossfade en cada actualización de música. Esto se llama periódicamente desde `updateMusic()`.
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

// Detener toda la música.
void AudioManager::stopAllMusic() {
    musMenu.stop();
    musLevel.stop();
    musBossBattle.stop();
}

// Actualizar la música según el modo de juego actual y el nivel. Cambia a la pista apropiada para el menú, niveles o batalla de jefe, usando crossfade para transiciones suaves.
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

// Procesar eventos de sonido basados en las banderas dentro de SharedState. Esto se llama desde el hilo principal, por lo que es seguro llamar a las APIs de audio de SFML aquí.
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

//Procesar un snapshot de eventos de sonido (banderas) sin necesidad de mantener el mutex de SharedState, para evitar bloqueos y errores de audio al llamar a las APIs de SFML desde el hilo principal.
void AudioManager::processSoundEventsSnapshot(bool playJump, bool playAbsorb, bool stopAbsorb, bool playHit, bool playDamage, bool playEnemyDie, bool playDeath, bool playDoor) {
    // Guardar el estado del sonido de absorción para no reiniciarlo cada frame.
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
