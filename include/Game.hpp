#ifndef GAME_HPP
#define GAME_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <pthread.h>
#include <string>
#include <unordered_map>
#include "SharedState.hpp"
#include "AudioManager.hpp"

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
    void startPlayerSelection();
    void resetGame();
    void nextLevel();
    void loadLevel(int level);
    void cleanEntities();

    void loadSettings();
    void saveSettings();
    void applyVolume();
    void adjustVolume(int delta);
    void updateMusic();

    static void* kirbyLogic(void* arg);
    static void* enemyLogic(void* arg);

    bool running;
    sf::RenderWindow window;
    float masterVolume;
    SharedState sharedState;
    // Slider drag state for UI interaction (menu / pause)
    bool draggingMainSlider = false;
    bool draggingPauseSlider = false;
    // Master mute flag (persisted to settings.cfg)
    bool isMuted = false;
    int bestScore = 0;
    std::string currentPlayerName;
    std::string typedPlayerName;
    std::unordered_map<std::string, int> bestScoresByName;
    int highScoresScrollOffset = 0;
    int highScoresSelectedIndex = 0;
    bool ignoreNextTextInput = false;
    
    pthread_t kirbyThread;
    pthread_t enemyThread;

    sf::Font font;
    sf::Texture bgTex, bgInstructionsTex, highScoresBgTex, groundTex, platformTex, doorLeftTex, doorRightTex, doorStarTex;
    sf::Texture levelBgTex;
    sf::Sprite bgSprite, bgInstructionsSprite, highScoresBgSprite, groundSprite, platformSprite, doorLeftSprite, doorRightSprite, doorStarSprite;
    sf::Sprite levelBgSprite;

    AudioManager audioManager;
    int lastMusicLevel;
    GameMode pausedModeBeforePause;
    GameMode lastGameplayMode = GameMode::MENU;
};

#endif
