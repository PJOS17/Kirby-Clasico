#ifndef SHARED_STATE_HPP
#define SHARED_STATE_HPP
#include <pthread.h>
#include <vector>
#include <string>
#include "Constants.hpp"
class Kirby; class Enemy; class Projectile;

struct Platform { float x, y, w, h; TileType type; };

class SharedState {
public:
    SharedState();
    ~SharedState();
    void lock();
    void unlock();
    bool isRunning() const;
    void setRunning(bool r);
    GameMode getMode() const;
    void setMode(GameMode m);

    Kirby* kirby;
    std::vector<Enemy*> enemies;
    std::vector<Projectile*> projectiles;
    std::vector<Platform> platforms;
    int score, currentLevel;
    float cameraX; // scroll horizontal
    std::string statusMessage;
    bool playSoundJump, playSoundAbsorb, stopSoundAbsorb, playSoundHit, playSoundDamage;
    bool playSoundEnemyDie, playSoundDeath, playSoundDoor;
    pthread_mutex_t gameMutex;
    pthread_cond_t gameCond;
private:
    bool running;
    GameMode currentMode;
};
#endif
