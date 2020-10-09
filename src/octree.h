#pragma once

#include "scene.h"

#define TopLeftFront 0 
#define TopRightFront 1 
#define BottomRightFront 2 
#define BottomLeftFront 3 
#define TopLeftBottom 4 
#define TopRightBottom 5 
#define BottomRightBack 6 
#define BottomLeftBack 7 

class Octree {
private:
	Triangle* curTri;

	glm::vec3 topLeftFront;
	glm::vec3 bottomRightBack;

	Octree *children[8];

public:
	Octree();
	Octree(int idx, glm::vec3 vert1, glm::vec3 vert2, glm::vec3 vert3);
	Octree(float x1, float y1, float z1, float x2, float y2, float z2);
	void insert(int Idx, const glm::vec3& vert1, const glm::vec3& vert2, const glm::vec3& vert3);
};



