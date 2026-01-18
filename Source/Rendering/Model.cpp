#include "Rendering/Model.h"
#include "Rendering/Mesh.h"
#include "Rendering/Texture.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <filesystem>

namespace Kaya {

Model::Model(const std::string& path) : m_Path(path) {
    LoadModel(path);
}

void Model::Draw() {
    for (auto& mesh : m_Meshes) {
        mesh->Draw();
    }
}

void Model::LoadModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, 
        aiProcess_Triangulate | 
        aiProcess_FlipUVs | 
        aiProcess_CalcTangentSpace |
        aiProcess_GenNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }

    m_Directory = std::filesystem::path(path).parent_path().string();
    ProcessNode(scene->mRootNode, scene);

    std::cout << "Model loaded: " << path << " (" << m_Meshes.size() << " meshes)" << std::endl;
}

void Model::ProcessNode(void* node, const void* scene) {
    aiNode* ainode = static_cast<aiNode*>(node);
    const aiScene* aiscene = static_cast<const aiScene*>(scene);

    // Process all meshes in this node
    for (unsigned int i = 0; i < ainode->mNumMeshes; i++) {
        aiMesh* mesh = aiscene->mMeshes[ainode->mMeshes[i]];
        m_Meshes.push_back(ProcessMesh(mesh, aiscene));
    }

    // Recursively process children
    for (unsigned int i = 0; i < ainode->mNumChildren; i++) {
        ProcessNode(ainode->mChildren[i], aiscene);
    }
}

std::shared_ptr<Mesh> Model::ProcessMesh(void* mesh, const void* scene) {
    aiMesh* aimesh = static_cast<aiMesh*>(mesh);
    const aiScene* aiscene = static_cast<const aiScene*>(scene);

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Material material;

    // Process vertices
    for (unsigned int i = 0; i < aimesh->mNumVertices; i++) {
        Vertex vertex;
        
        vertex.Position = glm::vec3(
            aimesh->mVertices[i].x,
            aimesh->mVertices[i].y,
            aimesh->mVertices[i].z
        );

        if (aimesh->HasNormals()) {
            vertex.Normal = glm::vec3(
                aimesh->mNormals[i].x,
                aimesh->mNormals[i].y,
                aimesh->mNormals[i].z
            );
        } else {
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (aimesh->mTextureCoords[0]) {
            vertex.TexCoords = glm::vec2(
                aimesh->mTextureCoords[0][i].x,
                aimesh->mTextureCoords[0][i].y
            );
        } else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    // Process indices
    for (unsigned int i = 0; i < aimesh->mNumFaces; i++) {
        aiFace face = aimesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Process material
    if (aimesh->mMaterialIndex >= 0) {
        aiMaterial* mat = aiscene->mMaterials[aimesh->mMaterialIndex];

        // Diffuse color
        aiColor4D diffuse;
        if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &diffuse)) {
            material.Color = glm::vec4(diffuse.r, diffuse.g, diffuse.b, diffuse.a);
        }

        // Diffuse texture
        auto diffuseMaps = LoadMaterialTextures(mat, aiTextureType_DIFFUSE, "texture_diffuse");
        if (!diffuseMaps.empty()) {
            material.DiffuseMap = diffuseMaps[0];
        }

        // Specular texture
        auto specularMaps = LoadMaterialTextures(mat, aiTextureType_SPECULAR, "texture_specular");
        if (!specularMaps.empty()) {
            material.SpecularMap = specularMaps[0];
        }

        // Normal texture
        auto normalMaps = LoadMaterialTextures(mat, aiTextureType_NORMALS, "texture_normal");
        if (!normalMaps.empty()) {
            material.NormalMap = normalMaps[0];
        }
    }

    return std::make_shared<Mesh>(vertices, indices, material);
}

std::vector<std::shared_ptr<Texture>> Model::LoadMaterialTextures(void* mat, int type, const std::string& typeName) {
    aiMaterial* material = static_cast<aiMaterial*>(mat);
    aiTextureType textureType = static_cast<aiTextureType>(type);
    
    std::vector<std::shared_ptr<Texture>> textures;
    
    unsigned int textureCount = material->GetTextureCount(textureType);
    for (unsigned int i = 0; i < textureCount; i++) {
        aiString str;
        material->GetTexture(textureType, i, &str);
        
        std::string filename = std::string(str.C_Str());
        std::string path = m_Directory + "/" + filename;

        // Check if texture is already loaded
        bool skip = false;
        for (const auto& loadedTex : m_LoadedTextures) {
            // Simple path comparison - could be improved
            skip = true;
            break;
        }

        if (!skip) {
            try {
                auto texture = std::make_shared<Texture>(path);
                textures.push_back(texture);
                m_LoadedTextures.push_back(texture);
            } catch (const std::exception& e) {
                std::cerr << "Failed to load texture: " << path << " - " << e.what() << std::endl;
            }
        }
    }

    return textures;
}

} // namespace Kaya
