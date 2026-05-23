#include "Game.hpp"
#include "Kirby.hpp"
#include "Enemy.hpp"
#include "Projectile.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>

Game::Game() : running(true), lastMusicLevel(-1), masterVolume(50.0f), pausedModeBeforePause(GameMode::MENU) {
    window.create(sf::VideoMode(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT), "Kirby Clásico - Motor Pthreads");
    window.setFramerateLimit(60);
    sharedState.setMode(GameMode::MENU);
    pthread_mutex_init(&sharedState.gameMutex, nullptr);

    if (!font.loadFromFile("arial.ttf")) std::cerr<<"[WARN] No se pudo cargar arial.ttf\n";
    loadSettings();
    if (bgTex.loadFromFile(GameConfig::BG_PATH)) { bgSprite.setTexture(bgTex); bgSprite.setScale(3.75f, 3.75f); }
    if (bgInstructionsTex.loadFromFile(GameConfig::BG_INSTRUCTIONS)) { bgInstructionsSprite.setTexture(bgInstructionsTex); bgInstructionsSprite.setScale(3.75f, 3.75f); }
    if (highScoresBgTex.loadFromFile("Sprys Nuevos/BackGround/sprite_121.png")) { highScoresBgSprite.setTexture(highScoresBgTex); highScoresBgSprite.setScale(3.75f, 3.75f); }
    if (groundTex.loadFromFile("Sprys Nuevos/Strages/sprite_001.png")) { groundSprite.setTexture(groundTex); groundSprite.setTextureRect(sf::IntRect(0, 87, 16, 16)); groundSprite.setScale(2.0f, 2.0f); }
    if (platformTex.loadFromFile(GameConfig::TILESET_PLATFORM)) { platformSprite.setTexture(platformTex); platformSprite.setScale(2.0f, 2.0f); }
    if (doorLeftTex.loadFromFile(GameConfig::DOOR_LEFT)) { doorLeftSprite.setTexture(doorLeftTex); doorLeftSprite.setScale(2.0f, 2.0f); }
    if (doorRightTex.loadFromFile(GameConfig::DOOR_RIGHT)) { doorRightSprite.setTexture(doorRightTex); doorRightSprite.setScale(2.0f, 2.0f); }
    if (doorStarTex.loadFromFile(GameConfig::DOOR_STAR)) { doorStarSprite.setTexture(doorStarTex); doorStarSprite.setScale(2.0f, 2.0f); }

    audioManager.loadAssets();
    applyVolume();
}

Game::~Game() {
    running = false;
    if(kirbyThread) pthread_join(kirbyThread, nullptr);
    if(enemyThread) pthread_join(enemyThread, nullptr);
    pthread_mutex_destroy(&sharedState.gameMutex);
    cleanEntities();
}

void Game::cleanEntities() {
    for(auto* e : sharedState.enemies) delete e;
    sharedState.enemies.clear();
    for(auto* p : sharedState.projectiles) delete p;
    sharedState.projectiles.clear();
    if(sharedState.kirby) { delete sharedState.kirby; sharedState.kirby = nullptr; }
    sharedState.platforms.clear();
}

void Game::loadSettings() {
    std::ifstream file("settings.cfg");
    if (file.is_open()) {
        // settings.cfg format: <volume> <mutedFlag> <bestScore> <bestKevin> <bestPablo> <bestFabiola>
        // If fields are absent, defaults are used for backwards compatibility.
        float savedVolume;
        int mutedFlag = 0;
        int savedBestScore = 0;
        int savedPlayerScore = 0;
        if (file >> savedVolume) {
            masterVolume = std::clamp(savedVolume, 0.0f, 100.0f);
            if (file >> mutedFlag) {
                isMuted = (mutedFlag != 0);
            } else {
                isMuted = false;
            }
            if (file >> savedBestScore) {
                bestScore = std::max(0, savedBestScore);
            }
            for (int i = 0; i < 3; i++) {
                if (file >> savedPlayerScore) {
                    bestScoreByPlayer[i] = std::max(0, savedPlayerScore);
                }
            }
        }
    }
}

void Game::saveSettings() {
    std::ofstream file("settings.cfg");
    if (file.is_open()) {
        // Persist volume, muted flag (0/1), global best score, and best player scores.
        file << masterVolume << " " << (isMuted ? 1 : 0) << " " << bestScore;
        for (int i = 0; i < 3; i++) {
            file << " " << bestScoreByPlayer[i];
        }
    }
}

void Game::applyVolume() {
    // Apply volume to audio manager; pass 0 when muted so all sounds are silenced.
    audioManager.applyVolume(isMuted ? 0.0f : masterVolume);
}

void Game::adjustVolume(int delta) {
    masterVolume = std::clamp(masterVolume + delta, 0.0f, 100.0f);
    applyVolume();
    saveSettings();
}

void Game::updateMusic() {
    audioManager.updateMusic(sharedState.getMode(), sharedState.currentLevel, lastMusicLevel);
}

