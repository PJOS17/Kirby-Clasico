// Análisis detallado de sprites
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>

struct SpriteBounds {
    int x, y, w, h;
};

// Encontrar todos los sprites en una fila
void findSpritesInRow(sf::Image& img, sf::Color bg, int startY, int endY) {
    bool inSprite = false;
    int spriteStartX = 0;
    int count = 0;
    
    for (int x = 0; x < (int)img.getSize().x; x++) {
        bool colHasPixel = false;
        for (int y = startY; y <= endY; y++) {
            sf::Color c = img.getPixel(x, y);
            if (c != bg && c.a > 10) { colHasPixel = true; break; }
        }
        if (colHasPixel && !inSprite) {
            spriteStartX = x;
            inSprite = true;
        } else if (!colHasPixel && inSprite) {
            int w = x - spriteStartX;
            if (w > 3) { // Ignorar ruido
                std::cout << "    Frame " << count << ": x=" << spriteStartX << " w=" << w << "\n";
                count++;
            }
            inSprite = false;
        }
    }
    if (inSprite) {
        int w = img.getSize().x - spriteStartX;
        if (w > 3) std::cout << "    Frame " << count << ": x=" << spriteStartX << " w=" << w << "\n";
    }
}

int main() {
    // === KIRBY PLAYABLE CHARACTERS (641x3388, bg=0,219,255) ===
    sf::Image kirbyImg;
    kirbyImg.loadFromFile("Playable Characters y Pooderes Absorvidos/Game Boy Advance - Kirby_ Nightmare in Dream Land - Playable Characters - Kirby.png");
    sf::Color kirbyBg(0, 219, 255);
    
    std::cout << "=== KIRBY PLAYABLE CHARACTERS (641x3388) ===\n";
    std::cout << "Buscando filas de sprites (primeras 200 filas):\n";
    bool inRow = false;
    int rowStart = 0;
    for (int y = 0; y < 300; y++) {
        bool hasPixel = false;
        for (int x = 0; x < (int)kirbyImg.getSize().x; x++) {
            sf::Color c = kirbyImg.getPixel(x, y);
            if (c != kirbyBg && c.a > 0) { hasPixel = true; break; }
        }
        if (hasPixel && !inRow) { rowStart = y; inRow = true; }
        else if (!hasPixel && inRow) {
            int rowH = y - rowStart;
            std::cout << "  Row y=" << rowStart << "-" << (y-1) << " (h=" << rowH << "):\n";
            findSpritesInRow(kirbyImg, kirbyBg, rowStart, y-1);
            inRow = false;
        }
    }

    // === ANIMACIONES KIRBY (900x485, bg=white) ===
    sf::Image animImg;
    animImg.loadFromFile("Kirby Animaciones/Animaciones Kirby.png");
    sf::Color animBg(255, 255, 255);
    
    std::cout << "\n=== ANIMACIONES KIRBY (900x485) ===\n";
    inRow = false;
    for (int y = 0; y < (int)animImg.getSize().y; y++) {
        bool hasPixel = false;
        for (int x = 0; x < (int)animImg.getSize().x; x++) {
            sf::Color c = animImg.getPixel(x, y);
            if (c != animBg && c.a > 10) { hasPixel = true; break; }
        }
        if (hasPixel && !inRow) { rowStart = y; inRow = true; }
        else if (!hasPixel && inRow) {
            int rowH = y - rowStart;
            std::cout << "  Row y=" << rowStart << "-" << (y-1) << " (h=" << rowH << "):\n";
            findSpritesInRow(animImg, animBg, rowStart, y-1);
            inRow = false;
        }
    }

    // === WADDLE DEE (144x152, bg=82,158,158) ===
    sf::Image waddleImg;
    waddleImg.loadFromFile("Enemigos/Game Boy Advance - Kirby_ Nightmare in Dream Land - Enemies - Waddle Dee.png");
    sf::Color waddleBg(82, 158, 158);
    
    std::cout << "\n=== WADDLE DEE (144x152) ===\n";
    inRow = false;
    for (int y = 0; y < (int)waddleImg.getSize().y; y++) {
        bool hasPixel = false;
        for (int x = 0; x < (int)waddleImg.getSize().x; x++) {
            sf::Color c = waddleImg.getPixel(x, y);
            if (c != waddleBg) { hasPixel = true; break; }
        }
        if (hasPixel && !inRow) { rowStart = y; inRow = true; }
        else if (!hasPixel && inRow) {
            std::cout << "  Row y=" << rowStart << "-" << (y-1) << ":\n";
            findSpritesInRow(waddleImg, waddleBg, rowStart, y-1);
            inRow = false;
        }
    }
    // Last row
    if (inRow) {
        std::cout << "  Row y=" << rowStart << "-" << (waddleImg.getSize().y-1) << ":\n";
        findSpritesInRow(waddleImg, waddleBg, rowStart, waddleImg.getSize().y-1);
    }

    // === BRONTO BURT ===
    sf::Image brontoImg;
    brontoImg.loadFromFile("Enemigos/Game Boy Advance - Kirby_ Nightmare in Dream Land - Enemies - Bronto Burt.png");
    std::cout << "\n=== BRONTO BURT ===\n";
    sf::Color brontoBg = brontoImg.getPixel(0,0);
    std::cout << "BG: R=" << (int)brontoBg.r << " G=" << (int)brontoBg.g << " B=" << (int)brontoBg.b << "\n";
    inRow = false;
    for (int y = 0; y < (int)brontoImg.getSize().y; y++) {
        bool hasPixel = false;
        for (int x = 0; x < (int)brontoImg.getSize().x; x++) {
            sf::Color c = brontoImg.getPixel(x, y);
            if (c != brontoBg) { hasPixel = true; break; }
        }
        if (hasPixel && !inRow) { rowStart = y; inRow = true; }
        else if (!hasPixel && inRow) {
            std::cout << "  Row y=" << rowStart << "-" << (y-1) << ":\n";
            findSpritesInRow(brontoImg, brontoBg, rowStart, y-1);
            inRow = false;
        }
    }
    if (inRow) {
        std::cout << "  Row y=" << rowStart << "-end:\n";
        findSpritesInRow(brontoImg, brontoBg, rowStart, brontoImg.getSize().y-1);
    }

    return 0;
}
