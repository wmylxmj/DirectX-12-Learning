#include "Camera.h"

Camera::Camera()
{
	// 设置相机初始参数
	m_eyePosition = DirectX::XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f);

	m_forwardDirection = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	m_upDirection = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_rightDirection = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

	m_fovY = DirectX::XM_PIDIV4;
	m_aspect = 16.0f / 9.0f;
	m_zNear = 0.1f;
	m_zFar = 1000.0f;

	UpdateViewMatrix();
	UpdateProjectionMatrix();

	m_viewMatrixNeedsUpdate = false;
	m_projectionMatrixNeedsUpdate = false;
}

DirectX::XMMATRIX Camera::GetViewMatrix()
{
	if (m_viewMatrixNeedsUpdate)
	{
		UpdateViewMatrix();
		m_viewMatrixNeedsUpdate = false;
	}
	return m_viewMatrix;
}

DirectX::XMMATRIX Camera::GetProjectionMatrix()
{
	if (m_projectionMatrixNeedsUpdate)
	{
		UpdateProjectionMatrix();
		m_projectionMatrixNeedsUpdate = false;
	}
	return m_projectionMatrix;
}

void Camera::RotatePosition(const DirectX::XMFLOAT3& axisPosition, const DirectX::XMFLOAT3& axisDirection, float angleInRadians)
{
	DirectX::XMVECTOR axisPosVec = DirectX::XMLoadFloat3(&axisPosition);
	axisPosVec = DirectX::XMVectorSetW(axisPosVec, 1.0f);

	DirectX::XMVECTOR axisDirVec = DirectX::XMLoadFloat3(&axisDirection);
	axisDirVec = DirectX::XMVector3Normalize(axisDirVec);

	DirectX::XMVECTOR axisPosToEyeVec = DirectX::XMVectorSubtract(m_eyePosition, axisPosVec);
	DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationAxis(axisDirVec, angleInRadians);
	DirectX::XMVECTOR rotatedAxisPosToEyeVec = DirectX::XMVector3Transform(axisPosToEyeVec, rotationMatrix);

	m_eyePosition = DirectX::XMVectorAdd(axisPosVec, rotatedAxisPosToEyeVec);
}

void Camera::UpdateViewMatrix()
{
	m_viewMatrix = DirectX::XMMatrixLookToLH(m_eyePosition, DirectX::XMVectorAdd(m_eyePosition, m_forwardDirection), m_upDirection);
}

void Camera::UpdateProjectionMatrix()
{
	m_projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(m_fovY, m_aspect, m_zNear, m_zFar);
}