void Game::run() {
    while (window.isOpen()) {
        processEvents();
        update();
        updateMusic();
        // Copiar banderas de sonido mientras se mantiene el mutex,
        // luego procesarlas fuera del mutex para evitar bloqueos y errores
        // de audio cuando SFML se ejecuta desde el hilo principal.
        bool pJump=false,pAbs=false,sAbs=false,pHit=false,pDmg=false,pEDie=false,pDeath=false,pDoor=false;
        pthread_mutex_lock(&sharedState.gameMutex);
        pJump = sharedState.playSoundJump;
        pAbs = sharedState.playSoundAbsorb;
        sAbs = sharedState.stopSoundAbsorb;
        pHit = sharedState.playSoundHit;
        pDmg = sharedState.playSoundDamage;
        pEDie = sharedState.playSoundEnemyDie;
        pDeath = sharedState.playSoundDeath;
        pDoor = sharedState.playSoundDoor;
        // Clear flags so other threads don't re-trigger them
        sharedState.playSoundJump = false;
        sharedState.playSoundAbsorb = false;
        sharedState.stopSoundAbsorb = false;
        sharedState.playSoundHit = false;
        sharedState.playSoundDamage = false;
        sharedState.playSoundEnemyDie = false;
        sharedState.playSoundDeath = false;
        sharedState.playSoundDoor = false;
        pthread_mutex_unlock(&sharedState.gameMutex);

        // Now safe to call audio APIs without holding the sharedState mutex
        audioManager.processSoundEventsSnapshot(pJump, pAbs, sAbs, pHit, pDmg, pEDie, pDeath, pDoor);
        render();
    }
}

void Game::processEvents() {
    sf::Event e;
    while (window.pollEvent(e)) {
        if (e.type == sf::Event::Closed) window.close();
        if (e.type == sf::Event::KeyPressed) {
            GameMode mode = sharedState.getMode();
            // Space toggles mute in non-paused modes (pause keeps its own M behavior)
            // Use Space so it's quick to reach while playing.
            if (e.key.code == sf::Keyboard::Space && mode != GameMode::PAUSED) {
                isMuted = !isMuted;
                applyVolume();
                saveSettings();
            }
            if (mode == GameMode::MENU) {
                if (e.key.code == sf::Keyboard::Num1) startPlayerSelection();
                if (e.key.code == sf::Keyboard::Num2) startGame(GameMode::MODE_2_CPU);
                if (e.key.code == sf::Keyboard::Num3) sharedState.setMode(GameMode::HIGH_SCORES);
                if (e.key.code == sf::Keyboard::I) sharedState.setMode(GameMode::INSTRUCTIONS);
                if (e.key.code == sf::Keyboard::Left) adjustVolume(-5);
                if (e.key.code == sf::Keyboard::Right) adjustVolume(5);
                if (e.key.code == sf::Keyboard::Escape) window.close();
            } else if (mode == GameMode::PLAYER_SELECT) {
                if (e.key.code == sf::Keyboard::Num1) {
                    currentPlayerIndex = 0;
                    currentPlayerName = "Kevin";
                    startGame(GameMode::MODE_1_PLAYER);
                }
                if (e.key.code == sf::Keyboard::Num2) {
                    currentPlayerIndex = 1;
                    currentPlayerName = "Pablo";
                    startGame(GameMode::MODE_1_PLAYER);
                }
                if (e.key.code == sf::Keyboard::Num3) {
                    currentPlayerIndex = 2;
                    currentPlayerName = "Fabiola";
                    startGame(GameMode::MODE_1_PLAYER);
                }
                if (e.key.code == sf::Keyboard::Escape) sharedState.setMode(GameMode::MENU);
            } else if (mode == GameMode::HIGH_SCORES) {
                if (e.key.code == sf::Keyboard::Escape) sharedState.setMode(GameMode::MENU);
            } else if (mode == GameMode::INSTRUCTIONS) {
                if (e.key.code == sf::Keyboard::Escape) sharedState.setMode(GameMode::MENU);
            } else if (mode == GameMode::PAUSED) {
                if (e.key.code == sf::Keyboard::Escape) {
                    sharedState.setMode(pausedModeBeforePause);
                }
                // In PAUSED mode, 'M' returns to main menu by resetting the game.
                if (e.key.code == sf::Keyboard::M) { resetGame(); sharedState.setMode(GameMode::MENU); }
                if (e.key.code == sf::Keyboard::Left) adjustVolume(-5);
                if (e.key.code == sf::Keyboard::Right) adjustVolume(5);
            } else if (mode == GameMode::GAME_OVER || mode == GameMode::VICTORY) {
                if (e.key.code == sf::Keyboard::Escape) resetGame();
                if (mode == GameMode::GAME_OVER && e.key.code == sf::Keyboard::R) {
                    GameMode retryMode = lastGameplayMode;
                    resetGame();
                    startGame(retryMode);
                }
            } else if ((mode == GameMode::MODE_1_PLAYER || mode == GameMode::MODE_2_CPU) && sharedState.kirby) {
                if (e.key.code == sf::Keyboard::Escape || e.key.code == sf::Keyboard::P) {
                    pausedModeBeforePause = mode;
                    sharedState.setMode(GameMode::PAUSED);
                }
                if (mode == GameMode::MODE_1_PLAYER) {
                    if (e.key.code == sf::Keyboard::Z) sharedState.kirby->jump(&sharedState);
                    if (e.key.code == sf::Keyboard::X) {
                        if (sharedState.kirby->getAbility() == Ability::NONE && sharedState.kirby->getState() != KirbyState::HAS_ENEMY)
                            sharedState.kirby->startAbsorb(&sharedState);
                        else sharedState.kirby->spitOrUseAbility(&sharedState);
                    }
                    if (e.key.code == sf::Keyboard::C || e.key.code == sf::Keyboard::Q) {
                        sharedState.kirby->setAbility(Ability::NONE);
                        sharedState.kirby->resetState();
                    }
                    if (e.key.code == sf::Keyboard::Down) {
                        sharedState.kirby->swallow(&sharedState);
                    }
                }
            }
        }
        // Mouse interactions for sliders
        if (e.type == sf::Event::MouseButtonPressed) {
            if (e.mouseButton.button == sf::Mouse::Left) {
                GameMode mode = sharedState.getMode();
                int mx = e.mouseButton.x;
                int my = e.mouseButton.y;
                // Área del slider del menú principal: clic para ajustar volumen.
                if (mode == GameMode::MENU) {
                    float sliderX = 240.0f;
                    float sliderY = 540.0f;
                    float sliderW = 320.0f;
                    float sliderH = 20.0f;
                    if (mx >= sliderX && mx <= sliderX + sliderW && my >= sliderY && my <= sliderY + sliderH) {
                        draggingMainSlider = true;
                        float rel = (mx - sliderX) / sliderW;
                        masterVolume = std::clamp(rel * 100.0f, 0.0f, 100.0f);
                        applyVolume(); saveSettings();
                    }
                }
                // Área del slider del menú de pausa: clic para ajustar volumen.
                if (mode == GameMode::PAUSED) {
                    float sliderX = 250.0f;
                    float sliderY = 390.0f;
                    float sliderW = 300.0f;
                    float sliderH = 18.0f;
                    if (mx >= sliderX && mx <= sliderX + sliderW && my >= sliderY && my <= sliderY + sliderH) {
                        draggingPauseSlider = true;
                        float rel = (mx - sliderX) / sliderW;
                        masterVolume = std::clamp(rel * 100.0f, 0.0f, 100.0f);
                        applyVolume(); saveSettings();
                    }
                }
            }
        }
        if (e.type == sf::Event::MouseButtonReleased) {
            if (e.mouseButton.button == sf::Mouse::Left) {
                draggingMainSlider = false;
                draggingPauseSlider = false;
            }
        }
        if (e.type == sf::Event::MouseMoved) {
            if (draggingMainSlider) {
                // Al arrastrar el ratón sobre el slider del menú, actualizar volumen.
                float sliderX = 240.0f;
                float sliderW = 320.0f;
                float mx = (float)e.mouseMove.x;
                float rel = (mx - sliderX) / sliderW;
                masterVolume = std::clamp(rel * 100.0f, 0.0f, 100.0f);
                applyVolume(); saveSettings();
            }
            if (draggingPauseSlider) {
                // Al arrastrar el ratón sobre el slider de pausa, actualizar volumen.
                float sliderX = 250.0f;
                float sliderW = 300.0f;
                float mx = (float)e.mouseMove.x;
                float rel = (mx - sliderX) / sliderW;
                masterVolume = std::clamp(rel * 100.0f, 0.0f, 100.0f);
                applyVolume(); saveSettings();
            }
        }
        if (e.type == sf::Event::KeyReleased && sharedState.getMode() == GameMode::MODE_1_PLAYER && sharedState.kirby) {
            if (e.key.code == sf::Keyboard::X) {
                sharedState.kirby->stopAbsorb(&sharedState);
            }
        }
    }

    // Input continuo
    if (sharedState.getMode() == GameMode::MODE_1_PLAYER && sharedState.kirby) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) sharedState.kirby->moveLeft();
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) sharedState.kirby->moveRight();
        else sharedState.kirby->stopMoving();
    }
}

