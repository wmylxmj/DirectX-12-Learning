#pragma once

#include "PrecompiledHeader.h"

class Camera
{
public:
	Camera();

	DirectX::XMMATRIX GetViewMatrix();
	DirectX::XMMATRIX GetProjectionMatrix();

	void RotatePosition(const DirectX::XMFLOAT3& axisPosition, const DirectX::XMFLOAT3& axisDirection, float angleInRadians);
	void RotateDirection(const DirectX::XMFLOAT3& axisDirection, float angleInRadians);

private:
	void UpdateViewMatrix();
	void UpdateProjectionMatrix();

	DirectX::XMVECTOR m_eyePosition;

	DirectX::XMVECTOR m_forwardDirection;
	DirectX::XMVECTOR m_rightDirection;
	DirectX::XMVECTOR m_upDirection;

	float m_fovY;
	float m_aspectRatio;
	float m_zNear;
	float m_zFar;

	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_projectionMatrix;

	// 矩阵是否需要更新
	bool m_viewMatrixNeedsUpdate;
	bool m_projectionMatrixNeedsUpdate;
};