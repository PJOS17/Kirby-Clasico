#include "Kirby.hpp"
#include "SharedState.hpp"
#include "Enemy.hpp"
#include "Projectile.hpp"
#include <cmath>
#include <iostream>

Kirby::Kirby(float sx, float sy)
    : x(sx),y(sy),vx(0),vy(0),direction(1),state(KirbyState::IDLE),
      ability(Ability::NONE),pendingAbility(Ability::NONE),
      lives(GameConfig::MAX_LIVES),health(GameConfig::MAX_HEALTH),
      floatsLeft(GameConfig::MAX_FLOATS),absorbTimer(0),hurtTimer(0),
      onGround(false),textureLoaded(false),animFrame(0),animTimer(0), attackCooldown(0)
{
    bool ok = true;
    std::string base = "Sprys Nuevos/Animaciones Kirby/sprite_";
    ok &= texIdle[0].loadFromFile(base + "002.png");
    ok &= texIdle[1].loadFromFile(base + "002.png");
    // Walking sprites left and right
    ok &= texWalkLeft[0].loadFromFile(base + "277.png");
    ok &= texWalkLeft[1].loadFromFile(base + "278.png");
    ok &= texWalkRight[0].loadFromFile(base + "279.png");
    ok &= texWalkRight[1].loadFromFile(base + "280.png");
    ok &= texJump.loadFromFile(base + "006.png");
    // Falling sprites
    ok &= texFall.loadFromFile(base + "006.png");
    // Float
    ok &= texFloat[0].loadFromFile(base + "051.png");
    ok &= texFloat[1].loadFromFile(base + "052.png");
    ok &= texFloat[2].loadFromFile(base + "053.png");
    ok &= texFloat[3].loadFromFile(base + "054.png");
    ok &= texAbsorb.loadFromFile(base + "050.png");
    ok &= texHurt.loadFromFile(base + "060.png");
    // Puffed cheeks (has enemy in mouth)
    ok &= texPuffed[0].loadFromFile(base + "015.png");
    ok &= texPuffed[1].loadFromFile(base + "016.png");
    ok &= texPuffed[2].loadFromFile(base + "017.png");
    ok &= texPuffed[3].loadFromFile(base + "018.png");
    ok &= texPuffed[4].loadFromFile(base + "019.png");
    // Spit animation
    ok &= texSpit[0].loadFromFile(base + "183.png");
    ok &= texSpit[1].loadFromFile(base + "184.png");
    ok &= texSpit[2].loadFromFile(base + "185.png");

    if (ok) {
        textureLoaded = true;
        sprite.setTexture(texIdle[0]);
        sprite.setScale(3.0f, 3.0f);
    } else std::cerr << "[WARN] Faltan sprites de Kirby en Sprys Nuevos/\n";
}
Kirby::~Kirby() {}

void Kirby::resetAnimation() {
    animFrame = 0;
    animTimer = 0;
    if (!textureLoaded) return;
    switch (state) {
        case KirbyState::IDLE: sprite.setTexture(texIdle[0]); break;
        case KirbyState::WALKING: sprite.setTexture(direction == -1 ? texWalkLeft[0] : texWalkRight[0]); break;
        case KirbyState::JUMPING: sprite.setTexture(texJump); break;
        case KirbyState::FLOATING: sprite.setTexture(texFloat[0]); break;
        case KirbyState::ABSORBING: sprite.setTexture(texAbsorb); break;
        case KirbyState::HAS_ENEMY: sprite.setTexture(texPuffed[0]); break;
        case KirbyState::SPITTING: sprite.setTexture(texSpit[0]); break;
        case KirbyState::USING_ABILITY: sprite.setTexture(texAbsorb); break;
        case KirbyState::HURT: sprite.setTexture(texHurt); break;
        default: sprite.setTexture(texIdle[0]); break;
    }
}

