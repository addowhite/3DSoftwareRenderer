#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>
#include "SDL2_gfxPrimitives.h"

#define true  1
#define false 0

#define WINDOW_WIDTH  1024
#define WINDOW_HEIGHT 720
#define WINDOW_CENTER_X WINDOW_WIDTH / 2
#define WINDOW_CENTER_Y WINDOW_HEIGHT / 2

typedef struct Vector3i {   int x, y, z; } Vector3i;
typedef struct Vector3f { float x, y, z; } Vector3f;
typedef Vector3f Point;
typedef struct Camera { Vector3f position; Vector3f rotation; } Camera;
typedef struct Mesh { Point* vertices; unsigned int vertexCount; Point* lines; unsigned int lineVertexCount; } Mesh;

typedef enum RenderType { RENDER_POINTS, RENDER_LINES, RENDER_FACES } RenderType;

SDL_Window   *windowHandle  = NULL;
SDL_Surface  *windowSurface = NULL;
SDL_Renderer *renderer      = NULL;

Camera camera;
Mesh cube;

Mesh createCubeMesh(int width, int height, int depth) {
	unsigned int i;
	Mesh cubeMesh;
	cubeMesh.vertexCount = 8;
	cubeMesh.vertices = (Point*)malloc(sizeof(Point) * cubeMesh.vertexCount);
	cubeMesh.vertices[i = 0] = (Point) { -width, -height,  depth };
	cubeMesh.vertices[ ++i ] = (Point) {  width, -height,  depth };
	cubeMesh.vertices[ ++i ] = (Point) {  width,  height,  depth };
	cubeMesh.vertices[ ++i ] = (Point) { -width,  height,  depth };
	cubeMesh.vertices[ ++i ] = (Point) { -width, -height, -depth };
	cubeMesh.vertices[ ++i ] = (Point) {  width, -height, -depth };
	cubeMesh.vertices[ ++i ] = (Point) {  width,  height, -depth };
	cubeMesh.vertices[ ++i ] = (Point) { -width,  height, -depth };

	unsigned int faceCount = 4;
	Vector3i *connections = (Vector3i*)malloc(sizeof(Vector3i) * faceCount);
	connections[i = 0] = (Vector3i) { 0, 1, 2 };
	connections[ ++i ] = (Vector3i) { 0, 2, 3 };
	connections[ ++i ] = (Vector3i) { 4, 5, 6 };
	connections[ ++i ] = (Vector3i) { 4, 6, 7 };

	cubeMesh.lineVertexCount = faceCount * 4;
	cubeMesh.lines = (Point*)malloc(sizeof(Point) * cubeMesh.lineVertexCount);
	for (i = 0; i < faceCount; ++i) {
		Vector3i currentFace = connections[i];
		cubeMesh.lines[i * 4    ] = cubeMesh.vertices[currentFace.x];
		cubeMesh.lines[i * 4 + 1] = cubeMesh.vertices[currentFace.y];
		cubeMesh.lines[i * 4 + 2] = cubeMesh.vertices[currentFace.z];
		cubeMesh.lines[i * 4 + 3] = cubeMesh.vertices[currentFace.x];
	}

	free(connections);

	return cubeMesh;
}

void freeMesh(Mesh* mesh) {
	free(mesh->vertices);
	free(mesh->lines);
}

int init() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Failed to initialize SDL2 video: %s", SDL_GetError());
		SDL_Quit();
		return false;
	}

	windowHandle = SDL_CreateWindow("Untitled Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
	if (windowHandle == NULL) {
		printf("Failed to create window: %s", SDL_GetError());
		SDL_Quit();
		return false;
	}
	windowSurface = SDL_GetWindowSurface(windowHandle);

	renderer = SDL_CreateRenderer(windowHandle, -1, SDL_RENDERER_ACCELERATED);

	cube = createCubeMesh(1, 1, 1);

	camera.position.z = -3;

	return true;
}

void cleanup() {
	freeMesh(&cube);
	SDL_DestroyWindow(windowHandle);
	SDL_Quit();
}

void renderMesh(Mesh* mesh, RenderType renderType) {
	unsigned int length;
	switch (renderType) {
		case RENDER_POINTS: length = mesh->vertexCount; break;
		case RENDER_LINES: case RENDER_FACES: length = mesh->lineVertexCount; break;
		default: break;
	}

	SDL_Point* projectedPoints = (SDL_Point*)malloc(sizeof(SDL_Point) * length);

	Point currentPoint; SDL_Point *projectedPoint;

	for (unsigned int i = 0; i < length; ++i) {
		switch (renderType) {
			case RENDER_POINTS: currentPoint = mesh->vertices[i]; break;
			case RENDER_LINES: case RENDER_FACES: currentPoint = mesh->lines[i]; break;
			default: break;
		}
		projectedPoint = &projectedPoints[i];
		
		float distanceScale = 200.0f / (currentPoint.z - camera.position.z);
		projectedPoint->x = WINDOW_CENTER_X + (currentPoint.x - camera.position.x) * distanceScale;
		projectedPoint->y = WINDOW_CENTER_Y + (currentPoint.y - camera.position.y) * distanceScale;
	}

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	switch (renderType) {
		case RENDER_POINTS: SDL_RenderDrawPoints(renderer, projectedPoints, length); break;
		case RENDER_LINES:  SDL_RenderDrawLines(renderer,  projectedPoints, length); break;

		case RENDER_FACES:
			for (unsigned int i = 0; i < length; i += 4) {
				filledTrigonRGBA(renderer,
					projectedPoints[i    ].x, projectedPoints[i    ].y,
					projectedPoints[i + 1].x, projectedPoints[i + 1].y,
					projectedPoints[i + 2].x, projectedPoints[i + 2].y,
					200, 200, 200, 255
				);
			}
			break;

		default: break;
	}

	free(projectedPoints);
}

void renderAll() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	renderMesh(&cube, RENDER_LINES);
	SDL_RenderPresent(renderer);
}

int update() {
	SDL_Event ev;
	while (SDL_PollEvent(&ev) != 0)
		if (ev.type == SDL_QUIT || (ev.type == SDL_KEYUP && ev.key.keysym.sym == SDLK_ESCAPE)) return false;
	
	float cameraMovespeed = 0.1f;
	const Uint8* keyboardState = SDL_GetKeyboardState(NULL);
	if (keyboardState[SDL_SCANCODE_W] || keyboardState[SDL_SCANCODE_UP])    camera.position.z += cameraMovespeed;
	if (keyboardState[SDL_SCANCODE_S] || keyboardState[SDL_SCANCODE_DOWN])  camera.position.z -= cameraMovespeed;
	if (keyboardState[SDL_SCANCODE_A] || keyboardState[SDL_SCANCODE_LEFT])  camera.position.x -= cameraMovespeed;
	if (keyboardState[SDL_SCANCODE_D] || keyboardState[SDL_SCANCODE_RIGHT]) camera.position.x += cameraMovespeed;
	if (keyboardState[SDL_SCANCODE_SPACE])  camera.position.y -= cameraMovespeed;
	if (keyboardState[SDL_SCANCODE_LSHIFT]) camera.position.y += cameraMovespeed;

	SDL_Delay(10);
	return true;
}

int main(int argc, char** argv) {
	if (!init()) return 1;
	while (update()) renderAll();
	cleanup();
	return 0;
}