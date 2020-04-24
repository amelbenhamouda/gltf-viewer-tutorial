#include "ViewerApplication.hpp"

#include <iostream>
#include <numeric>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

#include "utils/cameras.hpp"
#include "utils/gltf.hpp"
#include "utils/images.hpp"

#include <stb_image_write.h>
#include <tiny_gltf.h>

#include "Cube.hpp"

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
        glfwSetWindowShouldClose(window, 1);
    }
}

bool ViewerApplication::loadGltfFile(tinygltf::Model &model) {  // TODO Loading the glTF file
    std::clog << "Loading file " << m_gltfFilePath << std::endl;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    // m_gltfFilePath.string() au lieu de argv[1]
    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, m_gltfFilePath.string());

    if (!warn.empty()) {
        std::cerr << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    if (!ret) {
        std::cerr << "Failed to parse glTF file" << std::endl;
        return false;
    }

    return true;
}

std::vector<GLuint> ViewerApplication::createBufferObjects(const tinygltf::Model &model) {  // TODO Creation of Buffer Objects
    std::vector<GLuint> bufferObjects(model.buffers.size(), 0);

    glGenBuffers(GLsizei(model.buffers.size()), bufferObjects.data());
    for (size_t i = 0; i < model.buffers.size(); ++i) {
        glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[i]);
        glBufferStorage(GL_ARRAY_BUFFER, model.buffers[i].data.size(), model.buffers[i].data.data(), 0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return bufferObjects;
}

std::vector<GLuint> ViewerApplication::createVertexArrayObjects(const tinygltf::Model &model, const std::vector<GLuint> &bufferObjects, std::vector<VaoRange> &meshToVertexArrays) {   // TODO Creation of Vertex Array Objects
    std::vector<GLuint> vertexArrayObjects; // We don't know the size yet

    // For each mesh of model we keep its range of VAOs
    meshToVertexArrays.resize(model.meshes.size());

    const GLuint VERTEX_ATTRIB_POSITION_IDX = 0;
    const GLuint VERTEX_ATTRIB_NORMAL_IDX = 1;
    const GLuint VERTEX_ATTRIB_TEXCOORD0_IDX = 2;

    for (size_t i = 0; i < model.meshes.size(); ++i) {
        const auto &mesh = model.meshes[i];

        auto &vaoRange = meshToVertexArrays[i];
        vaoRange.begin = GLsizei(vertexArrayObjects.size()); // Range for this mesh will be at
        // the end of vertexArrayObjects
        vaoRange.count = GLsizei(mesh.primitives.size()); // One VAO for each primitive

        // Add enough elements to store our VAOs identifiers
        vertexArrayObjects.resize(vertexArrayObjects.size() + mesh.primitives.size());

        glGenVertexArrays(vaoRange.count, &vertexArrayObjects[vaoRange.begin]);
        for (size_t pIdx = 0; pIdx < mesh.primitives.size(); ++pIdx) {

            const auto vao = vertexArrayObjects[vaoRange.begin + pIdx];
            const auto &primitive = mesh.primitives[pIdx];
            glBindVertexArray(vao);
            {
                // POSITION attribute
                // scope, so we can declare const variable with the same name on each
                // scope
                const auto iterator = primitive.attributes.find("POSITION");
                if (iterator != end(primitive.attributes)) {
                    const auto accessorIdx = (*iterator).second;
                    const auto &accessor = model.accessors[accessorIdx];
                    const auto &bufferView = model.bufferViews[accessor.bufferView];
                    const auto bufferIdx = bufferView.buffer;

                    glEnableVertexAttribArray(VERTEX_ATTRIB_POSITION_IDX);
                    assert(GL_ARRAY_BUFFER == bufferView.target);
                    // Theorically we could also use bufferView.target, but it is safer
                    // Here it is important to know that the next call
                    // (glVertexAttribPointer) use what is currently bound
                    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[bufferIdx]);

                    // tinygltf converts strings type like "VEC3, "VEC2" to the number of
                    // components, stored in accessor.type
                    const auto byteOffset = accessor.byteOffset + bufferView.byteOffset;
                    glVertexAttribPointer(VERTEX_ATTRIB_POSITION_IDX, accessor.type, accessor.componentType, GL_FALSE, GLsizei(bufferView.byteStride), (const GLvoid *)byteOffset);
                }
            }
            // todo Refactor to remove code duplication (loop over "POSITION",
            // "NORMAL" and their corresponding VERTEX_ATTRIB_*)
            {
                // NORMAL attribute
                const auto iterator = primitive.attributes.find("NORMAL");
                if (iterator != end(primitive.attributes)) {
                    const auto accessorIdx = (*iterator).second;
                    const auto &accessor = model.accessors[accessorIdx];
                    const auto &bufferView = model.bufferViews[accessor.bufferView];
                    const auto bufferIdx = bufferView.buffer;

                    glEnableVertexAttribArray(VERTEX_ATTRIB_NORMAL_IDX);
                    assert(GL_ARRAY_BUFFER == bufferView.target);
                    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[bufferIdx]);
                    glVertexAttribPointer(VERTEX_ATTRIB_NORMAL_IDX, accessor.type, accessor.componentType, GL_FALSE, GLsizei(bufferView.byteStride), (const GLvoid *)(accessor.byteOffset + bufferView.byteOffset));
                }
            }
            {
                // TEXCOORD_0 attribute
                const auto iterator = primitive.attributes.find("TEXCOORD_0");
                if (iterator != end(primitive.attributes)) {
                    const auto accessorIdx = (*iterator).second;
                    const auto &accessor = model.accessors[accessorIdx];
                    const auto &bufferView = model.bufferViews[accessor.bufferView];
                    const auto bufferIdx = bufferView.buffer;

                    glEnableVertexAttribArray(VERTEX_ATTRIB_TEXCOORD0_IDX);
                    assert(GL_ARRAY_BUFFER == bufferView.target);
                    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[bufferIdx]);
                    glVertexAttribPointer(VERTEX_ATTRIB_TEXCOORD0_IDX, accessor.type, accessor.componentType, GL_FALSE, GLsizei(bufferView.byteStride), (const GLvoid *)(accessor.byteOffset + bufferView.byteOffset));
                }
            }
            // Index array if defined
            if (primitive.indices >= 0) {
                const auto accessorIdx = primitive.indices;
                const auto &accessor = model.accessors[accessorIdx];
                const auto &bufferView = model.bufferViews[accessor.bufferView];
                const auto bufferIdx = bufferView.buffer;

                assert(GL_ELEMENT_ARRAY_BUFFER == bufferView.target);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObjects[bufferIdx]); // Binding the index buffer to
                // GL_ELEMENT_ARRAY_BUFFER while the VAO
                // is bound is enough to tell OpenGL we
                // want to use that index buffer for that
                // VAO
            }
        }
    }
    glBindVertexArray(0);
    std::clog << "Number of VAOs: " << vertexArrayObjects.size() << std::endl;
    return vertexArrayObjects;
}


