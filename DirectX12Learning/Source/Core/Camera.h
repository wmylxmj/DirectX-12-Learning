#pragma once

#include "PrecompiledHeader.h"

class Camera
{
public:
	Camera();

	DirectX::XMMATRIX GetViewMatrix();
	DirectX::XMMATRIX GetProjectionMatrix();

private:
	void UpdateViewMatrix();
	void UpdateProjectionMatrix();

	DirectX::XMVECTOR m_eyePosition;

	DirectX::XMVECTOR m_forwardDirection;
	DirectX::XMVECTOR m_rightDirection;
	DirectX::XMVECTOR m_upDirection;

	float m_fovY;
	float m_aspect;
	float m_zNear;
	float m_zFar;

	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_projectionMatrix;

	// �����Ƿ���Ҫ����
	bool m_viewMatrixNeedsUpdate;
	bool m_projectionMatrixNeedsUpdate;
};