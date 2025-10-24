#include "Camera.h"

Camera::Camera()
{
	// 设置相机初始参数
	m_eyePosition = DirectX::XMFLOAT3(0.0f, 0.0f, -5.0f);

	m_forwardDirection = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	m_upDirection = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_rightDirection = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

	m_fovY = DirectX::XM_PIDIV4;
	m_aspect = 16.0f / 9.0f;
	m_zNear = 0.1f;
	m_zFar = 1000.0f;
}

void Camera::UpdateProjectionMatrix()
{
	m_projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(m_fovY, m_aspect, m_zNear, m_zFar);
}