std::vector<glm::vec4> computeTangent(const tinygltf::Model &model) {
    int nbpos = 0;
    auto posloc = std::vector<glm::vec3>(3, glm::vec3(0, 0, 0));
    auto texloc = std::vector<glm::vec2>(3, glm::vec2(0, 0));
    std::vector<glm::vec4> alltangent;
    glm::vec4 tangent;

    std::vector<glm::vec4> resTan;
    // Compute Tangente from attributes POSITION and TextCoord_0
    // todo refactor with scene drawing
    // todo need a visitScene generic function that takes a accept() functor
    if (model.defaultScene >= 0) {
        const std::function<void(int, const glm::mat4 &)> computeTan =
            [&](int nodeIdx, const glm::mat4 &parentMatrix)
        {
            const auto &node = model.nodes[nodeIdx];
            const glm::mat4 modelMatrix = getLocalToWorldMatrix(node, parentMatrix);
            if (node.mesh >= 0) {
                const auto &mesh = model.meshes[node.mesh];
                for (size_t pIdx = 0; pIdx < mesh.primitives.size(); ++pIdx)  {
                    const auto &primitive = mesh.primitives[pIdx];
                    /// Attrib Position
                    const auto positionAttrIdxIt = primitive.attributes.find("POSITION");
                    if (positionAttrIdxIt == end(primitive.attributes)) {
                        continue;
                    }
                    const auto &positionAccessor = model.accessors[(*positionAttrIdxIt).second];
                    if (positionAccessor.type != 3) {
                        std::cerr << "Position accessor with type != VEC3, skipping" << std::endl;
                        continue;
                    }
                    const auto &positionBufferView = model.bufferViews[positionAccessor.bufferView];
                    const auto posbyteOffset = positionAccessor.byteOffset + positionBufferView.byteOffset;
                    const auto &positionBuffer = model.buffers[positionBufferView.buffer];
                    const auto positionByteStride = positionBufferView.byteStride ? positionBufferView.byteStride : 3 * sizeof(float);

                    /// Attrib TextCoord_0
                    const auto texAttrIdxIt = primitive.attributes.find("TEXCOORD_0");
                    if (texAttrIdxIt == end(primitive.attributes)) {
                        continue;
                    }
                    const auto &texAccessor = model.accessors[(*texAttrIdxIt).second];
                    if (texAccessor.type != 2) {
                        std::cerr << "Position accessor with type != VEC3, skipping" << std::endl;
                        continue;
                    }
                    const auto &texBufferView = model.bufferViews[texAccessor.bufferView];
                    const auto texbyteOffset = texAccessor.byteOffset + texBufferView.byteOffset;
                    const auto &texBuffer = model.buffers[texBufferView.buffer];
                    const auto texByteStride = texBufferView.byteStride ? texBufferView.byteStride : 2 * sizeof(float);
                    ///

                    if (primitive.indices >= 0) {
                        const auto &indexAccessor = model.accessors[primitive.indices];
                        const auto &indexBufferView = model.bufferViews[indexAccessor.bufferView];
                        const auto indexByteOffset = indexAccessor.byteOffset + indexBufferView.byteOffset;
                        const auto &indexBuffer = model.buffers[indexBufferView.buffer];
                        auto indexByteStride = indexBufferView.byteStride;

                        switch (indexAccessor.componentType) {
	                        default:
	                            std::cerr << "Primitive index accessor with bad componentType " << indexAccessor.componentType << ", skipping it." << std::endl;
	                            continue;
	                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
	                            indexByteStride = indexByteStride ? indexByteStride : sizeof(uint8_t);
	                            break;
	                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
	                            indexByteStride = indexByteStride ? indexByteStride : sizeof(uint16_t);
	                            break;
	                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
	                            indexByteStride = indexByteStride ? indexByteStride : sizeof(uint32_t);
	                            break;
                        }

                        for (size_t i = 0; i < indexAccessor.count; ++i) {
                            uint32_t index = 0;
                            switch (indexAccessor.componentType) {
	                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
	                                index = *((const uint8_t *)&indexBuffer.data[indexByteOffset + indexByteStride * i]);
	                                break;
	                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
	                                index = *((const uint16_t *)&indexBuffer.data[indexByteOffset + indexByteStride * i]);
	                                break;
	                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
	                                index = *((const uint32_t *)&indexBuffer.data[indexByteOffset + indexByteStride * i]);
	                                break;
                            }
                            const auto &localPosition = *((const glm::vec3 *)&positionBuffer.data[posbyteOffset + positionByteStride * index]);
                            const auto worldPosition = glm::vec3(modelMatrix * glm::vec4(localPosition, 1.f));
                            const auto texCoord = *((const glm::vec2 *)&texBuffer.data[texbyteOffset + texByteStride * index]);

                            posloc[2] = posloc[1];
                            posloc[1] = posloc[0];
                            posloc[0] = localPosition;

                            texloc[2] = texloc[1];
                            texloc[1] = texloc[0];
                            texloc[0] = texCoord;
                            if((nbpos + 1) % 3 == 0) {
                                glm::vec3 edge1 = posloc[1] - posloc[0];
                                glm::vec3 edge2 = posloc[2] - posloc[0];
                                glm::vec2 deltaUV1 = texloc[1] - texloc[0];
                                glm::vec2 deltaUV2 = texloc[2] - texloc[0];
                                GLfloat f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
                                tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                                tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                                tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
                                tangent.w = 1.0f;
                                tangent = glm::normalize(tangent);
                                alltangent.push_back(tangent);
                                alltangent.push_back(tangent);
                                alltangent.push_back(tangent);
                            }
                            nbpos += 1;
                        }
                    }
                    else {
                        for (size_t i = 0; i < positionAccessor.count; ++i) {
                            const auto &localPosition = *((const glm::vec3 *)&positionBuffer.data[posbyteOffset + positionByteStride * i]);
                            const auto worldPosition = glm::vec3(modelMatrix * glm::vec4(localPosition, 1.f));
                            const auto texCoord = *((const glm::vec2 *)&texBuffer.data[texbyteOffset + texByteStride * i]);
                            posloc[2] = posloc[1];
                            posloc[1] = posloc[0];
                            posloc[0] = localPosition;

                            texloc[2] = texloc[1];
                            texloc[1] = texloc[0];
                            texloc[0] = texCoord;
                            if((nbpos + 1) % 3 == 0) {
                                glm::vec3 edge1 = posloc[1] - posloc[0];
                                glm::vec3 edge2 = posloc[2] - posloc[0];
                                glm::vec2 deltaUV1 = texloc[1] - texloc[0];
                                glm::vec2 deltaUV2 = texloc[2] - texloc[0];
                                GLfloat f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
                                tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                                tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                                tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
                                tangent.w = 1.0f;
                                tangent = glm::normalize(tangent);
                                alltangent.push_back(tangent);
                                alltangent.push_back(tangent);
                                alltangent.push_back(tangent);
                            }
                            nbpos += 1;
                        }
                    }
                }
            }
            for (const auto childNodeIdx : node.children) {
                computeTan(childNodeIdx, modelMatrix);
            }
        };
        for (const auto nodeIdx : model.scenes[model.defaultScene].nodes) {
            computeTan(nodeIdx, glm::mat4(1));
        }
    }
    resTan = alltangent;
    return resTan;
}

