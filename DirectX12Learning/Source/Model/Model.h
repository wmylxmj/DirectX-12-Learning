#pragma once

/*
	这里定义 3D 模型
*/

#include "assimp/scene.h"
#include "DirectXMath.h"

#define NUM_BONES_PER_VERTEX 4

#define INVALID_PARENT 0XFFFFFFFF

class Model {
public:

	typedef struct {
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
	} Vertex;

protected:
};