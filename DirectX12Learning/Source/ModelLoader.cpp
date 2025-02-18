#include "ModelLoader.h"
#include <codecvt>

bool AssimpImportModel(const std::string& pFile, Model& model) {

	Assimp::Importer importer;

	unsigned int pFlags = aiProcess_Triangulate |
						 aiProcess_JoinIdenticalVertices;

	const aiScene* scene = importer.ReadFile(pFile, pFlags);

	if (scene == nullptr) {
		MessageBox(0, L"�޷�����ģ���ļ�", L"ģ���ļ�����ʧ��", 0);
		return false;
	}

	return AssimpSceneProcessing(scene, model);
}

bool AssimpSceneProcessing(const aiScene* scene, Model& model) {
	return true;
}
	