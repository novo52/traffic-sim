#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#define OLC_PGEX_TRANSFORMEDVIEW
#include "olcPGEX_TransformedView.h"

#include <math.h>
#include <format>



class Renderer {

public:

	struct SpriteSheetPos {
		olc::vi2d pos;
		olc::vi2d size;
	};

	struct SpriteSheetRow {
		int spriteCount;
		int spriteWidth;
		int spriteHeight;
		int spriteVerticalOffset;

		olc::Pixel::Mode spriteRenderingMode;

		int rowVerticalOffset;
	};

	SpriteSheetRow* rows;

private:
	int tileWidth;
	int tileHeight;
	olc::Sprite* spriteSheet;
	olc::Decal* spriteSheetDecal;
	int rowCount;


public:
	Renderer(int tileWidth, int tileHeight, const std::string& filename)
		: tileWidth(tileWidth), tileHeight(tileHeight), rows(nullptr)
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
			olc::Pixel secondPixel = spriteSheet->GetPixel(0, yReading + 1);
			olc::Pixel::Mode transparencyMode = olc::Pixel::Mode(secondPixel.r);
			int offset = secondPixel.g;

			// Save data
			SpriteSheetRow rowData = { length, width, height, offset, transparencyMode, yReading };
			rows[row] = rowData;

			yReading += height;
		}

	}

	SpriteSheetPos GetSpriteSheetPos(int tileRow, int tileCol) {
		SpriteSheetRow& row = rows[tileRow];

		if (tileRow >= rowCount || tileCol >= row.spriteCount) {
			return GetSpriteSheetPos(0, 0); // "Null" tile
		}

		int textureY = row.rowVerticalOffset;
		int textureX = tileCol * row.spriteWidth + 1; // Cut off the first column of data
		int textureWidth = row.spriteWidth;
		int textureHeight = row.spriteHeight;

		return {
			{textureX, textureY},
			{textureWidth, textureHeight}
		};
	}

	void RenderSpriteIsometric(olc::TransformedView& tv, olc::vi2d cellPos, int tileRow, int tileCol, olc::vi2d screenSpaceOffset)
	{
		auto WorldToScreen = [&](olc::vf2d worldPos) {

			olc::vf2d screenSpaceCoordinate = {
				(worldPos.x - worldPos.y) * ((float)tileWidth * 0.5f),
				(worldPos.x + worldPos.y) * ((float)tileHeight * 0.5f)
			};

			olc::vf2d originOffsetScreenSpaceCoordinate = screenSpaceCoordinate;

			return originOffsetScreenSpaceCoordinate;

		};

		olc::vf2d screenPos = WorldToScreen(cellPos) + screenSpaceOffset;

		RenderSprite(tv, screenPos, tileRow, tileCol, olc::vf2d(1.0f, 1.0f));
	}

	void RenderSprite(olc::TransformedView& tv, olc::vf2d screenPos, int tileRow, int tileCol, olc::vf2d size) {
		SpriteSheetRow& row = rows[tileRow];

		SpriteSheetPos spriteSheetPos = GetSpriteSheetPos(tileRow, tileCol);

		int textureVerticalOffset = row.spriteVerticalOffset * tileHeight;
		screenPos.y -= textureVerticalOffset * size.y;

		// Clipping
		// Lose ~3 FPS by doing the check, but gain hundreds when only a few sprites are visible
		if (tv.IsRectVisible(screenPos, spriteSheetPos.size * size)) {

			tv.DrawPartialDecal(
				screenPos, spriteSheetDecal,
				spriteSheetPos.pos,
				spriteSheetPos.size,
				size);
		}
	}
	
	void RenderTileUpRight(olc::TransformedView& tv, olc::vi2d cellPos, int tileRow, int tileCol, olc::vi2d screenSpaceOffset) {

	}
};


// Override base class with your custom functionality
class Game : public olc::PixelGameEngine
{
public:
	Game()
	{
		// Name your application
		sAppName = "Game";
	}

	olc::TransformedView isometricTV;
	olc::TransformedView mapTV;
	olc::TransformedView ui;
	bool renderUI = false;
	// TODO: convert to enums
	int renderMode = 0; // 0 = Isometreic
	int editMode = 1; // 0 = Terrain height, 1 = Tile/overlay type

public:
	struct Tile {
		int ground;
		int overlay;
		int height;
	};

private:
	Tile* pWorldTiles = nullptr;
	Renderer* renderer = nullptr;
	int currentTile = 0;
	int currentOverlay = 0;

public:
	olc::vi2d vWorldSize = { 200, 200 };
	olc::vi2d vTileSize = { 36, 18 };