void Kirby::update(SharedState* ss) {
    if (state == KirbyState::DEAD) return;
    if (hurtTimer > 0) {
        hurtTimer -= 0.016f;
        if (hurtTimer <= 0 && state == KirbyState::HURT) {
            state = KirbyState::IDLE;
            resetAnimation();
        }
    }
    
    if (attackCooldown > 0) {
        attackCooldown -= 0.016f;
        if (attackCooldown <= 0 && state == KirbyState::SPITTING) {
            state = KirbyState::IDLE;
            resetAnimation();
        }
        if (attackCooldown <= 0 && state == KirbyState::USING_ABILITY) {
            state = KirbyState::IDLE;
            resetAnimation();
        }
    }

    x += vx; applyGravity(); y += vy;
    checkPlatformCollisions(ss);
    if (x < 0) { x=0; vx=0; }
    if (x > 2900) { x=2900; vx=0; }
    if (y > GameConfig::WINDOW_HEIGHT + 100) { takeDamage(ss); x=100; y=300; }

    // Absorción
    if (state == KirbyState::ABSORBING) {
        sf::FloatRect area = getAbsorbBounds();
        for (size_t i=0; i<ss->enemies.size(); i++) {
            Enemy* e = ss->enemies[i];
            if (e && !e->isDead() && area.intersects(e->getBounds())) {
                Ability absorbed = e->getDropAbility();
                pendingAbility = Ability::NONE;
                ability = absorbed;
                e->die(); ss->score += 100; ss->stopSoundAbsorb = true;
                state = KirbyState::HAS_ENEMY;
                resetAnimation();
                break;
            }
        }
        // Absorber bombas
        for (size_t i=0; i<ss->projectiles.size(); i++) {
            Projectile* p = ss->projectiles[i];
            if (p && !p->isInactive() && !p->isFromKirby() && p->getAbility() == Ability::BOMB && area.intersects(p->getBounds())) {
                ability = Ability::BOMB_RETURN;
                pendingAbility = Ability::NONE;
                p->deactivate();
                ss->stopSoundAbsorb = true;
                state = KirbyState::HAS_ENEMY;
                resetAnimation();
                break;
            }
        }
    }
    // Daño contacto
    if (hurtTimer<=0 && state!=KirbyState::ABSORBING && state!=KirbyState::HURT) {
        sf::FloatRect kb = getBounds();
        for (size_t i=0; i<ss->enemies.size(); i++) {
            Enemy* e = ss->enemies[i];
            if (e && !e->isDead() && kb.intersects(e->getBounds())) { takeDamage(ss); break; }
        }
    }
    // Proyectiles enemigos
    if (hurtTimer<=0) {
        sf::FloatRect kb = getBounds();
        for (size_t i=0; i<ss->projectiles.size(); i++) {
            Projectile* p = ss->projectiles[i];
            if (p && !p->isInactive() && !p->isFromKirby() && kb.intersects(p->getBounds()))
                { takeDamage(ss); p->deactivate(); break; }
        }
    }
    // Spikes
    sf::FloatRect kb = getBounds();
    for (size_t i=0; i<ss->platforms.size(); i++) {
        if (ss->platforms[i].type == TileType::SPIKE) {
            sf::FloatRect sr(ss->platforms[i].x, ss->platforms[i].y - 24, ss->platforms[i].w, 24);
            if (kb.intersects(sr)) { takeDamage(ss); break; }
        }
    }
    // Puerta
    for (size_t i=0; i<ss->platforms.size(); i++) {
        if (ss->platforms[i].type == TileType::DOOR) {
            sf::FloatRect dr(ss->platforms[i].x, ss->platforms[i].y, ss->platforms[i].w, ss->platforms[i].h);
            bool bossAlive = false;
            for(auto* e: ss->enemies) if(e->isBoss() && !e->isDead()) bossAlive=true;
            
            if (kb.intersects(dr)) {
                if (!bossAlive || ss->currentLevel >= GameConfig::NUM_LEVELS-1) {
                    ss->playSoundDoor = true;
                    if (ss->currentLevel < GameConfig::NUM_LEVELS-1) { ss->currentLevel++; ss->statusMessage="NEXT_LEVEL"; }
                    else ss->setMode(GameMode::VICTORY);
                    break;
                }
            }
        }
    }
    // Cámara
    ss->cameraX = x - GameConfig::WINDOW_WIDTH / 3.0f;
    if (ss->cameraX < 0) ss->cameraX = 0;

    // Animación
    animTimer += 0.016f;
    if (animTimer > 0.10f) {
        animTimer = 0; animFrame++;
        if (textureLoaded) {
            switch (state) {
                case KirbyState::IDLE: sprite.setTexture(texIdle[animFrame%2]); break;
                case KirbyState::WALKING: sprite.setTexture(direction == -1 ? texWalkLeft[animFrame%2] : texWalkRight[animFrame%2]); break;
                case KirbyState::JUMPING: sprite.setTexture(vy<0?texJump:texFall); break;
                case KirbyState::FLOATING: sprite.setTexture(texFloat[animFrame%4]); break;
                case KirbyState::ABSORBING: sprite.setTexture(texAbsorb); break;
                case KirbyState::HAS_ENEMY: sprite.setTexture(texPuffed[animFrame%5]); break;
                case KirbyState::SPITTING: sprite.setTexture(texSpit[animFrame%3]); break;
                case KirbyState::USING_ABILITY: sprite.setTexture(texAbsorb); break;
                case KirbyState::HURT: sprite.setTexture(texHurt); break;
                default: sprite.setTexture(texIdle[0]); break;
            }
        }
    }
}

