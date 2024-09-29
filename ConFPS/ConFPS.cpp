#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <Windows.h>

int nScreenWidth = 120;
int nScreenHeight = 40;

float fPlayerX = 4.0f;
float fPlayerY = 4.0f;
float fPlayerA = 0.0f;

int nMapHeight = 16;
int nMapWidth = 16;

float fFOV = 3.14159 / 4.0;
float fDepth = 16.0f;

int main()
{
	// Create screen buffer
	wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	std::wstring map;

	map += L"################";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#...........####";
	map += L"#..............#";
	map += L"#......##......#";
	map += L"#......##......#";
	map += L"#..............#";
	map += L"####...........#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"################";

	auto tp1 = std::chrono::system_clock::now();
	auto tp2 = std::chrono::system_clock::now();

	// Game loop
	while (true)
	{
		tp2 = std::chrono::system_clock::now();
		std::chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		/*Controls*/
		// Handle CCW Rotation
		if (GetAsyncKeyState((unsigned short)'A') & 0X8000)
		{
			fPlayerA -= (0.8f) * fElapsedTime;
		}

		if (GetAsyncKeyState((unsigned short)'D') & 0X8000)
		{
			fPlayerA += (0.8f) * fElapsedTime;
		}

		if (GetAsyncKeyState((unsigned short)'W') & 0X8000)
		{
			float nextX = fPlayerX + (cosf(fPlayerA) * 5.0f * fElapsedTime);
			float nextY = fPlayerY + (sinf(fPlayerA) * 5.0f * fElapsedTime);

			if (map[(int)nextY * nMapWidth + (int)nextX] != '#')
			{
				fPlayerX = nextX;
				fPlayerY = nextY;
			}
		}

		if (GetAsyncKeyState((unsigned short)'S') & 0X8000)
		{
			float nextX = fPlayerX - (cosf(fPlayerA) * 5.0f * fElapsedTime);
			float nextY = fPlayerY - (sinf(fPlayerA) * 5.0f * fElapsedTime);

			if (map[(int)nextY * nMapWidth + (int)nextX] != '#')
			{
				fPlayerX = nextX;
				fPlayerY = nextY;
			}
		}

		for (int x = 0; x < nScreenWidth; x++)
		{
			// For each column, calculate the projected ray angle into the world space
			float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;
			float fDistanceToWall = 0;

			// Unit vector for ray in player space
			float fEyeX = cosf(fRayAngle);
			float fEyeY = sinf(fRayAngle);

			bool bHitWall = false;
			bool bHitBoundary = false;

			while (!bHitWall && fDistanceToWall < fDepth)
			{
				fDistanceToWall += 0.1f;

				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				// Test if ray is out of bounds
				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
				{					
					// Just set distance to maximum depth
					fDistanceToWall = fDepth;
					
					bHitWall = true;
				}
				else
				{
					// Ray is inbounds so test to see if the ray cell is a wall or block
					if (map[nTestY * nMapWidth + nTestX] == '#')
					{
						bHitWall = true;

						// perfect corners
						std::vector<std::pair<float, float>> p; // distance, dot
						for (int tx = 0; tx < 2; tx++)
						{
							for (int ty = 0; ty < 2; ty++)
							{
								float vx = (float)nTestX + tx - fPlayerX;
								float vy = (float)nTestY + ty - fPlayerY;
								float d = sqrt(vx * vx + vy * vy);
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(std::make_pair(d, dot));
							}
						}

						// Sort pairs form closest to furthest
						std::sort(p.begin(), p.end(), [](const std::pair<float, float>& left, const std::pair<float, float>& right)
						{
							return left.first < right.first;
						});

						float fBound = 0.01;
						if (acos(p.at(0).second) < fBound) 
							bHitBoundary = true;
						if (acos(p.at(1).second) < fBound)
							bHitBoundary = true;
					}
				}
			}

			// Calculate distance to ceiling and floor
			int nCeiling = (float(nScreenHeight / 2.0) - nScreenHeight / (float)fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;

			short nShade = ' ';
			if (fDistanceToWall <= fDepth / 4.0f)
				nShade = 0x2588; // Very close
			else if (fDistanceToWall < fDepth / 3.0f)
				nShade = 0x2593; 
			if (fDistanceToWall < fDepth / 2.0f)
				nShade = 0x2592; 
			else if (fDistanceToWall < fDepth)
				nShade = 0x2591; 
			else
				nShade = ' '; // Too far away

			if (bHitBoundary)
				nShade = ' ';

			for (int y = 0; y < nScreenHeight; y++)
			{
				if (y < nCeiling) 
				{
					screen[y * nScreenWidth + x] = ' ';
				}
				else if (y > nCeiling && y <= nFloor)
				{
					screen[y * nScreenWidth + x] = nShade;
				}
				else
				{
					short floorShade = ' ';
					// Shade flooe based on distance
					float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
					if (b < 0.25f)
						floorShade = '#';
					else if (b < 0.5f)
						floorShade = 'x';
					else if (b < 0.75f)
						floorShade = '.';
					else if (b < 0.9f)
						floorShade = '-';
					else
						floorShade = ' ';

					screen[y * nScreenWidth + x] = floorShade;
				}
			}
		}

		// Display Stats
		swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.3f FPS=%3.2f", fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);

		// Display Map
		for (int nx = 0; nx < nMapWidth; nx++)
		{
			for (int ny = 0; ny < nMapWidth; ny++)
			{
				screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
			}
		}
		screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX] = 'P';

		// Write to the screen
		screen[nScreenWidth * nScreenHeight - 1] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	}
}