	bool OnUserCreate() override
	{
		pWorldTiles = new Tile[vWorldSize.x * vWorldSize.y]{};

		srand(69);

		// 0 = Normal (randomized terrain)
		// 1 = Stripes (good for testing sprite size/accuracy)
		// 2 = funky math generation
		const int generationMode = 0;

		for (int y = 0, i = 0; y < vWorldSize.y; y++) {
			for (int x = 0; x < vWorldSize.x; x++, i++) {
				if (generationMode == 0) {
					pWorldTiles[i].ground = (rand() % 2) + 1;
					if (pWorldTiles[i].ground == 1) {
						if (rand() % 10 != 1) continue;
						pWorldTiles[i].overlay = 1;
					}
					pWorldTiles[i].height = (rand() % 3) - 1;
				}
				else if (generationMode == 1) {
					pWorldTiles[i].ground = i % 3 + 1;
					pWorldTiles[i].overlay = 0;
					pWorldTiles[i].height = 0;
				}
				else if (generationMode == 2) {
					pWorldTiles[i].overlay = 0;
					int nx = x - (vWorldSize.x/2);
					int ny = y - (vWorldSize.y/2);
					// pringle shaped terrain
					pWorldTiles[i].height = (nx * nx - ny * ny) / 128; 
					const int layerCount = 13;
					// Tile type based on distance from the centre and height
					pWorldTiles[i].ground =  ((int)(sqrtf(nx * nx + ny * ny) / ((float)vWorldSize.x) * layerCount * 2) + (abs(pWorldTiles[i].height) / 8)) % 3 + 1;
				}
			}
		}

		renderer = new Renderer(vTileSize.x, vTileSize.y, "assets/spritesheet.png");

		isometricTV.Initialise({ScreenWidth(), ScreenHeight()});
		return true;
	}

	olc::vf2d WorldToScreen(float x, float y) {

		olc::vf2d screenSpaceCoordinate = {
			(x - y) * ((float)vTileSize.x * 0.5f),
			(x + y) * ((float)vTileSize.y * 0.5f)
		};

		return screenSpaceCoordinate;
	};

	olc::vf2d ScreenToWorld(float x, float y) {

		float u = x / (float)vTileSize.x;
		float v = y / (float)vTileSize.y;

		return olc::vf2d(u + v, v - u);
	};

	std::tuple<olc::vi2d, olc::vf2d> WorldToCell(float x, float y) {

		olc::vi2d vCell = olc::vi2d(x, y);
		olc::vf2d vCellWithin = olc::vf2d(x - vCell.x, y - vCell.y);

		// Fix negative coordinates rounding to int incorrectly
		if (x < 0) {
			vCell.x--;
			vCellWithin.x++;
		}
		if (y < 0) {
			vCell.y--;
			vCellWithin.y++;
		}

		return { vCell, vCellWithin };
	};

