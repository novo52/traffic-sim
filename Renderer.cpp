#include "Renderer.h"

Renderer::Renderer(int tileWidth, int tileHeight, const std::string& filename) 
	: tileWidth(tileWidth), tileHeight(tileHeight) 
{

	spriteSheet = new olc::Sprite(filename);
	spriteSheetDecal = new olc::Decal(spriteSheet);

	// Read in sprite data from png file
	rowCount = 0;

	// First read: determine number of rows
	for (int yReading = 0; yReading < spriteSheet->height; rowCount++)
	{
		olc::Pixel firstPixel = spriteSheet->GetPixel(0, yReading);
		printf("(%d, %d, %d, %d) at (%d, %d)\n", firstPixel.r, firstPixel.g, firstPixel.b, firstPixel.a, 0, yReading);
		int rowHeight = firstPixel.g;
		if (rowHeight == 0) {
			printf("Error reading spritesheet: pixel at (0, %d) has no height data", yReading);
			return;
		}
		yReading += rowHeight;
	}

	rows = new SpriteSheetRow[rowCount]{};

	// Load data from pixels in col 0

	for (int yReading = 0, row = 0; row < rowCount; row++)
	{

		// First pixel: dimensions of the row
		olc::Pixel firstPixel = spriteSheet->GetPixel(0, yReading);
		int width = firstPixel.r;
		int height = firstPixel.g;
		int length = firstPixel.b;

		// Second pixel: data about the tiles
		olc::Pixel secondPixel = spriteSheet->GetPixel(0, yReading+1);
		olc::Pixel::Mode transparencyMode = olc::Pixel::Mode(secondPixel.r);
		int offset = secondPixel.g;

		// Save data
		SpriteSheetRow rowData = { length, width, height, offset, transparencyMode, yReading };
		rows[row] = rowData;

		yReading += height;
	}

}
void Renderer::RenderSprite(olc::PixelGameEngine *game, olc::TransformedView &tv, int x, int y, int tileRow, int tileCol)
{

	SpriteSheetRow &row = rows[tileRow];

	if (tileRow >= rowCount || tileCol >= row.spriteCount) {
		RenderSprite(game, tv, x, y, 0, 0); // "Null" tile
		return;
	}

	int textureY = row.rowVerticalOffset;
	int textureX = tileCol * row.spriteWidth + 1; // Cut off the first column of data
	int textureWidth = row.spriteWidth;
	int textureHeight = row.spriteHeight;

	int textureVerticalOffset = row.spriteVerticalOffset * tileHeight;

	/*if (tileRow == 2 && tileCol == 1) {
		printf("(%d, %d - (%d * %d)), (%d, %d), %dx%d\n",
			x, y, row.spriteVerticalOffset, tileHeight,
			textureX, textureY,
			textureWidth,
			textureHeight);
	}*/

	olc::vf2d screenPos = olc::vf2d(x, y - textureVerticalOffset);

	tv.DrawPartialDecal(
		screenPos, spriteSheetDecal,
		olc::vf2d(textureX, textureY),
		olc::vf2d(textureWidth, textureHeight));
	return;
	
}



