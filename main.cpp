#include "olcPixelGameEngine.h"
#include <math.h>
#include <format>
#include "Renderer.h"

#include "extensions/olcPGEX_TransformedView.h"


// Override base class with your custom functionality
class Game : public olc::PixelGameEngine
{
public:
	Game()
	{
		// Name your application
		sAppName = "Game";
	}

	olc::TransformedView tv;

private:
	int* pWorldGround = nullptr;
	int* pWorldOverlay = nullptr;
	Renderer* renderer = nullptr;

public:
	olc::vi2d vWorldSize = { 14, 10 };
	olc::vi2d vTileSize = { 40, 20 };
	olc::vi2d vOrigin = { 5, 6 };


	bool OnUserCreate() override
	{
		pWorldGround = new int[vWorldSize.x * vWorldSize.y]{ 0 };

		for (int i = 0; i < vWorldSize.x * vWorldSize.y; i++) {
			pWorldGround[i] = (rand() % 3) + 1;
		}

		pWorldOverlay = new int[vWorldSize.x * vWorldSize.y]{ 0 };
		for (int i = 0; i < vWorldSize.x * vWorldSize.y; i++) {
			if (pWorldGround[i] == 3) continue;
			if (rand() % 3 != 1) continue;
			pWorldOverlay[i] = (rand() % 2) + 1;
		}
		renderer = new Renderer(vTileSize.x, vTileSize.y, "spritesheet.png");

		tv.Initialise({ScreenWidth(), ScreenHeight()});
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		tv.HandlePanAndZoom();
		Clear(olc::WHITE);

		auto WorldToScreen = [&](float x, float y) {

			olc::vf2d screenSpaceCoordinate = {
				(x-y) * ((float)vTileSize.x * 0.5f),
				(x+y) * ((float)vTileSize.y * 0.5f)
			};

			olc::vf2d originOffsetScreenSpaceCoordinate = screenSpaceCoordinate + (olc::vf2d)vOrigin * (olc::vf2d)vTileSize;

			return originOffsetScreenSpaceCoordinate;

		};

		auto ScreenToWorld = [&](float x, float y) {

			float u = (x - ((float)vOrigin.x * (float)vTileSize.x)) * 2.0f / ((float)vTileSize.x);
			float v = (y - ((float)vOrigin.y * (float)vTileSize.y)) * 2.0f / ((float)vTileSize.y);

			return olc::vf2d((u + v) *0.5f, (v - u) * 0.5f);
		};

		auto WorldToCell = [&](float x, float y) {

			olc::vi2d vCell = olc::vi2d(x, y);
			olc::vf2d vCellWithin = olc::vf2d(x - vCell.x, y - vCell.y);

			if (x < 0) {
				vCell.x--;
				vCellWithin.x++;
			}
			if (y < 0) {
				vCell.y--;
				vCellWithin.y++;
			}

			return std::tuple<olc::vi2d, olc::vf2d>(vCell, vCellWithin);
		};
				
		olc::vi2d vMouseScreen = tv.ScreenToWorld(GetMousePos()); // Mouse pos in screen space
		
		olc::vf2d vMouseWorld = ScreenToWorld(vMouseScreen.x, vMouseScreen.y) + olc::vf2d(-0.5f, 0.5f); // Mouse pos in world space

		olc::vi2d vSelectedCell; // Cell index of mouse pos in world space
		olc::vf2d vSelectedCellWithinWorld; // Mouse pos within its cell in world space
		std::tie(vSelectedCell, vSelectedCellWithinWorld) = WorldToCell(vMouseWorld.x, vMouseWorld.y); 
		
		olc::vi2d vCellWithinScreen = vMouseScreen - WorldToScreen(vSelectedCell.x, vSelectedCell.y); // Mouse pos within cell in screen space

		olc::vi2d vSelectedCellScreen = WorldToScreen(vSelectedCell.x, vSelectedCell.y);

		if (GetMouse(0).bPressed) {
			int i = vSelectedCell.y * vWorldSize.x + vSelectedCell.x;
			pWorldGround[i]++;
			pWorldGround[i] %= 5;
		}
		if (GetMouse(1).bPressed) {
			int i = vSelectedCell.y * vWorldSize.x + vSelectedCell.x;
			pWorldOverlay[i]++;
			pWorldOverlay[i] %= 3;
		}


		SetDecalMode(olc::DecalMode::NORMAL);
		for (int y = 0; y < vWorldSize.y; y++) { 
			for (int x = 0; x < vWorldSize.x; x++) {
				olc::vf2d vCellScreen = WorldToScreen(x, y);

				int worldIndex = y * vWorldSize.x + x;
				int groundTileIndex = pWorldGround[worldIndex];
				
				renderer->RenderSprite(this, tv, vCellScreen.x, vCellScreen.y, 2, groundTileIndex);

			}
		}
		
		renderer->RenderSprite(this, tv, vSelectedCellScreen.x, vSelectedCellScreen.y, 1, 0);
		

		for (int y = 0; y < vWorldSize.y; y++) {
			for (int x = 0; x < vWorldSize.x; x++) {
				olc::vf2d vCellScreen = WorldToScreen(x, y);

				int worldIndex = y * vWorldSize.x + x;
				int overlayTileIndex = pWorldOverlay[worldIndex];
				renderer->RenderSprite(this, tv, vCellScreen.x, vCellScreen.y, 4, overlayTileIndex);
			}
		}

		// Debug info
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
		
		return true;
	}
};

int main()
{
	Game demo;
	if (demo.Construct(1280, 720, 1, 1))
		demo.Start();
	return 0;
}