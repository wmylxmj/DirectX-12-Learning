#include "ModelLoader.h"
#include <codecvt>

bool AssimpImportModel(const std::string& pFile, Model& model) {

	Assimp::Importer importer;

	unsigned int pFlags = aiProcess_Triangulate |
						  aiProcess_JoinIdenticalVertices |
	                      aiProcess_GenNormals;

	const aiScene* scene = importer.ReadFile(pFile, pFlags);
	
	// 如果导入失败，报告错误信息
	if (scene == nullptr) {
		std::string importError = importer.GetErrorString();
		MessageBoxA(0, importError.c_str(), "Assimp Import Model Failed", 0);
		return false;
	}

	// 处理文件内容
	AssimpSceneProcessing(scene, model);

	return true;
}

void AssimpSceneProcessing(const aiScene* scene, Model& model) {

}
	