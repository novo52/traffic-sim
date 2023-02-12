#pragma once
#include <string>
#include "olcPixelGameEngine.h"
#include "extensions/olcPGEX_TransformedView.h"

struct SpriteSheetRow {
	int spriteCount;
	int spriteWidth;
	int spriteHeight;
	int spriteVerticalOffset;

	olc::Pixel::Mode spriteRenderingMode;

	int rowVerticalOffset;
};

class Renderer
{
public:
	Renderer(int tileWidth, int tileHeight, const std::string& filename);
	void RenderSprite(olc::PixelGameEngine* game, olc::TransformedView &tv, int x, int y, int tileType, int tileIndex);

	SpriteSheetRow* rows;

private:
	int tileWidth;
	int tileHeight;
	olc::Sprite* spriteSheet;
	olc::Decal* spriteSheetDecal;
	int rowCount;
};