void Kirby::updateCPU(SharedState* ss) {
    if (state == KirbyState::HURT || state == KirbyState::DEAD) return;

    bool safeAhead = false;
    bool spikeAhead = false;
    bool wallAhead = false;
    float checkX = x + (direction == 1 ? 60 : -30);
    float checkY = y + 48;
    
    for (auto& p : ss->platforms) {
        if (checkX >= p.x && checkX <= p.x + p.w) {
            if (checkY <= p.y + 10 && checkY >= p.y - 150) {
                if (p.type == TileType::SPIKE) spikeAhead = true;
                else if (p.type == TileType::GROUND || p.type == TileType::PLATFORM || p.type == TileType::DOOR) safeAhead = true;
            }
            if (checkY > p.y + 10 && checkY < p.y + p.h) {
                wallAhead = true;
            }
        }
    }

    if (wallAhead || (vx == 0 && !onGround && state != KirbyState::HURT)) {
        direction *= -1;
    } else if (spikeAhead || !safeAhead) {
        if (onGround) jump(ss);
        else if (floatsLeft > 0) floatUp(ss);
        else direction *= -1;
    }
    
    if (vx == 0 && onGround && state != KirbyState::HURT) direction *= -1;
    if (rand() % 400 == 0) direction *= -1;

    vx = direction * GameConfig::MOVE_SPEED;
    if (state == KirbyState::IDLE) state = KirbyState::WALKING;

    if (rand() % 60 == 0) {
        if (state == KirbyState::HAS_ENEMY) {
            if (rand() % 2 == 0) swallow(ss);
            else spitOrUseAbility(ss);
        }
        else if (ability != Ability::NONE) spitOrUseAbility(ss);
        else startAbsorb(ss);
    }
    
    update(ss);
}