std::vector<GLuint> ViewerApplication::createVertexArrayObjects_T_B(const tinygltf::Model &model, const std::vector<GLuint> &bufferObjects, std::vector<VaoRange> &meshToVertexArrays) {    // TODO Creation of Vertex Array Objects
    std::vector<GLuint> vertexArrayObjects; // We don't know the size yet

    // For each mesh of model we keep its range of VAOs
    meshToVertexArrays.resize(model.meshes.size());

    const GLuint VERTEX_ATTRIB_POSITION_IDX = 0;
    const GLuint VERTEX_ATTRIB_NORMAL_IDX = 1;
    const GLuint VERTEX_ATTRIB_TEXCOORD0_IDX = 2;
    const GLuint VERTEX_ATTRIB_TANGENT = 3;

    //std::vector<glm::vec3> Posloc =  std::vector(2,glm::vec3(0,0,0));

    for (size_t i = 0; i < model.meshes.size(); ++i) {
        const auto &mesh = model.meshes[i];
        auto &vaoRange = meshToVertexArrays[i];
        vaoRange.begin = GLsizei(vertexArrayObjects.size()); // Range for this mesh will be at
        // the end of vertexArrayObjects
        vaoRange.count = GLsizei(mesh.primitives.size()); // One VAO for each primitive

        // Add enough elements to store our VAOs identifiers
        vertexArrayObjects.resize(vertexArrayObjects.size() + mesh.primitives.size());

        glGenVertexArrays(vaoRange.count, &vertexArrayObjects[vaoRange.begin]);
        for (size_t pIdx = 0; pIdx < mesh.primitives.size(); ++pIdx) {
            const auto vao = vertexArrayObjects[vaoRange.begin + pIdx];
            const auto &primitive = mesh.primitives[pIdx];

            glBindVertexArray(vao);
            {
                // POSITION attribute
                // scope, so we can declare const variable with the same name on each
                // scope
                const auto iterator = primitive.attributes.find("POSITION");
                if (iterator != end(primitive.attributes)) {
                    const auto accessorIdx = (*iterator).second;
                    const auto &accessor = model.accessors[accessorIdx];
                    const auto &bufferView = model.bufferViews[accessor.bufferView];
                    const auto bufferIdx = bufferView.buffer;

                    glEnableVertexAttribArray(VERTEX_ATTRIB_POSITION_IDX);
                    assert(GL_ARRAY_BUFFER == bufferView.target);
                    // Theorically we could also use bufferView.target, but it is safer
                    // Here it is important to know that the next call
                    // (glVertexAttribPointer) use what is currently bound
                    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[bufferIdx]);

                    // tinygltf converts strings type like "VEC3, "VEC2" to the number of
                    // components, stored in accessor.type
                    const auto byteOffset = accessor.byteOffset + bufferView.byteOffset;
                    glVertexAttribPointer(VERTEX_ATTRIB_POSITION_IDX, accessor.type, accessor.componentType, GL_FALSE, GLsizei(bufferView.byteStride), (const GLvoid *)byteOffset);
                }
            }
            // todo Refactor to remove code duplication (loop over "POSITION",
            // "NORMAL" and their corresponding VERTEX_ATTRIB_*)
            {
                // NORMAL attribute
                const auto iterator = primitive.attributes.find("NORMAL");
                if (iterator != end(primitive.attributes)) {
                    const auto accessorIdx = (*iterator).second;
                    const auto &accessor = model.accessors[accessorIdx];
                    const auto &bufferView = model.bufferViews[accessor.bufferView];
                    const auto bufferIdx = bufferView.buffer;

                    glEnableVertexAttribArray(VERTEX_ATTRIB_NORMAL_IDX);
                    assert(GL_ARRAY_BUFFER == bufferView.target);
                    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[bufferIdx]);
                    glVertexAttribPointer(VERTEX_ATTRIB_NORMAL_IDX, accessor.type, accessor.componentType, GL_FALSE, GLsizei(bufferView.byteStride), (const GLvoid *)(accessor.byteOffset + bufferView.byteOffset));
                }
            }
            {
                // TEXCOORD_0 attribute
                const auto iterator = primitive.attributes.find("TEXCOORD_0");
                if (iterator != end(primitive.attributes)) {
                    const auto accessorIdx = (*iterator).second;
                    const auto &accessor = model.accessors[accessorIdx];
                    const auto &bufferView = model.bufferViews[accessor.bufferView];
                    const auto bufferIdx = bufferView.buffer;

                    glEnableVertexAttribArray(VERTEX_ATTRIB_TEXCOORD0_IDX);
                    assert(GL_ARRAY_BUFFER == bufferView.target);
                    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[bufferIdx]);
                    glVertexAttribPointer(VERTEX_ATTRIB_TEXCOORD0_IDX, accessor.type, accessor.componentType, GL_FALSE, GLsizei(bufferView.byteStride), (const GLvoid *)(accessor.byteOffset + bufferView.byteOffset));
                }
            }
            {
                // TANGENT attribute
                const auto iterator = primitive.attributes.find("TANGENT");
                if (iterator != end(primitive.attributes)) {
                    /// Attribut TANGENT présent dans le gltf
                    const auto accessorIdx = (*iterator).second;
                    const auto &accessor = model.accessors[accessorIdx];
                    const auto &bufferView = model.bufferViews[accessor.bufferView];
                    const auto bufferIdx = bufferView.buffer;

                    glEnableVertexAttribArray(VERTEX_ATTRIB_TANGENT);
                    assert(GL_ARRAY_BUFFER == bufferView.target);
                    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[bufferIdx]);
                    glVertexAttribPointer(VERTEX_ATTRIB_TANGENT, accessor.type, accessor.componentType, GL_FALSE, GLsizei(bufferView.byteStride), (const GLvoid *)(accessor.byteOffset + bufferView.byteOffset));
                }
                else {
                    /// Attribut TANGENT non présent dans le gltf necessite de le calculé
                    std::vector<glm::vec4> tangente = computeTangent(model);
                    /// bind vbo contenant les tangents
                    GLuint vbo;
                    glGenBuffers(1, &vbo);
                    glBindBuffer(GL_ARRAY_BUFFER, vbo);
                    glBufferData(GL_ARRAY_BUFFER, tangente.size() * sizeof (glm::vec4), tangente.data(), GL_STATIC_DRAW);
                    glBindBuffer(GL_ARRAY_BUFFER, 0);

                    /// passage du vbo de tangente au vao
                    glEnableVertexAttribArray(VERTEX_ATTRIB_TANGENT);
                    glBindBuffer(GL_ARRAY_BUFFER, vbo);
                    glVertexAttribPointer(VERTEX_ATTRIB_TANGENT, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 0);
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                }
            }

            // Index array if defined
            if (primitive.indices >= 0) {
                const auto accessorIdx = primitive.indices;
                const auto &accessor = model.accessors[accessorIdx];
                const auto &bufferView = model.bufferViews[accessor.bufferView];
                const auto bufferIdx = bufferView.buffer;

                assert(GL_ELEMENT_ARRAY_BUFFER == bufferView.target);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObjects[bufferIdx]); // Binding the index buffer to
                // GL_ELEMENT_ARRAY_BUFFER while the VAO
                // is bound is enough to tell OpenGL we
                // want to use that index buffer for that
                // VAO
            }
        }
    }
    glBindVertexArray(0);
    std::clog << "Number of VAOs: " << vertexArrayObjects.size() << std::endl;
    return vertexArrayObjects;
}

