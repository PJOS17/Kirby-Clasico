#include "Enemy.hpp"
#include "SharedState.hpp"
#include "Kirby.hpp"
#include "Projectile.hpp"
#include <cmath>
#include <iostream>

Enemy::Enemy(float sx, float sy, float pd, Ability da, EnemyType t)
    : x(sx),y(sy),originX(sx),patrolDistance(pd),direction(-1),speed(1.5f),
      dead(false),dropAbility(da),enemyType(t),textureLoaded(false),animFrame(0),animTimer(0),
      stateTimer(0), dancerState(0), vy(0), bombCooldown(0), hitTimer(0), onPlatform(false)
{
    bossHealth = (t == EnemyType::BOSS || t == EnemyType::DANCER_BOSS) ? 5 : 1;
    if (t == EnemyType::BOSS) speed = 2.5f;

    std::string b = "Sprys Nuevos/Enemigos/sprite_";
    std::string s0, s1;

    if (t == EnemyType::DANCER_BOSS) {
        frameTex[0].loadFromFile("Sprys Nuevos/Bosses/sprite_007.png");
        frameTex[1].loadFromFile("Sprys Nuevos/Bosses/sprite_008.png");
        frameTex[2].loadFromFile("Sprys Nuevos/Bosses/sprite_009.png");
        frameTex[3].loadFromFile("Sprys Nuevos/Bosses/sprite_011.png");
        frameTex[4].loadFromFile("Sprys Nuevos/Bosses/sprite_012.png");
        frameTex[5].loadFromFile("Sprys Nuevos/Bosses/sprite_013.png");
        frameTex[6].loadFromFile("Sprys Nuevos/Bosses/sprite_014.png");
        frameTex[7].loadFromFile("Sprys Nuevos/Bosses/sprite_016.png");
        frameTex[8].loadFromFile("Sprys Nuevos/Bosses/sprite_017.png");
        frameTex[9].loadFromFile("Sprys Nuevos/Bosses/sprite_020.png");
        textureLoaded = true;
        sprite.setTexture(frameTex[0]);
        sprite.setScale(3.0f, 3.0f);
    } else {
        switch(t) {
            case EnemyType::WADDLE_DEE: s0=b+"005.png"; s1=b+"006.png"; break;
            case EnemyType::BRONTO_BURT: s0=b+"010.png"; s1=b+"011.png"; break;
            case EnemyType::CAPPY: s0=b+"015.png"; s1=b+"016.png"; break;
            case EnemyType::BOSS: s0="Sprys Nuevos/Boses y mini Bosses/sprite_026.png"; s1="Sprys Nuevos/Boses y mini Bosses/sprite_027.png"; break;
            default: s0=""; s1="";
        }
        if (frameTex[0].loadFromFile(s0) && frameTex[1].loadFromFile(s1)) {
            textureLoaded = true;
            sprite.setTexture(frameTex[0]);
            if (t == EnemyType::BOSS) {
                sprite.setScale(4.0f, 4.0f);
            } else {
                sprite.setScale(3.0f, 3.0f);
            }
        } else std::cerr << "[WARN] Sprite enemigo no encontrado: " << s0 << "\n";
    }
}
Enemy::~Enemy() {}