void Kirby::render(sf::RenderWindow& win, float camX) {
    if (state==KirbyState::DEAD) return;
    if (hurtTimer>0 && ((int)(hurtTimer*10)%2==0)) return;
    float drawX = x - camX;
    if (textureLoaded) {
        float scale = 3.0f;
        float texH = sprite.getTexture()->getSize().y * scale;
        float drawY = y + 48 - texH;
        
        if (ability == Ability::FIRE) sprite.setColor(sf::Color(255, 150, 150));
        else if (ability == Ability::SWORD) sprite.setColor(sf::Color(150, 255, 150));
        else if (ability == Ability::SPARK) sprite.setColor(sf::Color(255, 255, 150));
        else if (ability == Ability::BEAM) sprite.setColor(sf::Color(150, 150, 255));
        else if (ability == Ability::BOMB_RETURN) sprite.setColor(sf::Color(230, 180, 120));
        else sprite.setColor(sf::Color(255, 255, 255));

        sprite.setPosition(drawX, drawY);
        if (direction==-1) { 
            if (state == KirbyState::WALKING) { sprite.setScale(scale, scale); sprite.setOrigin(0, 0); } // 277/278 face left natively
            else { sprite.setScale(-scale, scale); sprite.setOrigin((float)sprite.getTexture()->getSize().x, 0); }
        }
        else { 
            sprite.setScale(scale, scale); sprite.setOrigin(0,0); 
        }
        win.draw(sprite);

        if (state == KirbyState::HAS_ENEMY && ability != Ability::NONE) {
            sf::Color iconColor = sf::Color::White;
            if (ability == Ability::FIRE) iconColor = sf::Color(255, 120, 0);
            else if (ability == Ability::SWORD) iconColor = sf::Color(100, 255, 100);
            else if (ability == Ability::SPARK) iconColor = sf::Color(255, 255, 120);
            else if (ability == Ability::BEAM) iconColor = sf::Color(120, 200, 255);
            else if (ability == Ability::BOMB_RETURN) iconColor = sf::Color(200, 150, 100);

            sf::CircleShape icon(12);
            icon.setFillColor(iconColor);
            icon.setOutlineColor(sf::Color::White);
            icon.setOutlineThickness(2);
            icon.setPosition(drawX + 22, drawY - 18);
            win.draw(icon);

            if (ability == Ability::SWORD) {
                sf::RectangleShape blade(sf::Vector2f(2, 18));
                blade.setFillColor(sf::Color::White);
                blade.setPosition(drawX + 28, drawY - 12);
                blade.setRotation(-45);
                win.draw(blade);
            } else if (ability == Ability::FIRE) {
                sf::CircleShape flame(6);
                flame.setFillColor(sf::Color::Yellow);
                flame.setPosition(drawX + 26, drawY - 14);
                win.draw(flame);
            } else if (ability == Ability::SPARK) {
                sf::ConvexShape spark;
                spark.setPointCount(4);
                spark.setPoint(0, sf::Vector2f(drawX + 30, drawY - 16));
                spark.setPoint(1, sf::Vector2f(drawX + 26, drawY - 8));
                spark.setPoint(2, sf::Vector2f(drawX + 34, drawY - 4));
                spark.setPoint(3, sf::Vector2f(drawX + 32, drawY - 14));
                spark.setFillColor(sf::Color::White);
                win.draw(spark);
            } else if (ability == Ability::BEAM) {
                sf::RectangleShape beam(sf::Vector2f(14, 4));
                beam.setFillColor(sf::Color::White);
                beam.setPosition(drawX + 24, drawY - 12);
                win.draw(beam);
            } else if (ability == Ability::BOMB_RETURN) {
                sf::CircleShape dot(4);
                dot.setFillColor(sf::Color::Black);
                dot.setPosition(drawX + 28, drawY - 10);
                win.draw(dot);
            }
        }
    } else {
        sf::CircleShape c(24); c.setPosition(drawX,y);
        c.setFillColor(sf::Color(255,182,193)); c.setOutlineColor(sf::Color(220,120,150));
        c.setOutlineThickness(2); win.draw(c);
    }
    if (ability!=Ability::NONE) {
        sf::CircleShape h(8); h.setPosition(drawX+15,y-12);
        switch(ability){case Ability::FIRE:h.setFillColor(sf::Color(255,80,0));break;
        case Ability::SWORD:h.setFillColor(sf::Color(0,200,0));break;
        case Ability::SPARK:h.setFillColor(sf::Color(255,255,0));break;
        case Ability::BEAM:h.setFillColor(sf::Color(100,100,255));break;default:break;}
        win.draw(h);
    }
}

void Kirby::moveLeft()  { vx=-GameConfig::MOVE_SPEED; direction=-1; if(state==KirbyState::IDLE){state=KirbyState::WALKING; resetAnimation();} }
void Kirby::moveRight() { vx=GameConfig::MOVE_SPEED;  direction=1;  if(state==KirbyState::IDLE){state=KirbyState::WALKING; resetAnimation();} }
void Kirby::stopMoving(){ vx=0; if(state==KirbyState::WALKING){ state=KirbyState::IDLE; resetAnimation();} }
void Kirby::jump(SharedState* ss) {
    if(onGround){vy=GameConfig::JUMP_SPEED;state=KirbyState::JUMPING;onGround=false;ss->playSoundJump=true; resetAnimation();}
    else floatUp(ss);
}
void Kirby::floatUp(SharedState* ss) { if(floatsLeft>0){state=KirbyState::FLOATING;vy=GameConfig::FLOAT_LIFT;floatsLeft--;ss->playSoundJump=true; resetAnimation();} }
void Kirby::startAbsorb(SharedState* ss) { if(ability==Ability::NONE&&state!=KirbyState::HAS_ENEMY&&state!=KirbyState::ABSORBING){state=KirbyState::ABSORBING;ss->playSoundAbsorb=true; resetAnimation();} }
void Kirby::stopAbsorb(SharedState* ss) { if(state==KirbyState::ABSORBING){state=KirbyState::IDLE;ss->stopSoundAbsorb=true; resetAnimation();} }

void Kirby::swallow(SharedState* /*ss*/) {
    if (state != KirbyState::HAS_ENEMY) return;
    if (pendingAbility != Ability::NONE) {
        ability = pendingAbility;
    }
    pendingAbility = Ability::NONE;
    state = KirbyState::IDLE;
    resetAnimation();
}

