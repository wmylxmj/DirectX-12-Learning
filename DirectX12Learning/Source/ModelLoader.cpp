#include "ModelLoader.h"
#include <codecvt>

bool AssimpImportModel(const std::string& pFile, Model& model) {

	Assimp::Importer importer;

	unsigned int pFlags = aiProcess_Triangulate |
						  aiProcess_JoinIdenticalVertices |
	                      aiProcess_GenNormals;

	const aiScene* scene = importer.ReadFile(pFile, pFlags);
	
	// �������ʧ�ܣ����������Ϣ
	if (scene == nullptr) {
		std::string importError = importer.GetErrorString();
		MessageBoxA(0, importError.c_str(), "Assimp Import Model Failed", 0);
		return false;
	}

	// �����ļ�����
	AssimpSceneProcessing(scene, model);

	return true;
}

void AssimpSceneProcessing(const aiScene* scene, Model& model) {

}
	