void Game::resetGame() {
    pthread_mutex_lock(&sharedState.gameMutex);
    running = false;
    if(kirbyThread) pthread_join(kirbyThread, nullptr);
    if(enemyThread) pthread_join(enemyThread, nullptr);
    kirbyThread = 0; enemyThread = 0;
    sharedState.setMode(GameMode::MENU);
    sharedState.score = 0;
    sharedState.currentLevel = 0;
    currentPlayerIndex = -1;
    currentPlayerName.clear();
    cleanEntities();
    audioManager.stopAllMusic();
    lastMusicLevel = -1;
    audioManager.updateMusic(GameMode::MENU, sharedState.currentLevel, lastMusicLevel);
    pthread_mutex_unlock(&sharedState.gameMutex);
}

void Game::startPlayerSelection() {
    currentPlayerIndex = -1;
    currentPlayerName.clear();
    sharedState.setMode(GameMode::PLAYER_SELECT);
}

void Game::startGame(GameMode mode) {
    lastGameplayMode = mode;
    pthread_mutex_lock(&sharedState.gameMutex);
    sharedState.setMode(mode);
    sharedState.score = 0;
    sharedState.currentLevel = 0;
    sharedState.cameraX = 0;
    cleanEntities();
    sharedState.kirby = new Kirby(100, 300);
    audioManager.stopAllMusic();
    lastMusicLevel = -1;
    loadLevel(sharedState.currentLevel);
    running = true;
    pthread_create(&kirbyThread, nullptr, &Game::kirbyLogic, this);
    pthread_create(&enemyThread, nullptr, &Game::enemyLogic, this);
    pthread_mutex_unlock(&sharedState.gameMutex);
}

void Game::nextLevel() {
    sharedState.statusMessage = "";
    Ability oldAbility = Ability::NONE;
    int oldLives = 3, oldHealth = GameConfig::MAX_HEALTH;
    if (sharedState.kirby) { oldAbility = sharedState.kirby->getAbility(); oldLives = sharedState.kirby->getLives(); oldHealth = sharedState.kirby->getHealth(); }
    cleanEntities();
    sharedState.kirby = new Kirby(100, 300);
    sharedState.kirby->setAbility(oldAbility);
    sharedState.kirby->setLivesAndHealth(oldLives, oldHealth);
    sharedState.cameraX = 0;
    loadLevel(sharedState.currentLevel);
}

