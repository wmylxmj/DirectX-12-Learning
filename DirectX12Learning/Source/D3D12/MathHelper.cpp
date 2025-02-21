#include "MathHelper.h"

DirectX::XMMATRIX XM_CALLCONV RotMatrix::Rodrigues(DirectX::FXMVECTOR rotAxis, float rotAngle) {

	// ��ά������֤ w �������� 0 ��
	DirectX::XMVECTOR k = DirectX::XMVectorSetW(rotAxis, 0);
	// һ��Ҫ�ȹ�һ��
	k = DirectX::XMVector3Normalize(rotAxis);

	// �������Һ�����ֵ
	float sinTheta, cosTheta;
	DirectX::XMScalarSinCos(&sinTheta, &cosTheta, rotAngle);
	
	// ���� E * cos(theta)
	DirectX::XMMATRIX matComp1 = DirectX::XMMatrixScaling(cosTheta, cosTheta, cosTheta);

	// ���� (1 - cos(theta)) * k matmul k.T
	DirectX::XMMATRIX matComp2 = (1.0f - cosTheta) * DirectX::XMMatrixVectorTensorProduct(k, k);

	// ���� sin(theta) * k����
	DirectX::XMMATRIX matK;
	matK.r[0] = DirectX::XMVectorSet(0, -DirectX::XMVectorGetZ(k), DirectX::XMVectorGetY(k), 0);
	matK.r[1] = DirectX::XMVectorSet(DirectX::XMVectorGetZ(k), 0, -DirectX::XMVectorGetX(k), 0);
	matK.r[2] = DirectX::XMVectorSet(-DirectX::XMVectorGetY(k), DirectX::XMVectorGetX(k), 0, 0);
	matK.r[3] = DirectX::XMVectorZero();
	DirectX::XMMATRIX matComp3 = sinTheta * matK;

	DirectX::XMMATRIX rotMatrix = matComp1 + matComp2 + matComp3;
	rotMatrix.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

	return rotMatrix;
}