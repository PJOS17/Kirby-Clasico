#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP
#include <string>

namespace GameConfig {
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;
    const int NUM_LEVELS = 4;
    const float GRAVITY = 0.4f;
    const float FLOAT_GRAVITY = 0.2f;
    const float FLOAT_LIFT = -5.0f;
    const float JUMP_SPEED = -8.5f;
    const float MOVE_SPEED = 3.5f;
    const int MAX_LIVES = 4;
    const int MAX_HEALTH = 6;
    const int MAX_FLOATS = 6;
    const int THREAD_SLEEP_US = 16000;

    const std::string BG_PATH = "Sprys Nuevos/BackGround/sprite_002.png";
    const std::string KIRBY_FULL_PATH = "Sprys Nuevos/Animaciones Kirby/sprite_002.png";
    const std::string TILESET_GROUND = "Sprys Nuevos/Strages/sprite_339.png";
    const std::string TILESET_PLATFORM = "Sprys Nuevos/Strages/sprite_544.png";
    const std::string BG_INSTRUCTIONS = "Sprys Nuevos/BackGround/sprite_072.png";
    const std::string DOOR_LEFT = "Sprys Nuevos/Hub Words/sprite_187.png";
    const std::string DOOR_RIGHT = "Sprys Nuevos/Hub Words/sprite_188.png";
    const std::string DOOR_STAR = "Sprys Nuevos/Hub Words/sprite_248.png";

    const std::string SND_JUMP = "Sounds/Salto.mp3";
    const std::string SND_ABSORB = "Sounds/Sonido kirby absorver enemigos.mp3";
    const std::string SND_HIT = "Sounds/Sonido golpe de kirby.mp3";
    const std::string SND_DAMAGE = "Sounds/Sonido cuando te hacen da\xc3\xb1o.mp3"; // UTF-8 for daño
    const std::string SND_ENEMY_DIE = "Sounds/Sonido random al matar a un enemigo.mp3";
    const std::string SND_DEATH = "Sounds/Sonido de muerte kirby.mp3";
    const std::string SND_GAME_OVER = "Sounds/Game Over Sonido.mp3";
    const std::string SND_MENU = "Sounds/Sonido de fondo menu.mp3";
    const std::string SND_LEVEL = "Sounds/Sonido Niveles.mp3";
    const std::string SND_DOOR = "Sounds/Sonido entrar y salir de puertas.mp3";
    const std::string SND_BOSS_BATTLE = "Sounds/Sonnido batallas contra jefes.mp3";
}

enum class GameMode { MENU, INSTRUCTIONS, MODE_1_PLAYER, MODE_2_CPU, GAME_OVER, VICTORY, PAUSED };
enum class KirbyState { IDLE, WALKING, JUMPING, FLOATING, ABSORBING, HAS_ENEMY, SPITTING, USING_ABILITY, HURT, DEAD };
enum class Ability { NONE, FIRE, SWORD, SPARK, BEAM, BOMB, BOMB_RETURN };
enum class TileType { GROUND, PLATFORM, SPIKE, DOOR };
enum class EnemyType { WADDLE_DEE, BRONTO_BURT, CAPPY, BOSS, DANCER_BOSS };

#endif