void Game::loadLevel(int level) {
    std::string bgFile = "Sprys Nuevos/BackGround/sprite_002.png";
    if (level == 1) bgFile = "Sprys Nuevos/BackGround/sprite_003.png";
    else if (level == 2) bgFile = "Sprys Nuevos/BackGround/sprite_006.png";
    else if (level == 3) bgFile = "Sprys Nuevos/BackGround/sprite_021.png";

    if (levelBgTex.loadFromFile(bgFile)) {
        levelBgSprite.setTexture(levelBgTex, true);
        float scaleX = GameConfig::WINDOW_WIDTH / (float)levelBgTex.getSize().x;
        float scaleY = GameConfig::WINDOW_HEIGHT / (float)levelBgTex.getSize().y;
        float scale = std::max(scaleX, scaleY);
        levelBgSprite.setScale(scale, scale);
    }

    float groundY = GameConfig::WINDOW_HEIGHT - 32;

    // Music management
    audioManager.updateMusic(sharedState.getMode(), level, lastMusicLevel);

    if (level == 3) {
        // Boss arena: wider, 50 tiles wide
        for (int i = 0; i < 50; i++) {
            sharedState.platforms.push_back({i*32.0f, groundY, 32.0f, 32.0f, TileType::GROUND});
        }
        // Platforms for Kirby to dodge
        sharedState.platforms.push_back({64.0f, groundY-100.0f, 96, 20, TileType::PLATFORM});
        sharedState.platforms.push_back({400.0f, groundY-120.0f, 96, 20, TileType::PLATFORM});
        sharedState.platforms.push_back({220.0f, groundY-180.0f, 96, 20, TileType::PLATFORM});
        sharedState.platforms.push_back({700.0f, groundY-100.0f, 96, 20, TileType::PLATFORM});
        sharedState.platforms.push_back({1000.0f, groundY-140.0f, 96, 20, TileType::PLATFORM});
        sharedState.platforms.push_back({1300.0f, groundY-90.0f, 96, 20, TileType::PLATFORM});
        // Boss in center
        sharedState.enemies.push_back(new Enemy(600, groundY - 150, 200, Ability::NONE, EnemyType::DANCER_BOSS));
    } else {
        int numTiles = 94;
        for (int i = 0; i < numTiles; i++) {
            bool isGap = false;
            bool isSpike = false;
            if (level == 0) {
                if (i >= 22 && i <= 24) isGap = true;
                if (i == 21) isSpike = true;
                if (i >= 61 && i <= 63) isGap = true;
                if (i == 60) isSpike = true;
            }
            if (level == 1) {
                if (i >= 16 && i <= 18) isGap = true;
                if (i == 15) isSpike = true;
                if (i >= 42 && i <= 45) isGap = true;
                if (i == 41) isSpike = true;
                if (i >= 71 && i <= 73) isGap = true;
                if (i == 70) isSpike = true;
            }
            if (level == 2) {
                if (i >= 26 && i <= 29) isGap = true;
                if (i == 25) isSpike = true;
                if (i >= 56 && i <= 59) isGap = true;
                if (i == 55) isSpike = true;
                if (i >= 76 && i <= 80) isGap = true;
                if (i == 75) isSpike = true;
            }
            if (isGap) continue;
            if (isSpike) {
                sharedState.platforms.push_back({i*32.0f, groundY, 32.0f, 32.0f, TileType::SPIKE});
            } else {
                sharedState.platforms.push_back({i*32.0f, groundY, 32.0f, 32.0f, TileType::GROUND});
            }
        }

        sharedState.platforms.push_back({2800.0f, groundY-64, 64.0f, 64.0f, TileType::DOOR});

        if (level == 0) {
            for (int i = 1; i < 10; i++) {
                sharedState.platforms.push_back({i*280.0f, groundY-100.0f - (i%3)*40.0f, 100, 20, TileType::PLATFORM});
                sharedState.enemies.push_back(new Enemy(i*280.0f+20, groundY-140.0f - (i%3)*40.0f, 35, (Ability)(i%4), EnemyType::CAPPY));
                if (i%3==0) sharedState.enemies.push_back(new Enemy(i*280.0f-60, groundY-62, 60, Ability::NONE, EnemyType::WADDLE_DEE));
                if (i%4==0) sharedState.enemies.push_back(new Enemy(i*280.0f, groundY-232, 50, Ability::FIRE, EnemyType::BRONTO_BURT));
            }
        } else if (level == 1) {
            for(int i = 1; i < 10; i++) {
                sharedState.platforms.push_back({i*260.0f, groundY-120.0f, 80, 20, TileType::PLATFORM});
                sharedState.enemies.push_back(new Enemy(i*260.0f+10, groundY-170.0f, 30, (Ability)(i%4), EnemyType::WADDLE_DEE));
                if (i%2==0) sharedState.enemies.push_back(new Enemy(i*260.0f-80, groundY-62, 60, Ability::SWORD, EnemyType::CAPPY));
                if (i%3==0) sharedState.enemies.push_back(new Enemy(i*260.0f, groundY-252, 60, Ability::SPARK, EnemyType::BRONTO_BURT));
            }
        } else if (level == 2) {
            for(int i = 1; i < 8; i++) {
                sharedState.platforms.push_back({i*300.0f, groundY-130.0f + (i%2)*50.0f, 80, 20, TileType::PLATFORM});
                sharedState.enemies.push_back(new Enemy(i*300.0f+10, groundY-190.0f + (i%2)*50.0f, 30, Ability::BEAM, EnemyType::BRONTO_BURT));
                if (i%2==0) sharedState.enemies.push_back(new Enemy(i*300.0f-60, groundY-62, 50, (Ability)(i%4), EnemyType::WADDLE_DEE));
            }
        }
    }
}