std::vector<GLuint> ViewerApplication::createTextureObjects(const tinygltf::Model &model) const {
    std::vector<GLuint> textureObjects(model.textures.size(), 0);

    tinygltf::Sampler defaultSampler;
    defaultSampler.minFilter = GL_LINEAR;
    defaultSampler.magFilter = GL_LINEAR;
    defaultSampler.wrapS = GL_REPEAT;
    defaultSampler.wrapT = GL_REPEAT;
    defaultSampler.wrapR = GL_REPEAT;

    glActiveTexture(GL_TEXTURE0);

    glGenTextures(GLsizei(model.textures.size()), textureObjects.data());
    for (int i = 0; i < model.textures.size(); i++) {
        // Assume a texture object has been created and bound to GL_TEXTURE_2D
        const auto &texture = model.textures[i]; // get i-th texture
        assert(texture.source >= 0); // ensure a source image is present
        const auto &image = model.images[texture.source]; // get the image

        glBindTexture(GL_TEXTURE_2D, textureObjects[i]);
        // fill the texture object with the data from the image
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, image.pixel_type, image.image.data());

        const auto &sampler = texture.sampler >= 0 ? model.samplers[texture.sampler] : defaultSampler;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler.minFilter != -1 ? sampler.minFilter : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler.magFilter != -1 ? sampler.magFilter : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.wrapT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, sampler.wrapR);

        if (sampler.minFilter == GL_NEAREST_MIPMAP_NEAREST || sampler.minFilter == GL_NEAREST_MIPMAP_LINEAR || sampler.minFilter == GL_LINEAR_MIPMAP_NEAREST || sampler.minFilter == GL_LINEAR_MIPMAP_LINEAR) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureObjects;
}

GLuint ViewerApplication::initVbocube(GLsizei count_vertex,const std::vector<glimac::ShapeVertex> &vertices) {
    /// Bind VBO for Cube
    GLuint vbo;
    glGenBuffers(1, &vbo);
    // Binding d'un VBO sur la cible GL_ARRAY_BUFFER:
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof( glimac::ShapeVertex), vertices.data(), GL_STATIC_DRAW); // Envoi des données
    //Après avoir modifié le VBO, on le débind de la cible pour éviter de le remodifier par erreur

    glBindBuffer(GL_ARRAY_BUFFER, 0); // debind
    return vbo;
}

GLuint ViewerApplication::initVaocube(const GLuint &vbo) {
    /// Bind VAO for Cube
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    const GLuint VERTEX_ATTRIB_POSITION_IDX = 0;
    const GLuint VERTEX_ATTRIB_NORMAL_IDX = 1;
    const GLuint VERTEX_ATTRIB_TEXCOORD0_IDX = 2;
    glEnableVertexAttribArray(VERTEX_ATTRIB_POSITION_IDX);
    glEnableVertexAttribArray(VERTEX_ATTRIB_NORMAL_IDX); //1
    glEnableVertexAttribArray(VERTEX_ATTRIB_TEXCOORD0_IDX); //2
    glVertexAttribPointer(VERTEX_ATTRIB_POSITION_IDX, 3, GL_FLOAT, GL_FALSE, sizeof( glimac::ShapeVertex), (const GLvoid*)(offsetof( glimac::ShapeVertex, position)));
    glVertexAttribPointer(VERTEX_ATTRIB_NORMAL_IDX, 3, GL_FLOAT, GL_FALSE, sizeof( glimac::ShapeVertex), (const GLvoid*)(offsetof( glimac::ShapeVertex, normal)));
    glVertexAttribPointer(VERTEX_ATTRIB_TEXCOORD0_IDX, 2, GL_FLOAT, GL_FALSE, sizeof( glimac::ShapeVertex), (const GLvoid*)(offsetof( glimac::ShapeVertex, texCoords)));
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    /// Fin bind Vao Cube
    return vao;
}

void ViewerApplication::setVec3(const GLProgram &prog,const std::string &name,const glm::vec3 &vec) {
    glUniform3f(glGetUniformLocation(prog.glId(), name.c_str()), vec[0], vec[1], vec[2]);
}

void ViewerApplication::setFloat(const GLProgram &prog,const std::string &name, float value) {
    glUniform1f(glGetUniformLocation(prog.glId(), name.c_str()), value);
}

