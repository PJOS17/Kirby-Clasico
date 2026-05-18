#include "Game.hpp"
#include "Kirby.hpp"
#include "Enemy.hpp"
#include "Projectile.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>

Game::Game() : running(true), lastMusicLevel(-1), masterVolume(50.0f) {
    window.create(sf::VideoMode(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT), "Kirby Clásico - Motor Pthreads");
    window.setFramerateLimit(60);
    sharedState.setMode(GameMode::MENU);
    pthread_mutex_init(&sharedState.gameMutex, nullptr);

    if (!font.loadFromFile("arial.ttf")) std::cerr<<"[WARN] No se pudo cargar arial.ttf\n";
    loadSettings();
    if (bgTex.loadFromFile(GameConfig::BG_PATH)) { bgSprite.setTexture(bgTex); bgSprite.setScale(3.75f, 3.75f); }
    if (bgInstructionsTex.loadFromFile(GameConfig::BG_INSTRUCTIONS)) { bgInstructionsSprite.setTexture(bgInstructionsTex); bgInstructionsSprite.setScale(3.75f, 3.75f); }
    if (groundTex.loadFromFile("Sprys Nuevos/Strages/sprite_001.png")) { groundSprite.setTexture(groundTex); groundSprite.setTextureRect(sf::IntRect(0, 87, 16, 16)); groundSprite.setScale(2.0f, 2.0f); }
    if (platformTex.loadFromFile(GameConfig::TILESET_PLATFORM)) { platformSprite.setTexture(platformTex); platformSprite.setScale(2.0f, 2.0f); }
    if (doorLeftTex.loadFromFile(GameConfig::DOOR_LEFT)) { doorLeftSprite.setTexture(doorLeftTex); doorLeftSprite.setScale(2.0f, 2.0f); }
    if (doorRightTex.loadFromFile(GameConfig::DOOR_RIGHT)) { doorRightSprite.setTexture(doorRightTex); doorRightSprite.setScale(2.0f, 2.0f); }
    if (doorStarTex.loadFromFile(GameConfig::DOOR_STAR)) { doorStarSprite.setTexture(doorStarTex); doorStarSprite.setScale(2.0f, 2.0f); }

    // Init sounds
    if(sbufJump.loadFromFile(GameConfig::SND_JUMP)) sndJump.setBuffer(sbufJump);
    if(sbufAbsorb.loadFromFile(GameConfig::SND_ABSORB)) sndAbsorb.setBuffer(sbufAbsorb);
    if(sbufHit.loadFromFile(GameConfig::SND_HIT)) sndHit.setBuffer(sbufHit);
    if(sbufDamage.loadFromFile(GameConfig::SND_DAMAGE)) sndDamage.setBuffer(sbufDamage);
    if(sbufEnemyDie.loadFromFile(GameConfig::SND_ENEMY_DIE)) sndEnemyDie.setBuffer(sbufEnemyDie);
    if(sbufDeath.loadFromFile(GameConfig::SND_DEATH)) sndDeath.setBuffer(sbufDeath);
    if(sbufDoor.loadFromFile(GameConfig::SND_DOOR)) sndDoor.setBuffer(sbufDoor);
    if(sbufBossBattle.loadFromFile(GameConfig::SND_BOSS_BATTLE)) {
        sndBossBattle.setBuffer(sbufBossBattle);
        sndBossBattle.setLoop(true);
    }

    if(musMenu.openFromFile(GameConfig::SND_MENU)){ musMenu.setLoop(true); musMenu.play(); }
    if(musLevel.openFromFile(GameConfig::SND_LEVEL)){ musLevel.setLoop(true); }
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
        float savedVolume;
        if (file >> savedVolume) {
            masterVolume = std::clamp(savedVolume, 0.0f, 100.0f);
        }
    }
}

void Game::saveSettings() {
    std::ofstream file("settings.cfg");
    if (file.is_open()) {
        file << masterVolume;
    }
}

void Game::applyVolume() {
    sndJump.setVolume(masterVolume);
    sndAbsorb.setVolume(masterVolume);
    sndHit.setVolume(masterVolume);
    sndDamage.setVolume(masterVolume);
    sndEnemyDie.setVolume(masterVolume);
    sndDeath.setVolume(masterVolume);
    sndDoor.setVolume(masterVolume);
    sndBossBattle.setVolume(masterVolume);
    musMenu.setVolume(masterVolume);
    musLevel.setVolume(masterVolume);
}

void Game::adjustVolume(int delta) {
    masterVolume = std::clamp(masterVolume + delta, 0.0f, 100.0f);
    applyVolume();
    saveSettings();
}