void Kirby::spitOrUseAbility(SharedState* ss) {
    if (attackCooldown > 0) return;
    if (state == KirbyState::HAS_ENEMY && ability != Ability::NONE) {
        state = KirbyState::USING_ABILITY;
        resetAnimation();
        ss->playSoundHit = true;
        attackCooldown = 0.4f;
        if (ability == Ability::BOMB_RETURN) {
            ss->projectiles.push_back(new Projectile(x, y+20, direction, true, Ability::BOMB_RETURN));
            ability = Ability::NONE;
        } else {
            ss->projectiles.push_back(new Projectile(x, y+20, direction, true, ability));
        }
    } else if (state == KirbyState::HAS_ENEMY) {
        // Spit star without consuming an ability
        state = KirbyState::SPITTING;
        resetAnimation();
        ss->playSoundHit = true;
        attackCooldown = 0.4f;
        pendingAbility = Ability::NONE;
        ss->projectiles.push_back(new Projectile(x, y+20, direction, true, Ability::NONE));
    } else if (ability != Ability::NONE) {
        state = KirbyState::USING_ABILITY;
        resetAnimation();
        ss->playSoundHit = true;
        attackCooldown = 0.4f;
        if (ability == Ability::BOMB_RETURN) {
            ss->projectiles.push_back(new Projectile(x, y+20, direction, true, Ability::BOMB_RETURN));
            ability = Ability::NONE;
        } else {
            ss->projectiles.push_back(new Projectile(x, y+20, direction, true, ability));
        }
    }
}
void Kirby::takeDamage(SharedState* ss) {
    if(hurtTimer>0) return; 
    if(state==KirbyState::ABSORBING) ss->stopSoundAbsorb=true;
    health--; hurtTimer=1.5f; state=KirbyState::HURT; ability=Ability::NONE; pendingAbility=Ability::NONE;
    resetAnimation();
    vy=-5;vx=-direction*3;ss->playSoundDamage=true;
    if(health<=0){lives--;if(lives<=0){state=KirbyState::DEAD;ss->playSoundDeath=true;}
    else{health=GameConfig::MAX_HEALTH;floatsLeft=GameConfig::MAX_FLOATS;x=100;y=300;}}
}
void Kirby::resetState() { if(state==KirbyState::ABSORBING||state==KirbyState::USING_ABILITY){ state=KirbyState::IDLE; resetAnimation(); } }
sf::FloatRect Kirby::getBounds() const {
    return sf::FloatRect(x, y, 48, 48);
}
sf::FloatRect Kirby::getAbsorbBounds() const {
    if(direction==1)return sf::FloatRect(x+60,y,70,50);
    return sf::FloatRect(x-70,y,70,50);
}
void Kirby::applyGravity() {
    if(state==KirbyState::FLOATING){vy+=GameConfig::FLOAT_GRAVITY;if(vy>1)vy=1;}
    else if(state==KirbyState::HAS_ENEMY){vy+=GameConfig::GRAVITY*1.3f;if(vy>14)vy=14;} // Heavier with enemy
    else{vy+=GameConfig::GRAVITY;if(vy>12)vy=12;}
}
void Kirby::checkPlatformCollisions(SharedState* ss) {
    onGround=false; sf::FloatRect kr=getBounds();
    for(size_t i=0;i<ss->platforms.size();i++){
        const Platform& p=ss->platforms[i];
        if(p.type==TileType::SPIKE||p.type==TileType::DOOR)continue;
        sf::FloatRect pr(p.x,p.y,p.w,p.h);
        if(!kr.intersects(pr))continue;
        float oL=(kr.left+kr.width)-pr.left,oR=(pr.left+pr.width)-kr.left;
        float oT=(kr.top+kr.height)-pr.top,oB=(pr.top+pr.height)-kr.top;
        float mn=oL;int s=0;if(oR<mn){mn=oR;s=1;}if(oT<mn){mn=oT;s=2;}if(oB<mn){mn=oB;s=3;}
        switch(s){
            case 0:x=pr.left-kr.width;vx=0;break;
            case 1:x=pr.left+pr.width;vx=0;break;
            case 2:y=pr.top-kr.height;vy=0;onGround=true;
                floatsLeft=GameConfig::MAX_FLOATS;
                if(state==KirbyState::JUMPING||state==KirbyState::FLOATING){state=KirbyState::IDLE; resetAnimation();}break;
            case 3:y=pr.top+pr.height;vy=0;break;
        }
        kr=getBounds();
    }
}
