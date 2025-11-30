#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"
#include "shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

class Model 
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // draws the model, and thus all its meshes
    void Draw(Shader &shader)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }
    
private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        // process each mesh located at the current node
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        // walk through each of the mesh's vertices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x; 
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);        
        }
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];    

        // 1. base color / diffuse
        vector<Texture> baseColorMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_diffuse", scene);
        if (baseColorMaps.empty()) {
            vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
            baseColorMaps.insert(baseColorMaps.end(), diffuseMaps.begin(), diffuseMaps.end());
        }
        textures.insert(textures.end(), baseColorMaps.begin(), baseColorMaps.end());

        // 2. specular (legacy)
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", scene);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        // 3. normals
        vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", scene);
        if (normalMaps.empty()) {
            vector<Texture> altNormals = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", scene);
            normalMaps.insert(normalMaps.end(), altNormals.begin(), altNormals.end());
        }
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        // 4. emissive
        vector<Texture> emissiveMaps = loadMaterialTextures(material, aiTextureType_EMISSIVE, "texture_emissive", scene);
        textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());

        // 5. metallic / roughness (optional)
        vector<Texture> metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, "texture_metallic", scene);
        vector<Texture> roughnessMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness", scene);
        textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());
        textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());
        
        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures);
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName, const aiScene* scene)
    {
        vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            const char* cpath = str.C_Str();

            // de-dup
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), cpath) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            if (skip) continue;

            Texture texture;
            texture.type = typeName;
            texture.path = cpath;

            // embedded texture in GLB: paths like "*0", "*1", ...
            if (cpath[0] == '*')
            {
                const aiTexture* emb = scene->GetEmbeddedTexture(cpath);
                texture.id = TextureFromEmbedded(emb);
            }
            else
            {
                texture.id = TextureFromFile(cpath, this->directory);
            }

            textures.push_back(texture);
            textures_loaded.push_back(texture);
        }
        return textures;
    }

    static unsigned int MakeGLTextureRGBA8(const unsigned char* rgba, int w, int h) {
        unsigned int tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        return tex;
    }

    static unsigned int TextureFromEmbedded(const aiTexture* tex)
    {
        if (!tex) return 0;

        if (tex->mHeight == 0) {
            // compressed (png/jpg/etc.), mWidth is data size in bytes
            int w = 0, h = 0, n = 0;
            stbi_set_flip_vertically_on_load(false);
            const unsigned char* mem = reinterpret_cast<const unsigned char*>(tex->pcData);
            unsigned char* data = stbi_load_from_memory(mem, tex->mWidth, &w, &h, &n, STBI_rgb_alpha);
            if (!data) {
                std::cerr << "stbi_load_from_memory failed for embedded texture.\n";
                return 0;
            }
            unsigned int out = MakeGLTextureRGBA8(data, w, h);
            stbi_image_free(data);
            return out;
        }
        else {
            // uncompressed BGRA8 in aiTexel
            const int w = tex->mWidth;
            const int h = tex->mHeight;
            std::vector<unsigned char> rgba(w * h * 4);
            for (int i = 0; i < w * h; ++i) {
                const aiTexel& t = tex->pcData[i];
                rgba[4 * i + 0] = t.r;
                rgba[4 * i + 1] = t.g;
                rgba[4 * i + 2] = t.b;
                rgba[4 * i + 3] = t.a;
            }
            return MakeGLTextureRGBA8(rgba.data(), w, h);
        }
    }
};


unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma)
{
    std::string filename = directory + '/' + std::string(path);

    unsigned int textureID = 0;
    glGenTextures(1, &textureID);

    int width = 0, height = 0, nrComponents = 0;
    stbi_set_flip_vertically_on_load(false); // glTF 不需要翻转

    // 不强制转换为 RGBA，允许获取真实通道数
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (!data)
    {
        std::cerr << "Texture failed to load: " << filename
                  << " (" << stbi_failure_reason() << ")\n";
        // fallback 1x1 粉色
        unsigned char fallback[4] = { 255, 0, 255, 255 };
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, fallback);
    }
    else
    {
        GLenum format = GL_RGB; // 默认值，避免未初始化
        GLenum internalFormat = GL_RGB;
        if (nrComponents == 1)
        {
            format = GL_RED;
            internalFormat = GL_RED;
        }
        else if (nrComponents == 2)
        {
            // metallicRoughness 等两通道贴图
            format = GL_RG;
            internalFormat = GL_RG;
        }
        else if (nrComponents == 3)
        {
            format = GL_RGB;
            internalFormat = gamma ? GL_SRGB : GL_RGB;
        }
        else if (nrComponents == 4)
        {
            format = GL_RGBA;
            internalFormat = gamma ? GL_SRGB_ALPHA : GL_RGBA;
        }
        else
        {
            std::cout << "Warning: texture " << filename
                      << " has unexpected channel count: " << nrComponents
                      << ", defaulting to GL_RGB" << std::endl;
            format = GL_RGB;
            internalFormat = GL_RGB;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}

static unsigned int MakeGLTextureRGBA8(const unsigned char* rgba, int w, int h) {
    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // avoid row alignment issues
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

static unsigned int TextureFromEmbedded(const aiTexture* tex)
{
    if (!tex) return 0;

    if (tex->mHeight == 0) {
        // compressed (png/jpg/etc.), mWidth is data size in bytes
        int w = 0, h = 0, n = 0;
        stbi_set_flip_vertically_on_load(false); // glTF/GLB usually does not need flipping
        const unsigned char* mem = reinterpret_cast<const unsigned char*>(tex->pcData);
        unsigned char* data = stbi_load_from_memory(mem, tex->mWidth, &w, &h, &n, STBI_rgb_alpha);
        if (!data) {
            std::cerr << "stbi_load_from_memory failed for embedded texture.\n";
            return 0;
        }
        unsigned int out = MakeGLTextureRGBA8(data, w, h);
        stbi_image_free(data);
        return out;
    }
    else {
        // uncompressed BGRA8 in aiTexel
        const int w = tex->mWidth;
        const int h = tex->mHeight;
        std::vector<unsigned char> rgba(w * h * 4);
        for (int i = 0; i < w * h; ++i) {
            const aiTexel& t = tex->pcData[i];
            rgba[4 * i + 0] = t.r;
            rgba[4 * i + 1] = t.g;
            rgba[4 * i + 2] = t.b;
            rgba[4 * i + 3] = t.a;
        }
        return MakeGLTextureRGBA8(rgba.data(), w, h);
    }
}

#endif