void* Game::kirbyLogic(void* arg) {
    Game* game = (Game*)arg;
    while(game->running) {
        GameMode m = game->sharedState.getMode();
        if (m == GameMode::MENU || m == GameMode::GAME_OVER || m == GameMode::VICTORY) break;
        if (m == GameMode::PAUSED) { sf::sleep(sf::milliseconds(50)); continue; }
        pthread_mutex_lock(&game->sharedState.gameMutex);
        if (game->sharedState.statusMessage == "NEXT_LEVEL") { pthread_mutex_unlock(&game->sharedState.gameMutex); sf::sleep(sf::milliseconds(16)); continue; }
        if (game->sharedState.kirby) {
            if (game->sharedState.getMode() == GameMode::MODE_1_PLAYER) game->sharedState.kirby->update(&game->sharedState);
            else if (game->sharedState.getMode() == GameMode::MODE_2_CPU) game->sharedState.kirby->updateCPU(&game->sharedState);
            if (game->sharedState.kirby->getState() == KirbyState::DEAD) game->sharedState.setMode(GameMode::GAME_OVER);
        }
        pthread_mutex_unlock(&game->sharedState.gameMutex);
        sf::sleep(sf::milliseconds(16));
    }
    return nullptr;
}

void* Game::enemyLogic(void* arg) {
    Game* game = (Game*)arg;
    while(game->running) {
        GameMode m = game->sharedState.getMode();
        if (m == GameMode::MENU || m == GameMode::GAME_OVER || m == GameMode::VICTORY) break;
        if (m == GameMode::PAUSED) { sf::sleep(sf::milliseconds(50)); continue; }
        pthread_mutex_lock(&game->sharedState.gameMutex);
        if (game->sharedState.statusMessage == "NEXT_LEVEL") { pthread_mutex_unlock(&game->sharedState.gameMutex); sf::sleep(sf::milliseconds(16)); continue; }
        
        for (auto* e : game->sharedState.enemies) if(e) e->update(&game->sharedState);
        for (auto* p : game->sharedState.projectiles) if(p) p->update(&game->sharedState);

        // Limpiar
        for(size_t i=0; i<game->sharedState.enemies.size(); ) {
            if(game->sharedState.enemies[i]->isDead()){
                if (game->sharedState.enemies[i]->isBoss()) {
                    game->sharedState.score += 1000;
                    game->sharedState.playSoundEnemyDie = true;
                    game->sharedState.setMode(GameMode::VICTORY);
                }
                delete game->sharedState.enemies[i];
                game->sharedState.enemies.erase(game->sharedState.enemies.begin()+i);
            } else i++;
        }
        for(size_t i=0; i<game->sharedState.projectiles.size(); ) {
            if(game->sharedState.projectiles[i]->isInactive()){
                delete game->sharedState.projectiles[i];
                game->sharedState.projectiles.erase(game->sharedState.projectiles.begin()+i);
            } else i++;
        }
        pthread_mutex_unlock(&game->sharedState.gameMutex);
        sf::sleep(sf::milliseconds(16));
    }
    return nullptr;
}

void Game::update() {
    pthread_mutex_lock(&sharedState.gameMutex);
    bool saveNeeded = false;
    if (sharedState.score > bestScore) {
        bestScore = sharedState.score;
        saveNeeded = true;
    }
    if (sharedState.getMode() == GameMode::MODE_1_PLAYER && currentPlayerIndex >= 0) {
        if (sharedState.score > bestScoreByPlayer[currentPlayerIndex]) {
            bestScoreByPlayer[currentPlayerIndex] = sharedState.score;
            saveNeeded = true;
        }
    }
    if(saveNeeded) saveSettings();
    if(sharedState.statusMessage == "NEXT_LEVEL") nextLevel();
    pthread_mutex_unlock(&sharedState.gameMutex);
}