void Enemy::update(SharedState* ss) {
    if(dead)return;

    if (enemyType == EnemyType::DANCER_BOSS) {
        stateTimer += 0.016f;
        bombCooldown += 0.016f;
        if (hitTimer > 0) hitTimer -= 0.016f;

        // Gravedad
        y += vy;
        vy += 0.5f;

        // Colisión con suelo
        sf::FloatRect er = getBounds();
        bool onGround = false;
        for (size_t i = 0; i < ss->platforms.size(); i++) {
            if (ss->platforms[i].type == TileType::GROUND || ss->platforms[i].type == TileType::PLATFORM) {
                sf::FloatRect pr(ss->platforms[i].x, ss->platforms[i].y, ss->platforms[i].w, ss->platforms[i].h);
                if (vy >= 0 && x + er.width > pr.left && x < pr.left + pr.width && y + er.height >= pr.top && y + er.height - vy <= pr.top + 20) {
                    y = pr.top - er.height;
                    vy = 0;
                    onGround = true;
                    break;
                }
            }
        }
        // Safety: if boss fell below screen, reset
        if (y > GameConfig::WINDOW_HEIGHT) { y = GameConfig::WINDOW_HEIGHT - 200; vy = 0; }

        // Estado 0 = Idle/baile, 1 = Jump, 2 = Throw, 3 = Defeated
        if (dancerState == 3) {
            if (stateTimer > 2.0f) { dead = true; return; }
            animTimer += 0.016f;
            if (animTimer > 0.4f) {
                animTimer = 0;
                animFrame++;
                if (animFrame >= 2) animFrame = 0;
                sprite.setTexture(animFrame == 0 ? frameTex[8] : frameTex[9]);
            }
            return;
        }

        // Movimiento horizontal: perseguir a Kirby
        if (ss->kirby && dancerState != 2) {
            float kx = ss->kirby->getX();
            if (kx < x - 30) { x -= 1.8f; direction = -1; }
            else if (kx > x + 30) { x += 1.8f; direction = 1; }
        }
        // Limitar al área del arena
        if (x < 10) x = 10;
        if (x > 1500) x = 1500;

        // Saltar periódicamente
        if (onGround && dancerState == 0 && stateTimer > 1.5f) {
            dancerState = 1;
            vy = -20.0f;
            stateTimer = 0;
            animFrame = 0;
            animTimer = 0;
            sprite.setTexture(frameTex[5]);
        }
        // Aterrizar
        if (dancerState == 1 && onGround && vy == 0) {
            dancerState = 0;
            stateTimer = 0;
            animFrame = 0;
            animTimer = 0;
            sprite.setTexture(frameTex[0]);
        }

        // IA del boss
        if (dancerState == 0) {
            if (animTimer > 0.12f || animTimer == 0) {
                animTimer = 0;
                animFrame = (animFrame + 1) % 5;
                sprite.setTexture(frameTex[animFrame]);
            }
            animTimer += 0.016f;
            if (bombCooldown > 1.8f) {
                dancerState = 2;
                bombCooldown = 0;
                stateTimer = 0;
                animFrame = 0;
                animTimer = 0;
                sprite.setTexture(frameTex[7]);
            }
        } else if (dancerState == 1) {
            animTimer += 0.016f;
            if (animTimer > 0.1f) {
                animTimer = 0;
                animFrame = (animFrame + 1) % 3;
                sprite.setTexture(frameTex[5 + animFrame]);
            }
        } else if (dancerState == 2) {
            if (stateTimer > 0.15f) {
                stateTimer = 0;
                animFrame++;
                if (animFrame >= 2) {
                    int bombDir = (ss->kirby && ss->kirby->getX() < x) ? -1 : 1;
                    ss->projectiles.push_back(new Projectile(x, y - 30, bombDir, false, Ability::BOMB));
                    dancerState = 0;
                    animFrame = 0;
                    animTimer = 0;
                    sprite.setTexture(frameTex[0]);
                } else {
                    sprite.setTexture(frameTex[7 + animFrame]);
                }
            }
        }
        return;
    }

    x += speed*direction;
    if(std::abs(x-originX)>patrolDistance) direction*=-1;
    if(x<0){x=0;direction=1;}
    if (hitTimer > 0) hitTimer -= 0.016f;

    // Gravedad y colisión con plataformas
    onPlatform = false;
    y += 5;
    sf::FloatRect er = getBounds();
    for(size_t i=0; i<ss->platforms.size(); i++){
        if(ss->platforms[i].type == TileType::GROUND || ss->platforms[i].type == TileType::PLATFORM){
            sf::FloatRect pr(ss->platforms[i].x, ss->platforms[i].y, ss->platforms[i].w, ss->platforms[i].h);
            if(er.intersects(pr) && y + er.height - 10 <= pr.top + 15) {
                y = pr.top - er.height;
                onPlatform = true;
                break;
            }
        }
    }

    // Si está en plataforma, verificar si va a caer del borde
    if (onPlatform && enemyType != EnemyType::DANCER_BOSS) {
        float checkX = x + (direction == 1 ? er.width + 5 : -5);
        float checkY = y + er.height + 5;
        bool groundAhead = false;
        for(size_t i=0; i<ss->platforms.size(); i++){
            if(ss->platforms[i].type == TileType::GROUND || ss->platforms[i].type == TileType::PLATFORM){
                sf::FloatRect pr(ss->platforms[i].x, ss->platforms[i].y, ss->platforms[i].w, ss->platforms[i].h);
                if(checkX >= pr.left && checkX <= pr.left + pr.width && checkY >= pr.top && checkY <= pr.top + pr.height) {
                    groundAhead = true;
                    break;
                }
            }
        }
        if (!groundAhead) direction *= -1;
    }

    animTimer+=0.016f;
    if(animTimer>0.2f){animTimer=0;animFrame++;
        if(textureLoaded)sprite.setTexture(frameTex[animFrame%2]);}
}

