#pragma once

#include "Windows.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Model.h"

static void AssimpSceneProcessing(const aiScene* scene, Model& model);