void Game::updateMusic() {
    GameMode mode = sharedState.getMode();
    if (mode == GameMode::MENU || mode == GameMode::INSTRUCTIONS || mode == GameMode::GAME_OVER || mode == GameMode::VICTORY) {
        if (musMenu.getStatus() != sf::SoundSource::Playing) {
            musLevel.stop();
            sndBossBattle.stop();
            musMenu.setLoop(true);
            musMenu.play();
        }
    }
}

void Game::run() {
    while (window.isOpen()) {
        processEvents();
        update();
        updateMusic();
        pthread_mutex_lock(&sharedState.gameMutex);
        if(sharedState.playSoundJump){sndJump.play();sharedState.playSoundJump=false;}
        if(sharedState.playSoundAbsorb){sndAbsorb.setLoop(true);sndAbsorb.play();sharedState.playSoundAbsorb=false;}
        if(sharedState.stopSoundAbsorb){sndAbsorb.setLoop(false);sndAbsorb.stop();sharedState.stopSoundAbsorb=false;}
        if(sharedState.playSoundHit){sndHit.play();sharedState.playSoundHit=false;}
        if(sharedState.playSoundDamage){sndDamage.play();sharedState.playSoundDamage=false;}
        if(sharedState.playSoundDeath){sndDeath.play();sharedState.playSoundDeath=false;}
        if(sharedState.playSoundDoor){sndDoor.play();sharedState.playSoundDoor=false;}
        if(sharedState.playSoundEnemyDie){sndEnemyDie.play();sharedState.playSoundEnemyDie=false;}
        pthread_mutex_unlock(&sharedState.gameMutex);
        render();
    }
}