void Game::render() {
    window.clear(sf::Color(135,206,235)); // Fallback sky color
    pthread_mutex_lock(&sharedState.gameMutex);
    GameMode mode = sharedState.getMode();
    float camX = sharedState.cameraX;

    if (mode == GameMode::MENU || mode == GameMode::INSTRUCTIONS) {
        sf::Sprite& menuBg = (mode == GameMode::INSTRUCTIONS) ? bgInstructionsSprite : bgSprite;
        menuBg.setPosition(0,0);
        window.draw(menuBg);
        sf::RectangleShape fade(sf::Vector2f(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT));
        fade.setFillColor(sf::Color(20,20,40,200)); window.draw(fade);

        sf::Text t; t.setFont(font); t.setFillColor(sf::Color::White);
        if (mode == GameMode::MENU) {
            sf::RectangleShape menuCard(sf::Vector2f(520, 380));
            menuCard.setPosition(140, 110);
            menuCard.setFillColor(sf::Color(15, 15, 35, 220));
            menuCard.setOutlineColor(sf::Color(255, 182, 193));
            menuCard.setOutlineThickness(4);
            window.draw(menuCard);

            t.setString("KIRBY CLASICO"); t.setCharacterSize(60);
            t.setStyle(sf::Text::Bold); t.setFillColor(sf::Color(255,182,193));
            t.setPosition(170, 120); window.draw(t);
            
            t.setCharacterSize(24); t.setFillColor(sf::Color(220,220,255));
            t.setStyle(sf::Text::Regular);
            t.setString("1 - Modo 1 Jugador\n\n2 - Modo CPU vs Enemigos\n\n3 - Puntajes mas altos\n\nI - Instrucciones\n\nESC - Salir");
            t.setPosition(210, 210); window.draw(t);

            float sliderWidth = 320.0f;
            float sliderHeight = 20.0f;
            float sliderX = 240.0f;
            float sliderY = 540.0f; // moved down to avoid overlapping menu text
            sf::RectangleShape sliderBg(sf::Vector2f(sliderWidth, sliderHeight));
            sliderBg.setFillColor(sf::Color(70, 70, 90, 220));
            sliderBg.setOutlineColor(sf::Color(255, 182, 193));
            sliderBg.setOutlineThickness(2);
            sliderBg.setPosition(sliderX, sliderY);
            window.draw(sliderBg);

            sf::RectangleShape sliderFill(sf::Vector2f(sliderWidth * (masterVolume / 100.0f), sliderHeight));
            sliderFill.setFillColor(sf::Color(255, 182, 193));
            sliderFill.setPosition(sliderX, sliderY);
            window.draw(sliderFill);

            sf::CircleShape knob(10);
            knob.setFillColor(sf::Color(255, 255, 255));
            knob.setPosition(sliderX + sliderWidth * (masterVolume / 100.0f) - 10, sliderY - 6);
            window.draw(knob);

            t.setCharacterSize(20);
            t.setFillColor(sf::Color(220,220,255));
            t.setString("Volumen: " + std::to_string((int)masterVolume) + "%");
            t.setPosition(sliderX, sliderY - 40.0f); // label above slider
            window.draw(t);
            t.setString("Mouse para ajustar");
            t.setPosition(sliderX, sliderY + 30.0f); // helper below slider
            window.draw(t);

            // Mensaje de "MUTED" si el sonido está silenciado, para mayor claridad al usar el botón de silencio o arrastrar el slider a 0.
            if (isMuted) {
                sf::Text mutedText; mutedText.setFont(font);
                mutedText.setString("MUTED"); mutedText.setCharacterSize(18);
                mutedText.setFillColor(sf::Color::Red); mutedText.setStyle(sf::Text::Bold);
                mutedText.setPosition(sliderX + sliderWidth + 12.0f, sliderY - 6.0f);
                window.draw(mutedText);
            }
        } else {
            t.setString("INSTRUCCIONES"); t.setCharacterSize(40); t.setPosition(250, 50); window.draw(t);
            t.setCharacterSize(20);
            t.setString("Flechas - Moverse\nZ - Saltar / Flotar\nX - Absorber / Escupir / Habilidad\nQ / C - Soltar Habilidad\n\n- Absorbe enemigos para obtener habilidades\n- Flota para evitar caer en huecos\n- Llega a la puerta dorada al final de nivel\n- Derrota al Jefe en el Nivel 4\n\nESC - Volver al menu");
            t.setPosition(100, 150); window.draw(t);
        }
    } else if (mode == GameMode::GAME_OVER || mode == GameMode::VICTORY) {
        sf::RectangleShape panel(sf::Vector2f(520, 320));
        panel.setPosition(140, 150);
        panel.setFillColor(sf::Color(18, 28, 72, 230));
        panel.setOutlineColor(sf::Color::White);
        panel.setOutlineThickness(4);
        window.draw(panel);

        sf::Text t; t.setFont(font);
        if (mode == GameMode::GAME_OVER) {
            t.setCharacterSize(56);
            t.setString("MORISTE");
            t.setFillColor(sf::Color::White);
            t.setStyle(sf::Text::Bold);
            t.setPosition(240, 180);
            window.draw(t);

            t.setCharacterSize(28);
            t.setStyle(sf::Text::Regular);
            t.setFillColor(sf::Color::White);
            t.setString("Te quedaste sin vidas");
            t.setPosition(210, 250);
            window.draw(t);


            sf::RectangleShape retryBtn(sf::Vector2f(220, 50));
            retryBtn.setFillColor(sf::Color::White);
            retryBtn.setPosition(180, 320);
            window.draw(retryBtn);
            sf::Text retryText; retryText.setFont(font);
            retryText.setString("R - Reintentar"); retryText.setCharacterSize(24);
            retryText.setFillColor(sf::Color::Black); retryText.setPosition(210, 333);
            window.draw(retryText);

            sf::RectangleShape menuBtn(sf::Vector2f(220, 50));
            menuBtn.setFillColor(sf::Color::White);
            menuBtn.setPosition(340, 320);
            window.draw(menuBtn);
            sf::Text menuText; menuText.setFont(font);
            menuText.setString("ESC - Menu"); menuText.setCharacterSize(24);
            menuText.setFillColor(sf::Color::Black); menuText.setPosition(380, 333);
            window.draw(menuText);
        } else {
            t.setCharacterSize(50);
            t.setString("VICTORY!");
            t.setFillColor(sf::Color::White);
            t.setStyle(sf::Text::Bold);
            t.setPosition(240, 190);
            window.draw(t);
            t.setCharacterSize(24);
            t.setStyle(sf::Text::Regular);
            t.setFillColor(sf::Color::White);
            t.setString("Presiona ESC para volver");
            t.setPosition(250, 300);
            window.draw(t);
        }
    } else if (mode == GameMode::PLAYER_SELECT) {
        bgSprite.setPosition(0,0);
        window.draw(bgSprite);
        sf::RectangleShape selectPanel(sf::Vector2f(520, 340));
        selectPanel.setPosition(140, 110);
        selectPanel.setFillColor(sf::Color(15, 15, 35, 220));
        selectPanel.setOutlineColor(sf::Color(255, 182, 193));
        selectPanel.setOutlineThickness(4);
        window.draw(selectPanel);

        sf::Text t; t.setFont(font);
        t.setString("SELECCIONA LA PERSONA"); t.setCharacterSize(35); t.setFillColor(sf::Color(255, 182, 193));
        t.setStyle(sf::Text::Bold);
        t.setPosition(160, 150); window.draw(t);

        t.setCharacterSize(24); t.setStyle(sf::Text::Regular); t.setFillColor(sf::Color(230,230,255));
        t.setString("1 - Kevin\n\n2 - Pablo\n\n3 - Fabiola\n\nESC - Volver al menu");
        t.setPosition(240, 220); window.draw(t);
        t.setCharacterSize(20);
    } else if (mode == GameMode::HIGH_SCORES) {
        highScoresBgSprite.setPosition(0,0);
        window.draw(highScoresBgSprite);
        sf::RectangleShape scoresPanel(sf::Vector2f(520, 360));
        scoresPanel.setPosition(140, 100);
        scoresPanel.setFillColor(sf::Color(12, 12, 24, 220));
        scoresPanel.setOutlineColor(sf::Color(255, 215, 0, 200));
        scoresPanel.setOutlineThickness(4);
        window.draw(scoresPanel);

        sf::Text t; t.setFont(font);
        t.setString("PUNTAJES MAS ALTOS"); t.setCharacterSize(40); t.setFillColor(sf::Color(255, 215, 0));
        t.setStyle(sf::Text::Bold);
        t.setPosition(180, 130); window.draw(t);

        t.setCharacterSize(24); t.setStyle(sf::Text::Regular); t.setFillColor(sf::Color(240,240,255));
        t.setString("Kevin: " + std::to_string(bestScoreByPlayer[0]) + "\n\nPablo: " + std::to_string(bestScoreByPlayer[1]) + "\n\nFabiola: " + std::to_string(bestScoreByPlayer[2]));
        t.setPosition(260, 220); window.draw(t);

        t.setCharacterSize(20);
        t.setString("ESC - Volver al menu");
        t.setPosition(300, 420); window.draw(t);
    } else if (mode == GameMode::PAUSED) {
        // Renderizar el juego de fondo
        levelBgSprite.setPosition(-((int)(camX * 0.2f) % (int)levelBgSprite.getGlobalBounds().width), 0);
        window.draw(levelBgSprite);
        levelBgSprite.setPosition(-((int)(camX * 0.2f) % (int)levelBgSprite.getGlobalBounds().width) + levelBgSprite.getGlobalBounds().width, 0);
        window.draw(levelBgSprite);
        for (const auto& p : sharedState.platforms) {
            float drawX = p.x - camX;
            if (drawX > GameConfig::WINDOW_WIDTH || drawX + p.w < 0) continue;
            if (p.type == TileType::GROUND || p.type == TileType::PLATFORM) {
                sf::Sprite& tileSpr = (p.type == TileType::PLATFORM) ? platformSprite : groundSprite;
                tileSpr.setPosition(drawX, p.y); window.draw(tileSpr);
            }
        }
        for (auto* e : sharedState.enemies) if(e) e->render(window, camX);
        if (sharedState.kirby) sharedState.kirby->render(window, camX);

        sf::RectangleShape pauseOverlay(sf::Vector2f(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT));
        pauseOverlay.setFillColor(sf::Color(0, 0, 0, 140));
        window.draw(pauseOverlay);

        sf::RectangleShape panel(sf::Vector2f(420, 320));
        panel.setPosition(190, 130);
        panel.setFillColor(sf::Color(25, 25, 45, 230));
        panel.setOutlineColor(sf::Color(255, 255, 255, 180));
        panel.setOutlineThickness(3);
        window.draw(panel);

        sf::Text t; t.setFont(font);
        t.setString("PAUSA"); t.setCharacterSize(48); t.setFillColor(sf::Color(255, 230, 210));
        t.setStyle(sf::Text::Bold);
        t.setPosition(320, 150); window.draw(t);

        t.setCharacterSize(22); t.setStyle(sf::Text::Regular); t.setFillColor(sf::Color(235,235,245));
        t.setString("ESC - Continuar\nM - Volver al menu principal\nAjustar volumen con mouse\n\nNivel: " + std::to_string(sharedState.currentLevel + 1) + "   Puntaje: " + std::to_string(sharedState.score));
        t.setPosition(230, 220); window.draw(t);

        float sliderX = 250.0f;
        float sliderY = 390.0f; // moved down to sit below pause text
        float sliderW = 300.0f;
        sf::RectangleShape sliderBg(sf::Vector2f(sliderW, 18));
        sliderBg.setPosition(sliderX, sliderY);
        sliderBg.setFillColor(sf::Color(70, 70, 90, 220));
        sliderBg.setOutlineColor(sf::Color(255, 182, 193));
        sliderBg.setOutlineThickness(2);
        window.draw(sliderBg);

        sf::RectangleShape sliderFill(sf::Vector2f(sliderW * (masterVolume / 100.0f), 18));
        sliderFill.setFillColor(sf::Color(255, 182, 193));
        sliderFill.setPosition(sliderX, sliderY);
        window.draw(sliderFill);

        sf::CircleShape knob(9);
        knob.setFillColor(sf::Color(255,255,255));
        knob.setPosition(sliderX + sliderW * (masterVolume / 100.0f) - 9, sliderY - 6);
        window.draw(knob);

        t.setCharacterSize(18);
        t.setFillColor(sf::Color(210,210,230));
        t.setString("Volumen: " + std::to_string((int)masterVolume) + "%");
        t.setPosition(sliderX, sliderY + 26); window.draw(t);
        // MUTED indicator for pause menu
        if (isMuted) {
            sf::Text mutedText; mutedText.setFont(font);
            mutedText.setString("MUTED"); mutedText.setCharacterSize(18);
            mutedText.setFillColor(sf::Color::Red); mutedText.setStyle(sf::Text::Bold);
            mutedText.setPosition(sliderX + sliderW + 12.0f, sliderY - 6.0f);
            window.draw(mutedText);
        }
    } else {
        // Parallax background
        levelBgSprite.setPosition(-((int)(camX * 0.2f) % (int)levelBgSprite.getGlobalBounds().width), 0);
        window.draw(levelBgSprite);
        levelBgSprite.setPosition(-((int)(camX * 0.2f) % (int)levelBgSprite.getGlobalBounds().width) + levelBgSprite.getGlobalBounds().width, 0);
        window.draw(levelBgSprite);

        // Tiles - primero suelo y plataformas
        for (const auto& p : sharedState.platforms) {
            float drawX = p.x - camX;
            if (drawX > GameConfig::WINDOW_WIDTH || drawX + p.w < 0) continue;
            
            if (p.type == TileType::DOOR) {
                sf::RectangleShape door(sf::Vector2f(p.w, p.h));
                door.setFillColor(sf::Color(20, 20, 20));
                door.setPosition(drawX, p.y);
                
                sf::CircleShape doorTop(p.w / 2.0f);
                doorTop.setFillColor(sf::Color(20, 20, 20));
                doorTop.setPosition(drawX, p.y - p.w / 2.0f);
                
                sf::CircleShape knob(4);
                knob.setFillColor(sf::Color(255, 215, 0));
                knob.setPosition(drawX + p.w - 16, p.y + p.h / 2.0f);

                window.draw(doorTop);
                window.draw(door);
                window.draw(knob);

                doorLeftSprite.setPosition(drawX - 32, p.y);
                window.draw(doorLeftSprite);
                doorRightSprite.setPosition(drawX + p.w, p.y);
                window.draw(doorRightSprite);
                doorStarSprite.setPosition(drawX + 16, p.y - 48);
                window.draw(doorStarSprite);
            } else if (p.type == TileType::GROUND || p.type == TileType::PLATFORM) {
                sf::Sprite& tileSpr = (p.type == TileType::PLATFORM) ? platformSprite : groundSprite;
                int reps = p.w / 32;
                for(int j=0; j<reps; j++) {
                    tileSpr.setPosition(drawX + j*32, p.y);
                    window.draw(tileSpr);
                }
            }
        }

        // Dibujar pinchos al final
        for (const auto& p : sharedState.platforms) {
            if (p.type != TileType::SPIKE) continue;
            float drawX = p.x - camX;
            if (drawX > GameConfig::WINDOW_WIDTH || drawX + p.w < 0) continue;
            
            sf::ConvexShape spike; spike.setPointCount(3);
            spike.setPoint(0, sf::Vector2f(drawX, p.y));
            spike.setPoint(1, sf::Vector2f(drawX+p.w/2, p.y - 24));
            spike.setPoint(2, sf::Vector2f(drawX+p.w, p.y));
            spike.setFillColor(sf::Color(200,50,50)); window.draw(spike);
        }
        for (auto* e : sharedState.enemies) if(e) e->render(window, camX);
        for (auto* pr : sharedState.projectiles) if(pr) pr->render(window, camX);
        if (sharedState.kirby) sharedState.kirby->render(window, camX);

        // HUD
        sf::Text ht; ht.setFont(font); ht.setCharacterSize(20); ht.setFillColor(sf::Color::White);
        if (sharedState.kirby) {
            std::string hudText = "HP: " + std::to_string(sharedState.kirby->getHealth()) + "/" + std::to_string(GameConfig::MAX_HEALTH) +
                         " | Vidas: " + std::to_string(sharedState.kirby->getLives()) +
                         " | Vuelos: " + std::to_string(sharedState.kirby->getFloatsLeft()) +
                         " | Nivel: " + std::to_string(sharedState.currentLevel+1) +
                         " | Score: " + std::to_string(sharedState.score);
            if (!currentPlayerName.empty()) {
                hudText = "Jugador: " + currentPlayerName + " | " + hudText;
            }
            ht.setString(hudText);
        }
        ht.setPosition(10, 10); window.draw(ht);
    }
    pthread_mutex_unlock(&sharedState.gameMutex);
    window.display();
}
