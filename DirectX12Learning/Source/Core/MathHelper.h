#pragma once

#include <DirectXMath.h>

class RotMatrix {
	static DirectX::XMMATRIX XM_CALLCONV Rodrigues(DirectX::FXMVECTOR rotAxis, float rotAngle);
};