void Game::processEvents() {
    sf::Event e;
    while (window.pollEvent(e)) {
        if (e.type == sf::Event::Closed) window.close();
        if (e.type == sf::Event::KeyPressed) {
            GameMode mode = sharedState.getMode();
            if (mode == GameMode::MENU) {
                if (e.key.code == sf::Keyboard::Num1) startGame(GameMode::MODE_1_PLAYER);
                if (e.key.code == sf::Keyboard::Num2) startGame(GameMode::MODE_2_CPU);
                if (e.key.code == sf::Keyboard::I) sharedState.setMode(GameMode::INSTRUCTIONS);
                if (e.key.code == sf::Keyboard::O) adjustVolume(-5);
                if (e.key.code == sf::Keyboard::P) adjustVolume(5);
                if (e.key.code == sf::Keyboard::Escape) window.close();
            } else if (mode == GameMode::INSTRUCTIONS) {
                if (e.key.code == sf::Keyboard::Escape) sharedState.setMode(GameMode::MENU);
            } else if (mode == GameMode::PAUSED) {
                if (e.key.code == sf::Keyboard::Escape) sharedState.setMode(GameMode::MODE_1_PLAYER);
                if (e.key.code == sf::Keyboard::M) { resetGame(); sharedState.setMode(GameMode::MENU); }
                if (e.key.code == sf::Keyboard::O) adjustVolume(-5);
                if (e.key.code == sf::Keyboard::P) adjustVolume(5);
            } else if (mode == GameMode::GAME_OVER || mode == GameMode::VICTORY) {
                if (e.key.code == sf::Keyboard::Escape) resetGame();
            } else if ((mode == GameMode::MODE_1_PLAYER || mode == GameMode::MODE_2_CPU) && sharedState.kirby) {
                if (e.key.code == sf::Keyboard::Escape) sharedState.setMode(GameMode::PAUSED);
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
    cleanEntities();
    musLevel.stop();
    sndBossBattle.stop();
    lastMusicLevel = -1;
    if(musMenu.getStatus() != sf::SoundSource::Playing) musMenu.play();
    pthread_mutex_unlock(&sharedState.gameMutex);
}

void Game::startGame(GameMode mode) {
    pthread_mutex_lock(&sharedState.gameMutex);
    sharedState.setMode(mode);
    sharedState.score = 0;
    sharedState.currentLevel = 0;
    sharedState.cameraX = 0;
    cleanEntities();
    sharedState.kirby = new Kirby(100, 300);
    musMenu.stop();
    musLevel.stop();
    sndBossBattle.stop();
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
    if (level == 3) {
        if (lastMusicLevel != 3 || sndBossBattle.getStatus() != sf::SoundSource::Playing) {
            musLevel.stop();
            sndBossBattle.setPlayingOffset(sf::seconds(0));
            sndBossBattle.play();
            lastMusicLevel = 3;
        }
    } else {
        if (lastMusicLevel < 0 || lastMusicLevel == 3 || musLevel.getStatus() != sf::SoundSource::Playing) {
            sndBossBattle.stop();
            musLevel.setPlayingOffset(sf::seconds(0));
            musLevel.play();
            lastMusicLevel = level;
        }
    }

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
            t.setString("KIRBY CLASICO"); t.setCharacterSize(60);
            t.setStyle(sf::Text::Bold); t.setFillColor(sf::Color(255,182,193));
            t.setPosition(150, 100); window.draw(t);
            
            t.setCharacterSize(24); t.setFillColor(sf::Color::White);
            t.setString("1 - Modo 1 Jugador\n\n2 - Modo CPU vs Enemigos\n\nI - Instrucciones\n\nESC - Salir");
            t.setPosition(250, 350); window.draw(t);

            float sliderWidth = 260.0f;
            float sliderHeight = 18.0f;
            float sliderX = (GameConfig::WINDOW_WIDTH - sliderWidth) / 2.0f;
            float sliderY = 280.0f;
            sf::RectangleShape sliderBg(sf::Vector2f(sliderWidth, sliderHeight));
            sliderBg.setFillColor(sf::Color(80, 80, 80, 220));
            sliderBg.setPosition(sliderX, sliderY);
            window.draw(sliderBg);

            sf::RectangleShape sliderFill(sf::Vector2f(sliderWidth * (masterVolume / 100.0f), sliderHeight));
            sliderFill.setFillColor(sf::Color(255, 182, 193));
            sliderFill.setPosition(sliderX, sliderY);
            window.draw(sliderFill);

            t.setCharacterSize(18);
            t.setFillColor(sf::Color::White);
            t.setString("Volumen: " + std::to_string((int)masterVolume) + "%   O / P");
            t.setPosition(sliderX, sliderY - 28.0f);
            window.draw(t);
            t.setString("Volumen guardado automaticamente.");
            t.setPosition(sliderX, sliderY + 24.0f);
            window.draw(t);
        } else {
            t.setString("INSTRUCCIONES"); t.setCharacterSize(40); t.setPosition(250, 50); window.draw(t);
            t.setCharacterSize(20);
            t.setString("Flechas - Moverse\nZ - Saltar / Flotar\nX - Absorber / Escupir / Habilidad\nQ / C - Soltar Habilidad\n\n- Absorbe enemigos para obtener habilidades\n- Flota para evitar caer en huecos\n- Llega a la puerta dorada al final de nivel\n- Derrota al Jefe en el Nivel 4\n\nESC - Volver al menu");
            t.setPosition(100, 150); window.draw(t);
        }
    } else if (mode == GameMode::GAME_OVER || mode == GameMode::VICTORY) {
        sf::Text t; t.setFont(font); t.setCharacterSize(50);
        t.setString(mode == GameMode::GAME_OVER ? "GAME OVER" : "VICTORY!");
        t.setFillColor(mode == GameMode::GAME_OVER ? sf::Color::Red : sf::Color::Yellow);
        t.setPosition(250, 200); window.draw(t);
        t.setCharacterSize(24); t.setFillColor(sf::Color::White);
        t.setString("Presiona ESC para volver"); t.setPosition(260, 300); window.draw(t);
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
        
        // Overlay de pausa
        sf::RectangleShape pauseOverlay(sf::Vector2f(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT));
        pauseOverlay.setFillColor(sf::Color(0, 0, 0, 150));
        window.draw(pauseOverlay);
        
        sf::Text t; t.setFont(font); t.setCharacterSize(50);
        t.setString("PAUSA"); t.setFillColor(sf::Color::White);
        t.setPosition(300, 150); window.draw(t);
        t.setCharacterSize(24);
        t.setString("ESC - Continuar\nM - Volver al menu principal\nO / P - Ajustar volumen");
        t.setPosition(220, 250); window.draw(t);
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
            ht.setString("HP: " + std::to_string(sharedState.kirby->getHealth()) + "/" + std::to_string(GameConfig::MAX_HEALTH) +
                         " | Vidas: " + std::to_string(sharedState.kirby->getLives()) +
                         " | Vuelos: " + std::to_string(sharedState.kirby->getFloatsLeft()) +
                         " | Nivel: " + std::to_string(sharedState.currentLevel+1) +
                         " | Score: " + std::to_string(sharedState.score));
        }
        ht.setPosition(10, 10); window.draw(ht);
    }
    pthread_mutex_unlock(&sharedState.gameMutex);
    window.display();
}
