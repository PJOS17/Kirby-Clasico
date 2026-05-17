// ============================================================================
// ThreadManager.cpp - Hilos POSIX (pthread) para concurrencia
//
// ARQUITECTURA DE HILOS:
// - Hilo principal: Renderizado SFML + Input (OBLIGATORIO en hilo principal)
// - Hilo Kirby: Física y lógica del personaje
// - Hilo por Enemigo: Patrullaje independiente
// - Hilo por Proyectil: Movimiento y colisiones
//
// Todos los hilos usan pthread_mutex_lock/unlock para acceder al SharedState.
// ============================================================================
#include "ThreadManager.hpp"
#include "SharedState.hpp"
#include "Kirby.hpp"
#include "Enemy.hpp"
#include "Projectile.hpp"
#include <unistd.h>

ThreadManager::ThreadManager(SharedState* state)
    : state(state), kirbyThread(0), kirbyThreadActive(false) {}

ThreadManager::~ThreadManager() { joinAll(); }

void ThreadManager::startKirbyThread(bool isCPU) {
    KirbyThreadArg* arg = new KirbyThreadArg{state, isCPU};
    pthread_create(&kirbyThread, NULL, kirbyRoutine, arg);
    kirbyThreadActive = true;
}

void ThreadManager::startEnemyThread(Enemy* enemy) {
    pthread_t t;
    EnemyThreadArg* arg = new EnemyThreadArg{state, enemy};
    pthread_create(&t, NULL, enemyRoutine, arg);
    enemyThreads.push_back(t);
}

void ThreadManager::startProjectileThread(Projectile* proj) {
    pthread_t t;
    ProjectileThreadArg* arg = new ProjectileThreadArg{state, proj};
    pthread_create(&t, NULL, projectileRoutine, arg);
    projectileThreads.push_back(t);
}

void ThreadManager::joinAll() {
    if (kirbyThreadActive) {
        pthread_join(kirbyThread, NULL);
        kirbyThreadActive = false;
    }
    for (size_t i = 0; i < enemyThreads.size(); i++)
        pthread_join(enemyThreads[i], NULL);
    enemyThreads.clear();
    for (size_t i = 0; i < projectileThreads.size(); i++)
        pthread_join(projectileThreads[i], NULL);
    projectileThreads.clear();
}

// ============================================================================
// kirbyRoutine - Punto de entrada del hilo de Kirby.
// void* arg se castea a KirbyThreadArg* (empaquetado para pthread_create).
// El mutex se adquiere con state->lock() antes de tocar datos compartidos
// y se libera con state->unlock() antes de dormir.
// ============================================================================
void* ThreadManager::kirbyRoutine(void* arg) {
    KirbyThreadArg* tArg = static_cast<KirbyThreadArg*>(arg);
    SharedState* state = tArg->state;
    bool isCPU = tArg->isCPU;
    delete tArg;

    while (true) {
        state->lock(); // *** SECCIÓN CRÍTICA: adquirir mutex ***

        if (!state->isRunning()) { state->unlock(); break; }
        GameMode m = state->getMode();
        if (m != GameMode::MODE_1_PLAYER && m != GameMode::MODE_2_CPU) {
            state->unlock(); break;
        }

        if (state->kirby) {
            if (isCPU) state->kirby->updateCPU(state);
            else       state->kirby->update(state);
        }

        state->unlock(); // *** FIN SECCIÓN CRÍTICA: liberar mutex ***
        usleep(GameConfig::THREAD_SLEEP_US);
    }
    return NULL;
}

// ============================================================================
// enemyRoutine - Hilo independiente por cada enemigo.
// void* arg -> EnemyThreadArg* con puntero al enemigo específico.
// ============================================================================
void* ThreadManager::enemyRoutine(void* arg) {
    EnemyThreadArg* tArg = static_cast<EnemyThreadArg*>(arg);
    SharedState* state = tArg->state;
    Enemy* enemy = tArg->enemy;
    delete tArg;

    while (true) {
        state->lock();
        if (!state->isRunning()) { state->unlock(); break; }
        GameMode m = state->getMode();
        if (m != GameMode::MODE_1_PLAYER && m != GameMode::MODE_2_CPU) {
            state->unlock(); break;
        }
        if (enemy->isDead()) { state->unlock(); break; }

        enemy->update(state);
        state->unlock();
        usleep(GameConfig::THREAD_SLEEP_US);
    }
    return NULL;
}

// ============================================================================
// projectileRoutine - Hilo independiente por cada proyectil.
// El proyectil se mueve hasta impactar o salir de pantalla, luego el hilo termina.
// ============================================================================
void* ThreadManager::projectileRoutine(void* arg) {
    ProjectileThreadArg* tArg = static_cast<ProjectileThreadArg*>(arg);
    SharedState* state = tArg->state;
    Projectile* proj = tArg->projectile;
    delete tArg;

    while (true) {
        state->lock();
        if (!state->isRunning()) { state->unlock(); break; }
        GameMode m = state->getMode();
        if (m != GameMode::MODE_1_PLAYER && m != GameMode::MODE_2_CPU) {
            state->unlock(); break;
        }
        if (proj->isInactive()) { state->unlock(); break; }

        proj->update(state);
        state->unlock();
        usleep(GameConfig::THREAD_SLEEP_US);
    }
    return NULL;
}
