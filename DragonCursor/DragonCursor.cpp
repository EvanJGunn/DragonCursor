// DragonCursor.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <windows.h>
#include <strsafe.h>
#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

void redrawWindow(HWND myWindow) {
	// Clear old markings
	RECT myRec = { 0, 0, 10000, 10000 };
	RedrawWindow(myWindow, &myRec, NULL, RDW_NOERASE | RDW_INVALIDATE | RDW_UPDATENOW);
}

void DrawImage(POINT origin, POINT midPoint, int angle, Graphics *g, Image *image) {

	// Rotate image
	g->TranslateTransform(origin.x, origin.y);
	g->RotateTransform(angle);
	g->TranslateTransform(-origin.x, -origin.y);

	// Set up alpha
	ImageAttributes imAtt;
	imAtt.SetColorKey(Color(255, 255, 255), Color(255, 255, 255), ColorAdjustTypeBitmap);

	// Draw image
	g->TranslateTransform(midPoint.x, midPoint.y);
	Rect myRect(0, 0, 119, 72);
	g->DrawImage(image, myRect, 0, 0, 119, 72, UnitPixel, &imAtt, NULL,NULL);

	// g->DrawImage(image,midPoint.x,midPoint.y);
	g->ResetTransform();
}

float getDotProduct(POINT a, POINT b) {
	return (a.x*b.x) + (a.y*b.y);
}

float getVectorLength(POINT a) {
	return sqrt(pow(a.x,2) + pow(a.y,2));
}

float distance(POINT a, POINT b) {
	return sqrt(pow((a.x-b.x),2) + pow((a.y-b.y),2));
}

#define PI 3.14159265

// Get the angle from point a to point b,
// with point a as the origin
float getAngle(POINT a, POINT b) {
	float lengthMult = ((getVectorLength(a)*getVectorLength(b)));
	if (lengthMult == 0) return acos(0) * 180.0 / PI;;
	return acos(getDotProduct(a, b) /lengthMult) * 180.0 / PI;;
}

POINT normalize(POINT a) {
	POINT retVal(a);

	float length = getVectorLength(retVal);
	retVal.x = retVal.x / length;
	retVal.y = retVal.y / length;
	return retVal;
}

POINT movePoint(POINT origin, POINT direction, float distance) {
	POINT retVal(origin);

	POINT norm = normalize(direction);
	norm.x *= distance;
	norm.y *= distance;

	retVal.x += norm.x;
	retVal.y += norm.y;
	return retVal;
}

float minBodyMoveDist = 50;
const int bodySegmentCount = 22;
POINT bodyPos[bodySegmentCount];
float bodyAngle[bodySegmentCount];

void moveBody(POINT newPos, int newAngle) {
	for (int i = bodySegmentCount - 1; i > 0; i--) {
		bodyPos[i] = bodyPos[i - 1];
		bodyAngle[i] = bodyAngle[i - 1];
	}
	bodyPos[0] = newPos;
	bodyAngle[0] = newAngle;
}

int main()
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);

	HINSTANCE hInstance = GetModuleHandle(NULL);

	HWND hProgMan = NULL;
	HWND hShell = NULL;
	hProgMan = FindWindow("Progman", "Program Manager");
	hShell = FindWindowEx(hProgMan, 0, "SHELLDLL_DefView", NULL);

	HWND listView = FindWindowEx(hShell, 0, "SysListView32", "FolderView");

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;

	// Initialize GDI+.
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// Get the hdc
	HDC hdc = GetDC(listView);
	Graphics g(hdc);

	// Load dragon image
	Image bm(L"./Dragon.bmp");
	int imageOffsetX = (bm.GetWidth() / 2);
	int imageOffsetY = (bm.GetHeight() / 2);

	// Load dragon body
	Image bmBody(L"./DragonBody.bmp");

	// Set up the dragon body
	POINT bodyStart;
	GetCursorPos(&bodyStart);

	for (int i = 0; i < bodySegmentCount; i++) {
		bodyPos[i] = bodyStart;
		bodyAngle[i] = 0;
	}

	// Track the old point so that new images are only drawn when the cursor moves
	POINT oldOrigin;
	GetCursorPos(&oldOrigin);
	int oldAngle = 0;

	POINT vectorRight = {1,0};
	float tracker = 0;
	while (1) {
		// Get the position of the cursor
		POINT origin;
		GetCursorPos(&origin);

		// Change the position so that the image is centered on the cursor
		POINT midPoint(origin);
		midPoint.x -= imageOffsetX;
		midPoint.y -= imageOffsetY;

		// Get the angle of the dragon head based on the difference
		// between the origin and the old origin
		POINT direction;
		direction.x = oldOrigin.x - origin.x;
		direction.y = oldOrigin.y - origin.y;
		
		float angle = 0;
		if (direction.y <= 0) {
			angle = -getAngle(vectorRight, direction) - 180;
		}
		else {
			angle = getAngle(vectorRight, direction) - 180;
		}

		if (origin.x != oldOrigin.x || origin.y != oldOrigin.y) {
			// std::cout << "\nANGLE: " << angle << "... VECTOR: " << direction.x << "," << direction.y;
			// Clear old image
			redrawWindow(listView);

			// Set up direction for image to be displayed
			//POINT negativeDir(direction);
			//negativeDir.x = -negativeDir.x;
			//negativeDir.y = -negativeDir.y;
			//POINT newMidPoint = movePoint(midPoint, direction, 50);

			// Move the body
			if (distance(bodyPos[0], origin) > minBodyMoveDist) {
				moveBody(oldOrigin, oldAngle);
			}

			// Draw the body, but not the first two segments, as that would be on the head
			for (int i = 1; i < bodySegmentCount; i++) {
				POINT midBodyPoint(bodyPos[i]);
				midBodyPoint.x -= imageOffsetX;
				midBodyPoint.y -= imageOffsetY;

				DrawImage(bodyPos[i], midBodyPoint, bodyAngle[i], &g, &bmBody);
			}

			// Draw the head
			DrawImage(origin, midPoint, angle, &g, &bm);
		}

		// Set the old origin & angle
		oldOrigin.x = origin.x;
		oldOrigin.y = origin.y;
		oldAngle = angle;
	}

	// Clear the dragon
	redrawWindow(listView);

	// Release the hdc
	ReleaseDC(listView, hdc);

	// Shutdown gdiplus
	GdiplusShutdown(gdiplusToken);
}


