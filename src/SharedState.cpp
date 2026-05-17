#include "SharedState.hpp"
SharedState::SharedState()
    : kirby(nullptr), score(0), currentLevel(0), cameraX(0),
      playSoundJump(false), playSoundAbsorb(false), stopSoundAbsorb(false), playSoundHit(false),
      playSoundDamage(false), playSoundEnemyDie(false), playSoundDeath(false),
      playSoundDoor(false), running(true), currentMode(GameMode::MENU)
{
    pthread_mutex_init(&gameMutex, NULL);
    pthread_cond_init(&gameCond, NULL);
}
SharedState::~SharedState() { pthread_mutex_destroy(&gameMutex); pthread_cond_destroy(&gameCond); }
void SharedState::lock()   { pthread_mutex_lock(&gameMutex); }
void SharedState::unlock() { pthread_mutex_unlock(&gameMutex); }
bool SharedState::isRunning() const { return running; }
void SharedState::setRunning(bool r) { running = r; }
GameMode SharedState::getMode() const { return currentMode; }
void SharedState::setMode(GameMode m) { currentMode = m; }