	bool OnUserUpdate(float fElapsedTime) override
	{

		Clear(olc::WHITE);

		if (renderMode == 0) {

			// Handle pan
			isometricTV.HandlePanAndZoom(2, 0.0f, true, false);
			// Handle zoom
			HandleMultiplicativeZoom(isometricTV, 1.41421356237f); // sqrt(2), so every second zoom level is an exact power of 2
			
			// Use mouse position to determine sleected cell and related info
			olc::vi2d vMouseScreen = isometricTV.ScreenToWorld(GetMousePos()); // Mouse pos in screen space
			olc::vf2d vMouseWorld = ScreenToWorld(vMouseScreen.x, vMouseScreen.y) + olc::vf2d(-0.5f, 0.5f); // Mouse pos in world space
			olc::vi2d vSelectedCell; // Cell index of mouse pos in world space
			olc::vf2d vSelectedCellWithinWorld; // Mouse pos within its cell in world space
			std::tie(vSelectedCell, vSelectedCellWithinWorld) = WorldToCell(vMouseWorld.x, vMouseWorld.y);
			olc::vi2d vCellWithinScreen = vMouseScreen - WorldToScreen(vSelectedCell.x, vSelectedCell.y); // Mouse pos within cell in screen space
			olc::vi2d vSelectedCellScreen = WorldToScreen(vSelectedCell.x, vSelectedCell.y); // Screen pos of selected cell


			// Toggle UI on H
			if (GetKey(olc::Key::H).bPressed) renderUI = !renderUI;

			if (editMode == 0) {
				HandleTerraingHeightEdit(vSelectedCell);
				renderUI = false;
			}
			else if (editMode == 1) {
				HandleTileTypeAndOverlayEdit(vSelectedCell);
			}

			RenderIsometricWorld(vSelectedCell);
			

			// Inventory
			// WARNING: horrid code
			// TODO: refactor (delete and replace)

			if (renderUI) {

				const olc::vf2d invTileSize = olc::vf2d(3.0f, 3.0f);
				const olc::vf2d uiStartPos = olc::vf2d(vTileSize.x, ScreenHeight() - (float)vTileSize.y * 7.5f);

				FillRectDecal(uiStartPos - olc::vf2d(0.5f, invTileSize.y + 0.5f) * vTileSize, (olc::vf2d(2.0f, 3.0f) * invTileSize + olc::vf2d(1.0f, 1.0f)) * vTileSize, olc::GREY);
				if (currentTile == 0) {
					renderer->RenderSprite(ui, uiStartPos, 3, 0, invTileSize);
				}
				else {
					renderer->RenderSprite(ui, uiStartPos, 2, currentTile, invTileSize);
				}
				renderer->RenderSprite(ui, uiStartPos + olc::vf2d(vTileSize.x * invTileSize.x, 0.0f), 2, 0, invTileSize);
				renderer->RenderSprite(ui, uiStartPos + olc::vf2d(vTileSize.x * invTileSize.x, 0.0f), 4, currentOverlay, invTileSize);

				const olc::vf2d textScale = olc::vf2d(4, 4);
				const olc::vf2d centreOffset = olc::vf2d(0.5f, 0.5f) * invTileSize * vTileSize - textScale * 0.5f * 8.0f;
				const olc::vf2d textOffset = centreOffset + (olc::vf2d(0.0f, 1.0f) * invTileSize + olc::vf2d(0.0f, 0.5f)) * vTileSize;

				DrawStringDecal(uiStartPos + textOffset,
					"Q", olc::BLACK, textScale);
				DrawStringDecal(uiStartPos + textOffset + olc::vf2d(1.0f, 0.0f) * invTileSize * vTileSize,
					"E", olc::BLACK, textScale);
				DrawStringDecal(uiStartPos - olc::vf2d(0.5f, invTileSize.y + 0.5f) * vTileSize + olc::vf2d(0.0f, ((olc::vf2d(2.0f, 3.0f) * invTileSize + olc::vf2d(1.0f, 1.0f)) * vTileSize).y),
					"H to hide/show", olc::BLACK, olc::vf2d(1.0f, 1.0f));
			}

			// Debug info

#ifdef DEBUG
#define NAME_LENGTH "20"
			std::vector<std::string> messages = {
				std::format("{:^" NAME_LENGTH "}:{:>6.3f}, {:>6.3f}",	"Mouse (World)",		vMouseWorld.x,					vMouseWorld.y),
				std::format("{:^" NAME_LENGTH "}:{:>6.3f},{:>6.3f}",	"Within (World)",		vSelectedCellWithinWorld.x,		vSelectedCellWithinWorld.y),
				std::format("{:^" NAME_LENGTH "}:{:>6d}, {:>6d}",		"Within (Screen)",		vCellWithinScreen.x,			vCellWithinScreen.y),
				std::format("{:^" NAME_LENGTH "}:{:>6d}, {:>6d}",		"Selected",				vSelectedCell.x,				vSelectedCell.y)
			};

			const float scale = 2;
			for (int i = 0; i < messages.size(); i++) {
				olc::vf2d pos = olc::vf2d(4, 10 * i + 4) * scale;
				DrawStringDecal(pos, messages[i], olc::BLACK, olc::vf2d(scale, scale));
			}
#endif
		}
		else if (renderMode == 1) { // 2d map mode

			mapTV.HandlePanAndZoom();

		
			for (int y = 0; y < vWorldSize.y; y++) {
				for (int x = 0; x < vWorldSize.x; x++) {

				}
			}
		}
		
		return true;
	}

