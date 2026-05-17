#ifndef GAME_HPP
#define GAME_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <pthread.h>
#include "SharedState.hpp"

class Game {
public:
    Game();
    ~Game();
    void run();

private:
    void processEvents();
    void update();
    void render();
    
    void startGame(GameMode mode);
    void resetGame();
    void nextLevel();
    void loadLevel(int level);
    void cleanEntities();

    static void* kirbyLogic(void* arg);
    static void* enemyLogic(void* arg);

    bool running;
    sf::RenderWindow window;
    SharedState sharedState;
    
    pthread_t kirbyThread;
    pthread_t enemyThread;

    sf::Font font;
    sf::Texture bgTex, bgInstructionsTex, groundTex, platformTex, doorLeftTex, doorRightTex, doorStarTex;
    sf::Texture levelBgTex;
    sf::Sprite bgSprite, bgInstructionsSprite, groundSprite, platformSprite, doorLeftSprite, doorRightSprite, doorStarSprite;
    sf::Sprite levelBgSprite;

    sf::SoundBuffer sbufJump, sbufAbsorb, sbufHit, sbufDamage, sbufEnemyDie, sbufDeath, sbufDoor, sbufBossBattle;
    sf::Sound sndJump, sndAbsorb, sndHit, sndDamage, sndEnemyDie, sndDeath, sndDoor, sndBossBattle;
    sf::Music musMenu, musLevel;
    int lastMusicLevel;
};

#endif
