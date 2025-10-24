#pragma once

#include "PrecompiledHeader.h"

class Camera
{
public:

private:
	DirectX::XMFLOAT3 m_eyePosition;

	DirectX::XMVECTOR m_forwardDirection;
	DirectX::XMVECTOR m_rightDirection;
	DirectX::XMVECTOR m_upDirection;

	float m_fovY;
	float m_aspect;
	float m_zNear;
	float m_zFar;
};