	void HandleMultiplicativeZoom(olc::TransformedView& tv, const float zoomFactor)
	{
		const olc::vi2d vMousePos = GetMousePos();
		const int vMouseWheel = GetMouseWheel();
		if (vMouseWheel > 0) tv.ZoomAtScreenPos(zoomFactor, vMousePos);
		if (vMouseWheel < 0) tv.ZoomAtScreenPos(1.0f / zoomFactor, vMousePos);
	}

	void HandleTerraingHeightEdit(olc::vi2d vSelectedCell) {
		if (GetMouse(0).bPressed) {
			if (vSelectedCell.x >= 0 && vSelectedCell.x < vWorldSize.x && vSelectedCell.y >= 0 && vSelectedCell.y < vWorldSize.y) {
				int i = vSelectedCell.y * vWorldSize.x + vSelectedCell.x;
				pWorldTiles[i].height++;
			}
		}

		if (GetMouse(1).bPressed) {
			if (vSelectedCell.x >= 0 && vSelectedCell.x < vWorldSize.x && vSelectedCell.y >= 0 && vSelectedCell.y < vWorldSize.y) {
				int i = vSelectedCell.y * vWorldSize.x + vSelectedCell.x;
				pWorldTiles[i].height--;
			}
		}
	}

	void HandleTileTypeAndOverlayEdit(olc::vi2d vSelectedCell) {
		if (GetMouse(0).bHeld) {
			if (vSelectedCell.x >= 0 && vSelectedCell.x < vWorldSize.x && vSelectedCell.y >= 0 && vSelectedCell.y < vWorldSize.y) {
				int i = vSelectedCell.y * vWorldSize.x + vSelectedCell.x;
				pWorldTiles[i].ground = currentTile;

				if (pWorldTiles[i].ground == 3 || pWorldTiles[i].ground == 0)
					pWorldTiles[i].overlay = 0; // No plants of water and stone
			}
		}
		if (GetMouse(1).bHeld) {
			if (vSelectedCell.x >= 0 && vSelectedCell.x < vWorldSize.x && vSelectedCell.y >= 0 && vSelectedCell.y < vWorldSize.y) {
				int i = vSelectedCell.y * vWorldSize.x + vSelectedCell.x;
				if (pWorldTiles[i].ground != 3 && pWorldTiles[i].ground != 0) { 
					pWorldTiles[i].overlay = currentOverlay; // No plants of water and stone
				}
			}
		}

		if (GetKey(olc::Key::Q).bPressed) {
			currentTile++;
			currentTile %= 4;
		}
		if (GetKey(olc::Key::E).bPressed) {
			currentOverlay++;
			currentOverlay %= 3;
		}
	}

	void RenderIsometricWorld(olc::vi2d vSelectedCell) {
		const int heightMultiplier = -9;
		SetDecalMode(olc::DecalMode::NORMAL);
		for (int y = 0; y < vWorldSize.y; y++) {
			for (int x = 0; x < vWorldSize.x; x++) {

				int worldIndex = y * vWorldSize.x + x;
				int groundType = pWorldTiles[worldIndex].ground;
				int overlayType = pWorldTiles[worldIndex].overlay;
				int height = pWorldTiles[worldIndex].height * heightMultiplier;

				int groundTileRow = 2;

				bool isWater = (groundType == 0);
				if (isWater) {
					height = heightMultiplier;
					groundTileRow = 3;
				}

				if (pWorldTiles[worldIndex].height == 1 &&
					pWorldTiles[worldIndex + 1].height == 0 &&
					pWorldTiles[worldIndex + vWorldSize.x].height == 0 &&
					pWorldTiles[worldIndex + vWorldSize.x + 1].height == 0 && false) {
				}
				else {
					renderer->RenderSpriteIsometric(isometricTV, { x, y }, groundTileRow, groundType, { 0, height });

					if (x == vSelectedCell.x && y == vSelectedCell.y) {
						renderer->RenderSpriteIsometric(isometricTV, vSelectedCell, 1, 0, { 0, height });
					}

					renderer->RenderSpriteIsometric(isometricTV, { x, y }, 4, overlayType, { 0, height });
				}
			}
		}
	}
};

int main()
{
	Game demo;
	if (demo.Construct(1280, 720, 1, 1))
		demo.Start();
	return 0;
}