void Enemy::render(sf::RenderWindow& win, float camX) {
    if(dead)return;
    float drawX = x - camX;
    if(textureLoaded){
        float sc;
        if (enemyType == EnemyType::DANCER_BOSS) sc = 3.0f;
        else sc = (enemyType == EnemyType::BOSS) ? 6.0f : 3.0f;
        
        float drawY = y;
        if (enemyType == EnemyType::DANCER_BOSS) {
            float texH = sprite.getTexture()->getSize().y * sc;
            drawY = y + 150.0f - texH;
        }
        
        sprite.setPosition(drawX, drawY);
        if(direction==1){sprite.setScale(-sc,sc);sprite.setOrigin((float)sprite.getTexture()->getSize().x,0);}
        else{sprite.setScale(sc,sc);sprite.setOrigin(0,0);}
        win.draw(sprite);
    } else {
        sf::RectangleShape fb(sf::Vector2f(enemyType==EnemyType::BOSS?64:32,enemyType==EnemyType::BOSS?64:32));
        fb.setPosition(drawX,y);fb.setFillColor(sf::Color(255,140,0));win.draw(fb);
    }
    if(dropAbility!=Ability::NONE && enemyType != EnemyType::DANCER_BOSS){
        sf::CircleShape d(5);d.setPosition(drawX+(enemyType==EnemyType::BOSS?32:16),y-10);
        switch(dropAbility){case Ability::FIRE:d.setFillColor(sf::Color(255,50,0));break;
        case Ability::SWORD:d.setFillColor(sf::Color(0,200,0));break;
        case Ability::SPARK:d.setFillColor(sf::Color(255,255,0));break;
        case Ability::BEAM:d.setFillColor(sf::Color(80,80,255));break;default:break;}
        win.draw(d);
    }
    // Boss health bar
    if (enemyType == EnemyType::BOSS || enemyType == EnemyType::DANCER_BOSS) {
        sf::RectangleShape hbg(sf::Vector2f(80, 8));
        hbg.setFillColor(sf::Color::Red);
        hbg.setPosition(drawX-10, y-15);
        float maxHealth = 5.0f;
        sf::RectangleShape hf(sf::Vector2f(80 * (bossHealth/maxHealth), 8));
        hf.setFillColor(sf::Color::Green);
        hf.setPosition(drawX-10, y-15);
        win.draw(hbg); win.draw(hf);
    }
}

sf::FloatRect Enemy::getBounds() const {
    float sc = (enemyType == EnemyType::BOSS) ? 6.0f : (enemyType == EnemyType::DANCER_BOSS ? 3.0f : 3.0f);
    float w=textureLoaded?sprite.getTexture()->getSize().x*sc:32;
    float h=textureLoaded?sprite.getTexture()->getSize().y*sc:32;
    if (enemyType == EnemyType::DANCER_BOSS) { 
        w = 140; 
        h = 150; 
    }
    return sf::FloatRect(x,y,w,h);
}

void Enemy::die() {
    if (hitTimer > 0) return;
    if (enemyType == EnemyType::DANCER_BOSS) {
        bossHealth--;
        hitTimer = 1.0f;
        if (bossHealth <= 0) {
            dancerState = 3;
            stateTimer = 0;
        }
    } else if (enemyType == EnemyType::BOSS) {
        bossHealth--;
        hitTimer = 1.0f;
        if (bossHealth <= 0) dead = true;
    } else {
        dead = true;
    }
}

void Enemy::takeDamage() {
    if (enemyType == EnemyType::DANCER_BOSS && bossHealth > 0 && hitTimer <= 0) {
        bossHealth--;
        hitTimer = 1.0f;
        if (bossHealth <= 0) {
            dancerState = 3;
            stateTimer = 0;
        }
    }
}