int ViewerApplication::run() {
    // Loader shaders
    const auto glslProgram = compileProgram({ m_ShadersRootPath / m_AppName / m_vertexShader, m_ShadersRootPath / m_AppName / m_fragmentShader });
    const auto modelViewProjMatrixLocation = glGetUniformLocation(glslProgram.glId(), "uModelViewProjMatrix");
    const auto modelViewMatrixLocation = glGetUniformLocation(glslProgram.glId(), "uModelViewMatrix");
    const auto normalMatrixLocation = glGetUniformLocation(glslProgram.glId(), "uNormalMatrix");
    // Récupérer les uniform du fragment shader
    const auto uLightDirectionLocation = glGetUniformLocation(glslProgram.glId(), "uLightDirection");
    const auto uLightIntensity = glGetUniformLocation(glslProgram.glId(), "uLightIntensity");
    const auto uBaseColorTexture = glGetUniformLocation(glslProgram.glId(), "uBaseColorTexture");
    const auto uNormalTexture = glGetUniformLocation(glslProgram.glId(), "uNormalTexture");
    const auto uNormalScale = glGetUniformLocation(glslProgram.glId(), "uNormalScale");
    const auto uActiveNormal = glGetUniformLocation(glslProgram.glId(), "uActiveNormal");
    const auto uBaseColorFactor = glGetUniformLocation(glslProgram.glId(), "uBaseColorFactor");
    const auto uMetallicRoughnessTexture = glGetUniformLocation(glslProgram.glId(), "uMetallicRoughnessTexture");
    const auto uMetallicFactor = glGetUniformLocation(glslProgram.glId(), "uMetallicFactor");
    const auto uRoughnessFactor = glGetUniformLocation(glslProgram.glId(), "uRoughnessFactor");
    const auto uEmissiveTexture = glGetUniformLocation(glslProgram.glId(), "uEmissiveTexture");
    const auto uEmissiveFactor = glGetUniformLocation(glslProgram.glId(), "uEmissiveFactor");

    const auto glslCube = compileProgram({ m_ShadersRootPath / m_AppName / m_vertexShader_cube, m_ShadersRootPath / m_AppName / m_fragmentShader_cube });
    const auto uSize_cube = glGetUniformLocation(glslCube.glId(), "uSize_cube");
    const auto uVMatrix = glGetUniformLocation(glslCube.glId(), "uVMatrix");
    const auto uPosCube = glGetUniformLocation(glslCube.glId(), "uPosCube");
    const auto uPMatrix = glGetUniformLocation(glslCube.glId(), "uPMatrix");
    const auto uColor = glGetUniformLocation(glslCube.glId(), "uColor");

    tinygltf::Model model;
    // TODO Loading the glTF file
    if (!loadGltfFile(model)) {
        return -1;
    }

    ///init Cube
    glimac::Cube cube(1);
    GLsizei count_vertex = cube.getVertexCount();
    const  glimac::ShapeVertex*  Datapointeur = cube.getDataPointer();
    std::vector<glimac::ShapeVertex> vertices;
    for (auto i = 0; i < count_vertex; i++) {  // Cube
        vertices.push_back(*Datapointeur);
        ///rencentre le cube en (0,0,0)
        vertices[i].position[0] -= 0.5;
        vertices[i].position[1] -= 0.5;
        vertices[i].position[1] -= 0.5;
        Datapointeur++;
    }
    GLuint vbocube = initVbocube(count_vertex,vertices);
    GLuint vaocube = initVaocube(vbocube);

    glm::vec3 bboxMin, bboxMax;
    computeSceneBounds(model, bboxMin, bboxMax);
    std::vector <glm::vec3> posCube = {bboxMax, bboxMin, glm::vec3(bboxMax[0], bboxMin[1], bboxMax[2]), glm::vec3(bboxMin[0], bboxMax[1], bboxMax[2])};
    float dist = glm::distance(bboxMax, bboxMin);
    float sizeCube[] = {dist * 0.2f, dist * 0.1f, dist * 0.05f, dist * 0.02f};
    // // Build projection matrix
    const auto diag = bboxMax - bboxMin;
    auto maxDistance = glm::length(diag);
    auto projMatrix = glm::perspective(70.f, float(m_nWindowWidth) / m_nWindowHeight, 0.001f * maxDistance, 1000.0f);

    // TODO Implement a new CameraController model and use it instead. Propose the
    // choice from the GUI
    std::unique_ptr<CameraController> cameraController = std::make_unique<TrackballCameraController>(m_GLFWHandle.window(), 0.5f * maxDistance);
    if (m_hasUserCamera) {
        cameraController->setCamera(m_userCamera);
    }
    else {
        const auto center = 0.5f * (bboxMax + bboxMin);
        const auto up = glm::vec3(0, 1, 0);
        const auto eye = diag.z > 0 ? center + diag : center + 2.f * glm::cross(diag, up);
        // TODO Use scene bounds to compute a better default camera
        cameraController->setCamera(Camera{eye, center, up});
    }

    // Initialisation light parameters
    bool lightFromCamera = false;
    ///directional
    glm::vec3 lightDirection(1, 1, 1);
    glm::vec3 lightIntensity(1, 1, 1);
    glm::vec3 prelightIntensity = lightIntensity;
    ///Ponctual
    const unsigned int NbCube = 4;
    glm::vec3 CubeIntensity[] = {glm::vec3(1, 1, 1), glm::vec3(1, 0, 0), glm::vec3(1, 0.5, 0), glm::vec3(0.5, 0.9, 0.3)};
    glm::vec3 precCubeIntensity[NbCube];
    for(unsigned int i = 0; i<NbCube; i++) {
        precCubeIntensity[i] = CubeIntensity[i];
    }

    std::vector <glm::vec3> CubeColor = {glm::vec3(1, 1, 1), glm::vec3(1, 0, 0), glm::vec3(1, 0.5, 0), glm::vec3(0.5, 0.9, 0.3)};
    std::vector <glm::vec3> preCubeColor = CubeColor;
    float CubeDist[] = {33.f, 21.f, 14.f, 8.f};

    /// Spotlight
    glm::vec3 spotligthIntensity(1, 0.91, 0);
    float spotligthCutOff = 8.5f;
    float spotligthOuterCutOff = 10.5f;
    float spotligthtDistAttenuation = 32;
    bool SpotlightfromCursor = false;
    glm::vec3 precSpotligthIntensity = spotligthIntensity;

    // TODO Creation of Texture Objects
    const auto textureObjects = createTextureObjects(model);
    GLuint whiteTexture = 0;

    // Create white texture for object with no base color texture
    glGenTextures(1, &whiteTexture);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    float white[] = {1, 1, 1, 1};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_FLOAT, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    // TODO Creation of Buffer Objects
    const auto bufferObjects = createBufferObjects(model);

    // TODO Creation of Vertex Array Objects
    std::vector<VaoRange> meshToVertexArrays;
    const auto vertexArrayObjects = createVertexArrayObjects_T_B(model, bufferObjects, meshToVertexArrays);

    ///Normal map
    float ActiveNormalMap = 1;
    bool normaltexturecheck = 1;

    // Setup OpenGL state for rendering
    glEnable(GL_DEPTH_TEST);
    glslProgram.use();

    const auto bindMaterial = [&](const auto materialIndex)
    {
        // Material binding
        if (materialIndex >= 0) {
            // only valid is materialIndex >= 0
            const auto &material = model.materials[materialIndex];
            const auto &pbrMetallicRoughness = material.pbrMetallicRoughness;
            const auto &normalTexture = material.normalTexture;
            //uNormalTexture
            const auto &emissiveTexture = material.emissiveTexture;
            const auto &emissiveFactor = material.emissiveFactor;

            if (uBaseColorTexture >= 0) {
                auto textureObject = whiteTexture;
                if (pbrMetallicRoughness.baseColorTexture.index >= 0) {
                    // only valid if pbrMetallicRoughness.baseColorTexture.index >= 0:
                    const auto &texture = model.textures[pbrMetallicRoughness.baseColorTexture.index];
                    if (texture.source >= 0) {
                        textureObject = textureObjects[texture.source];
                    }
                }
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureObject);
                glUniform1i(uBaseColorTexture, 0);
            }

            ///Normal Texture
            if (uNormalTexture >= 0) {
                auto textureNormal = whiteTexture;
                if (normalTexture.index >= 0) {
                    // only valid if normalTexture..index >= 0:
                    const auto &texture = model.textures[normalTexture.index];
                    if (texture.source >= 0) {
                        textureNormal = textureObjects[texture.source];
                    }
                    glUniform1f(uNormalScale,normalTexture.scale);
                } 
                else {
                    glUniform1f(uActiveNormal,0); // si il n'y a pas de normaltexture spécifié pour le fichier gltf
                    normaltexturecheck = 0;
                }
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, textureNormal);
                glUniform1i(uNormalTexture, 3);
            }

            if (uBaseColorFactor >= 0) {
                glUniform4f(uBaseColorFactor, (float)pbrMetallicRoughness.baseColorFactor[0], (float)pbrMetallicRoughness.baseColorFactor[1], (float)pbrMetallicRoughness.baseColorFactor[2], (float)pbrMetallicRoughness.baseColorFactor[3]);
            }
            if (uMetallicFactor >= 0) {
                glUniform1f(uMetallicFactor, (float)pbrMetallicRoughness.metallicFactor);
            }
            if (uRoughnessFactor >= 0) {
                glUniform1f(uRoughnessFactor, (float)pbrMetallicRoughness.roughnessFactor);
            }
            if (uMetallicRoughnessTexture > 0) {
                auto textureObject = 0;
                if (pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
                    const auto &texture = model.textures[pbrMetallicRoughness.metallicRoughnessTexture.index];
                    if (texture.source >= 0) {
                        textureObject = textureObjects[texture.source];
                    }
                }
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, textureObject);
                glUniform1i(uMetallicRoughnessTexture, 1);
            }

            if (uEmissiveTexture > 0) {
                auto textureObject = 0;
                if (emissiveTexture.index >= 0) {
                    const auto &texture = model.textures[emissiveTexture.index];
                    if (texture.source >= 0) {
                        textureObject = textureObjects[texture.source];
                    }
                }
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, textureObject);
                glUniform1i(uEmissiveTexture, 2);
            }

            if (uEmissiveFactor >= 0) {
                glUniform3f(uEmissiveFactor, (float)emissiveFactor[0], (float)emissiveFactor[1], (float)emissiveFactor[2]);
            }
        }
        else {
            // Apply default material
            if (uBaseColorTexture >= 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, whiteTexture);
                glUniform1i(uBaseColorTexture, 0);
            }
            if (uBaseColorFactor >= 0) {
                glUniform4f(uBaseColorFactor, 1, 1, 1, 1);
            }
            if (uMetallicFactor >= 0) {
                glUniform1f(uMetallicFactor, 1.f);
            }
            if (uRoughnessFactor >= 0) {
                glUniform1f(uRoughnessFactor, 1.f);
            }
            if (uMetallicRoughnessTexture > 0) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, 0);
                glUniform1i(uMetallicRoughnessTexture, 1);
            }
            if (uEmissiveFactor >= 0) {
                glUniform3f(uEmissiveFactor, 1, 1, 1);
            }
            if (uEmissiveTexture > 0) {
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, 0);
                glUniform1i(uEmissiveTexture, 2);
            }
            if(uNormalTexture >=0) {
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, 0);
                glUniform1i(uNormalTexture, 3);
            }
        }
    };

    // Lambda function to draw the scene
    const auto drawScene = [&](const Camera &camera)
    {
        glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const auto viewMatrix = camera.getViewMatrix();
        //Activation ou Non de la normal map
        glUniform1f(uActiveNormal,ActiveNormalMap);
        // Envoie lightIntensity au shader
        if (uLightDirectionLocation >= 0) {
            if (lightFromCamera) {  // Si lumiere camera cocher
                glUniform3f(uLightDirectionLocation, 0, 0, 1);
            }
            else {
                const auto lightDirectionInViewSpace = glm::normalize(glm::vec3(viewMatrix * glm::vec4(lightDirection, 0.)));
                glUniform3f(uLightDirectionLocation, lightDirectionInViewSpace[0], lightDirectionInViewSpace[1], lightDirectionInViewSpace[2]);
            }
        }
        if (uLightIntensity >= 0) {
            glUniform3f(uLightIntensity, lightIntensity[0], lightIntensity[1], lightIntensity[2]);
        }

        ///drawCube
        glslProgram.use();
        for (unsigned int i = 0; i < NbCube; i++) {
            std::string num = std::to_string(i);
            setVec3(glslProgram, ("pointLights[" + num + "].LightPosition").c_str(), glm::vec3(viewMatrix * glm::vec4(posCube[i], 1)));
            setVec3(glslProgram, ("pointLights[" + num + "].CubeIntensity").c_str(), CubeIntensity[i]);
            setFloat(glslProgram, ("pointLights[" + num + "].CubeDist").c_str(), CubeDist[i]);
        }

        auto camPos = glm::vec3(0, 0, 0);
        glm::vec3 spotLigthDirection;
        if (SpotlightfromCursor) {
            double xpos, ypos;
            glfwGetCursorPos(m_GLFWHandle.window(), &xpos, &ypos);
            spotLigthDirection = glm::vec3(float((xpos - m_nWindowWidth / 2) / m_nWindowWidth), float(-(ypos - m_nWindowHeight / 2) / m_nWindowHeight), -1);
        }
        else {
            spotLigthDirection = glm::vec3(0, 0, -1);
        }
        setVec3(glslProgram, "spotligth.LightPosition", camPos);
        setVec3(glslProgram, "spotligth.LightIntensity", spotligthIntensity);
        setVec3(glslProgram, "spotligth.LightDirection", spotLigthDirection);
        setFloat(glslProgram, "spotligth.CutOff", glm::cos(glm::radians(spotligthCutOff)));
        setFloat(glslProgram, "spotligth.OuterCutOff", glm::cos(glm::radians(spotligthOuterCutOff)));
        setFloat(glslProgram, "spotligth.DistAttenuation", spotligthtDistAttenuation);

        glslCube.use();
        glBindVertexArray(vaocube);

        glUniformMatrix4fv(uVMatrix, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(uPMatrix, 1, GL_FALSE, glm::value_ptr(projMatrix));
        for (unsigned int i = 0; i < NbCube; i++) {
            glUniform3fv(uPosCube, 1, glm::value_ptr(posCube[i]));
            glUniform3fv(uColor, 1, glm::value_ptr(CubeColor[i]));
            glUniform1f(uSize_cube,sizeCube[i]);
            glDrawArrays(GL_TRIANGLES, 0, count_vertex);
        }
        glBindVertexArray(0);

        // The recursive function that should draw a node
        // We use a std::function because a simple lambda cannot be recursive
        const std::function<void(int, const glm::mat4 &)> drawNode = [&](int nodeIdx, const glm::mat4 &parentMatrix)
        {
            // TODO The drawNode function
            const auto &node = model.nodes[nodeIdx];
            const glm::mat4 modelMatrix = getLocalToWorldMatrix(node, parentMatrix);

            // If the node references a mesh (a node can also reference a
            // camera, or a light)
            if (node.mesh >= 0) {
                // Also called localToCamera matrix
                const auto mvMatrix = viewMatrix * modelMatrix;
                // Also called localToScreen matrix
                const auto mvpMatrix = projMatrix * mvMatrix;
                // Normal matrix is necessary to maintain normal vectors
                // orthogonal to tangent vectors
                const auto normalMatrix = glm::transpose(glm::inverse(mvMatrix));

                glUniformMatrix4fv(modelViewProjMatrixLocation, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                glUniformMatrix4fv(modelViewMatrixLocation, 1, GL_FALSE, glm::value_ptr(mvMatrix));
                glUniformMatrix4fv(normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMatrix));

                const auto &mesh = model.meshes[node.mesh];
                const auto &vaoRange = meshToVertexArrays[node.mesh];
                for (int i = 0; i < mesh.primitives.size(); ++i) {
                    const auto vao = vertexArrayObjects[vaoRange.begin + i];
                    const auto &primitive = mesh.primitives[i];

                    bindMaterial(primitive.material);

                    glBindVertexArray(vao);

                    if (primitive.indices >= 0) {
                        const auto &accessor = model.accessors[primitive.indices];
                        const auto &bufferView = model.bufferViews[accessor.bufferView];
                        const auto byteOffset = accessor.byteOffset + bufferView.byteOffset;
                        glDrawElements(primitive.mode, GLsizei(accessor.count), accessor.componentType, (const GLvoid *)byteOffset);
                    }
                    else {  // Take first accessor to get the count
                        const auto accessorIdx = (*begin(primitive.attributes)).second;
                        const auto &accessor = model.accessors[accessorIdx];
                        glDrawArrays(primitive.mode, 0, GLsizei(accessor.count));
                    }
                }
            }
            // Draw children
            for (auto nodeChild : node.children) {
                drawNode(nodeChild, modelMatrix);
            }
        };
        // Draw the scene referenced by gltf file
        glslProgram.use();
        if (model.defaultScene >= 0) {
            // TODO Draw all nodes
            for (const auto nodeIdx : model.scenes[model.defaultScene].nodes) {
                drawNode(nodeIdx, glm::mat4(1));
            }
        }
    };

    //TODO Render to image
    if (!(m_OutputPath.empty())) {
        const auto nbComponent = 3;
        std::vector<unsigned char> pixels(m_nWindowWidth * m_nWindowHeight * nbComponent);
        renderToImage(m_nWindowWidth, m_nWindowHeight, nbComponent, pixels.data(), [&]()
        {
            drawScene(cameraController->getCamera());
        });
        flipImageYAxis(m_nWindowWidth, m_nWindowHeight, nbComponent, pixels.data());

        const auto strPath = m_OutputPath.string();
        stbi_write_png(strPath.c_str(), m_nWindowWidth, m_nWindowHeight, 3, pixels.data(), 0);

        return 0;
    }
    int currentcam = 0;

    /// Loop until the user closes the window
    for (auto iterationCount = 0u; !m_GLFWHandle.shouldClose(); ++iterationCount) {
        glfwGetFramebufferSize(m_GLFWHandle.window(), &m_nWindowWidth, &m_nWindowHeight);
        projMatrix = glm::perspective(70.f, float(m_nWindowWidth) / m_nWindowHeight, 0.001f * maxDistance, 1000.0f);

        const auto seconds = glfwGetTime();
        const auto camera = cameraController->getCamera();
        drawScene(camera);

        // GUI code:
        imguiNewFrame();
        {
            ImGui::Begin("GUI");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("eye: %.3f %.3f %.3f", camera.eye().x, camera.eye().y, camera.eye().z);
                ImGui::Text("center: %.3f %.3f %.3f", camera.center().x, camera.center().y, camera.center().z);
                ImGui::Text("up: %.3f %.3f %.3f", camera.up().x, camera.up().y, camera.up().z);
                ImGui::Text("front: %.3f %.3f %.3f", camera.front().x, camera.front().y, camera.front().z);
                ImGui::Text("left: %.3f %.3f %.3f", camera.left().x, camera.left().y, camera.left().z);

                if (ImGui::Button("CLI camera args to clipboard")) {
                    std::stringstream ss;
                    ss << "--lookat " << camera.eye().x << "," << camera.eye().y << ","
                       << camera.eye().z << "," << camera.center().x << ","
                       << camera.center().y << "," << camera.center().z << ","
                       << camera.up().x << "," << camera.up().y << "," << camera.up().z;
                    const auto str = ss.str();
                    glfwSetClipboardString(m_GLFWHandle.window(), str.c_str());
                }
                // Ajout du bouton radio pour choisir le type de caméra
                static int cameraControllerType = 0;
                const auto cameraControllerTypeChanged = ImGui::RadioButton("Trackball", &cameraControllerType, 0) || ImGui::RadioButton("First Person", &cameraControllerType, 1);
                if (cameraControllerTypeChanged) {
                    if (cameraControllerType == 0) {  // Trackball 
                        cameraController = std::make_unique<TrackballCameraController>(m_GLFWHandle.window(), 0.5f * maxDistance);
                        const auto center = 0.5f * (bboxMax + bboxMin);
                        const auto up = glm::vec3(0, 1, 0);
                        const auto eye = diag.z > 0 ? center + diag : center + 2.f * glm::cross(diag, up);
                        // TODO Use scene bounds to compute a better default camera
                        cameraController->setCamera(Camera{eye, center, up});
                        currentcam = 0;
                    }
                    else {  // First Person
                        const auto currentCamera = cameraController->getCamera();
                        cameraController = std::make_unique<FirstPersonCameraController>(m_GLFWHandle.window(), 0.5f * maxDistance);
                        cameraController->setCamera(currentCamera);
                        currentcam = 1;
                    }
                }
            }
            if (currentcam == 0) {
                ImGui::Text("Current cam : Trackball");
            }
            else if (currentcam == 1) {
                ImGui::Text("Current cam : FPS");
            }
            
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                static float lightTheta = 0.f;
                static float lightPhi = 0.f;
                ImGui::TextColored(ImVec4(1,1,0,1), "Directional Ligth");
                if (ImGui::SliderFloat("theta", &lightTheta, 0, glm::pi<float>()) || ImGui::SliderFloat("phi", &lightPhi, 0, 2.f * glm::pi<float>())) {
                    const auto sinPhi = glm::sin(lightPhi);
                    const auto cosPhi = glm::cos(lightPhi);
                    const auto sinTheta = glm::sin(lightTheta);
                    const auto cosTheta = glm::cos(lightTheta);
                    lightDirection = glm::vec3(sinTheta * cosPhi, cosTheta, sinTheta * sinPhi);
                }

                static glm::vec3 lightColor(1, 1, 1);
                static std::vector<glm::vec3> CubeNewColor= CubeColor;
                static std::vector<glm::vec3>  CubePose = posCube;
                static float lightIntensityFactor;
                static std::vector<float> LigthCubeIntensity(NbCube, 1.f);
                static float maxIntensity = 100.0f;
                static float cubeposefactor = 20;

                if (ImGui::ColorEdit3("Color Directional ligth", (float *)&lightColor)) {
                    lightIntensity = lightColor * lightIntensityFactor;
                }
                if (ImGui::SliderFloat("Intensity", &lightIntensityFactor,0, maxIntensity)) {
                    lightIntensity = lightColor * lightIntensityFactor;
                    prelightIntensity = lightIntensity;
                }
                // Ajout d'une boîte à cocher
                ImGui::Checkbox("Light from camera", &lightFromCamera);
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Cube");
                static int cubetochange = 0;
                ImGui::TextColored(ImVec4(1, 1, 1, 1), "Choose Cube : ");
                for (int i = 0; i < NbCube; i++) {
                    std::string s = std::to_string(i+1);
                    char strcube[10] = "cube n°";
                    //strcat_s(strcube, sizeof strcube, s.c_str());
                    strcat(strcube, s.c_str());
                    ImGui::RadioButton(strcube, &cubetochange, i);
                }

                if (ImGui::ColorEdit3("Color cube", (float *)&CubeNewColor[cubetochange])) {
                    CubeIntensity[cubetochange] = CubeNewColor[cubetochange] * LigthCubeIntensity[cubetochange];
                    CubeColor[cubetochange] = CubeNewColor[cubetochange] * glm::vec3(LigthCubeIntensity[cubetochange] / (maxIntensity * 0.5f));
                    precCubeIntensity[cubetochange] = CubeIntensity[cubetochange];
                    preCubeColor[cubetochange] = CubeColor[cubetochange];
                }
                if (ImGui::SliderFloat("Cube intensity", &LigthCubeIntensity[cubetochange], 0, maxIntensity)) {
                    CubeIntensity[cubetochange] = CubeNewColor[cubetochange] * LigthCubeIntensity[cubetochange];
                    CubeColor[cubetochange] = CubeNewColor[cubetochange] * glm::vec3(LigthCubeIntensity[cubetochange] / (maxIntensity * 0.2f));
                    precCubeIntensity[cubetochange] = CubeIntensity[cubetochange];
                    preCubeColor[cubetochange] = CubeColor[cubetochange];
                }
                if (ImGui::SliderFloat("X_pos", &CubePose[cubetochange][0], -cubeposefactor*bboxMax[0], cubeposefactor*bboxMax[0]) || ImGui::SliderFloat("Y_pos", &CubePose[cubetochange][1], -cubeposefactor*bboxMax[1], cubeposefactor*bboxMax[1]) || ImGui::SliderFloat("Z_pos", &CubePose[cubetochange][2], -cubeposefactor*bboxMax[2], cubeposefactor*bboxMax[2])) {
                    posCube[cubetochange] = CubePose[cubetochange];
                }

                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Spotligth");

                static float NewspotligthCutOff = spotligthCutOff;
                static float NewspotligthOuterCutOff = spotligthOuterCutOff;
                static float BothCutoffandOuter = spotligthCutOff;
                static float NewspotligthtDistAttenuation = spotligthtDistAttenuation;
                static glm::vec3 spotlightColor = spotligthIntensity;
                static float SpotlightIntensityFactor;
                if (ImGui::ColorEdit3("Color SpotLight", (float *)&spotlightColor) || ImGui::SliderFloat("Intensity spotligth", &SpotlightIntensityFactor, 0, maxIntensity)) {
                    spotligthIntensity = spotlightColor * SpotlightIntensityFactor;
                    precSpotligthIntensity = spotligthIntensity;
                }
                if (ImGui::SliderFloat("Dist CuteOff", &NewspotligthCutOff, 0.f, 180.f)) {
                    spotligthCutOff = NewspotligthCutOff;
                }
                if (ImGui::SliderFloat("Dist OuterCuteOff", &NewspotligthOuterCutOff, 0.f, 180.f)) {
                    spotligthOuterCutOff = NewspotligthOuterCutOff;
                }
                if (ImGui::SliderFloat("Both CuteOff & Outer", &BothCutoffandOuter, 0.f, 180.f)) {
                    spotligthCutOff = BothCutoffandOuter;
                    spotligthOuterCutOff = BothCutoffandOuter * 1.1f;
                    NewspotligthCutOff = spotligthCutOff;
                    NewspotligthOuterCutOff = spotligthOuterCutOff;
                }
                if (ImGui::SliderFloat("Dist attenuation spotligth", &NewspotligthtDistAttenuation, 0, 150)) {
                    spotligthtDistAttenuation = NewspotligthtDistAttenuation;
                }

                if (ImGui::Button("Spot light from Cursor / centered Spot light")) {
                    SpotlightfromCursor = !SpotlightfromCursor;
                }

                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Switch Off/On all ligth: ");

                ImGui::SameLine();
                auto buttonOff = ImGui::Button("Off");
                ImGui::SameLine();
                auto buttonOn = ImGui::Button("On");

                if (buttonOff) {
                    glm::vec3 off(0, 0, 0);
                    precSpotligthIntensity = spotligthIntensity;
                    spotligthIntensity = off;
                    for (auto i = 0; i < NbCube; i++) {
                        precCubeIntensity[i] = CubeIntensity[i];
                        preCubeColor[i] = CubeColor[i];
                        CubeIntensity[i] = off;
                        CubeColor[i] = off;
                    }
                    prelightIntensity = lightIntensity;
                    lightIntensity = off;
                }
                else if (buttonOn) {
                    for (auto i = 0; i < NbCube; i++) {
                        CubeIntensity[i] = precCubeIntensity[i];
                        CubeColor[i] = preCubeColor[i];
                    }
                    spotligthIntensity = precSpotligthIntensity;
                    lightIntensity = prelightIntensity;
                }
            }

            if (ImGui::CollapsingHeader("Normal Map", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (normaltexturecheck == 1) {
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Switch Off/On Normal map : ");
                    ImGui::SameLine();
                    auto NormalOff = ImGui::Button("_Off_");
                    ImGui::SameLine();
                    auto NormalOn = ImGui::Button("_On_");
                    if (NormalOff) {
                        ActiveNormalMap = 0.f;
                    }
                    else if (NormalOn) {
                        ActiveNormalMap = 1.f;
                    }
                } 
                else {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "No normalTexture in the gltf file ");
                }
            }
            ImGui::End();
        }
        imguiRenderFrame();
        glfwPollEvents(); // Poll for and process events
        auto ellapsedTime = glfwGetTime() - seconds;
        auto guiHasFocus = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
        if (!guiHasFocus) {
            cameraController->update(float(ellapsedTime));
        }
        m_GLFWHandle.swapBuffers(); // Swap front and back buffers
    }
    // TODO clean up allocated GL data
    glDeleteBuffers(1, &vbocube);
    glDeleteVertexArrays(1, &vaocube);
    for (auto &it : textureObjects) {
        glDeleteTextures(1, &it);
    }
    for (auto &it : bufferObjects) {
        glDeleteBuffers(1, &it);
    }
    for (auto &it : vertexArrayObjects) {
        glDeleteVertexArrays(1, &it);
    }
    return 0;
}

