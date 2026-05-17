// ============================================================================
// ThreadManager.hpp - Gestión de hilos POSIX
// ============================================================================
#ifndef THREAD_MANAGER_HPP
#define THREAD_MANAGER_HPP

#include <pthread.h>
#include <vector>

class SharedState;
class Enemy;
class Projectile;

struct KirbyThreadArg {
    SharedState* state;
    bool isCPU;
};

struct EnemyThreadArg {
    SharedState* state;
    Enemy* enemy;
};

struct ProjectileThreadArg {
    SharedState* state;
    Projectile* projectile;
};

class ThreadManager {
public:
    ThreadManager(SharedState* state);
    ~ThreadManager();

    void startKirbyThread(bool isCPU);
    void startEnemyThread(Enemy* enemy);
    void startProjectileThread(Projectile* projectile);
    void joinAll();

    static void* kirbyRoutine(void* arg);
    static void* enemyRoutine(void* arg);
    static void* projectileRoutine(void* arg);

private:
    SharedState* state;
    pthread_t kirbyThread;
    bool kirbyThreadActive;
    std::vector<pthread_t> enemyThreads;
    std::vector<pthread_t> projectileThreads;
};

#endif