ViewerApplication::ViewerApplication(const fs::path &appPath, uint32_t width, uint32_t height, const fs::path &gltfFile, const std::vector<float> &lookatArgs, const std::string &vertexShader, const std::string &fragmentShader, const fs::path &output) 
		: m_nWindowWidth(width), m_nWindowHeight(height), m_AppPath{appPath}, m_AppName{m_AppPath.stem().string()}, m_ImGuiIniFilename{m_AppName + ".imgui.ini"}, m_ShadersRootPath{m_AppPath.parent_path() / "shaders"}, m_gltfFilePath{gltfFile}, m_OutputPath{output} {
    if (!lookatArgs.empty()) {
        m_hasUserCamera = true;
        m_userCamera = Camera { glm::vec3(lookatArgs[0], lookatArgs[1], lookatArgs[2]), glm::vec3(lookatArgs[3], lookatArgs[4], lookatArgs[5]), glm::vec3(lookatArgs[6], lookatArgs[7], lookatArgs[8])};
    }

    if (!vertexShader.empty()) {
        m_vertexShader = vertexShader;
    }

    if (!fragmentShader.empty()) {
        m_fragmentShader = fragmentShader;
    }

    ImGui::GetIO().IniFilename = m_ImGuiIniFilename.c_str(); // At exit, ImGUI will store its windows
    // positions in this file
    glfwSetKeyCallback(m_GLFWHandle.window(), keyCallback);
    printGLVersion();
}
