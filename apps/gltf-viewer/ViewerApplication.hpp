#pragma once

#include "utils/GLFWHandle.hpp"
#include "utils/cameras.hpp"
#include "utils/filesystem.hpp"
#include "utils/shaders.hpp"
#include "Cube.hpp"
#include <tiny_gltf.h> // TODO Loading the glTF file

class ViewerApplication {
    public:
        ViewerApplication(const fs::path &appPath, uint32_t width, uint32_t height, const fs::path &gltfFile, const std::vector<float> &lookatArgs,
                          const std::string &vertexShader, const std::string &fragmentShader, const fs::path &output);

        int run();

    private:
        // A range of indices in a vector containing Vertex Array Objects
        struct VaoRange {
            GLsizei begin; // Index of first element in vertexArrayObjects
            GLsizei count; // Number of elements in range
        };

        bool loadGltfFile(tinygltf::Model &model);
        std::vector<GLuint> createBufferObjects(const tinygltf::Model &model);
        std::vector<GLuint> createVertexArrayObjects(const tinygltf::Model &model, const std::vector<GLuint> &bufferObjects, std::vector<VaoRange> &meshToVertexArrays);
        std::vector<GLuint> createVertexArrayObjects_T_B(const tinygltf::Model &model, const std::vector<GLuint> &bufferObjects, std::vector<VaoRange> &meshToVertexArrays);
        std::vector<GLuint> createTextureObjects(const tinygltf::Model &model) const;
        GLuint initVbocube(GLsizei count_vertex,const std::vector<glimac::ShapeVertex> &vertices);
        GLuint initVaocube(const GLuint &vbo);
        void setVec3(const GLProgram &prog,const std::string &name,const glm::vec3 &vec);
        void setFloat(const GLProgram &prog,const std::string &name, float value);

        GLsizei m_nWindowWidth = 1280;
        GLsizei m_nWindowHeight = 720;

        const fs::path m_AppPath;
        const std::string m_AppName;
        const fs::path m_ShadersRootPath;

        fs::path m_gltfFilePath;
        std::string m_vertexShader = "forward.vs.glsl";
        std::string m_vertexShader_cube = "shad3Dcube.vs.glsl";
        std::string m_fragmentShader = "pbr_directional_light.fs.glsl";
        std::string m_fragmentShader_cube = "shad3Dcube.fs.glsl";

        bool m_hasUserCamera = false;
        Camera m_userCamera;

        fs::path m_OutputPath;

        // Order is important here, see comment below
        const std::string m_ImGuiIniFilename;
        // Last to be initialized, first to be destroyed:
        GLFWHandle m_GLFWHandle { int(m_nWindowWidth), int(m_nWindowHeight), "glTF Viewer", m_OutputPath.empty() }; // show the window only if m_OutputPath is empty
        /*
        ! THE ORDER OF DECLARATION OF MEMBER VARIABLES IS IMPORTANT !
        - m_ImGuiIniFilename.c_str() will be used by ImGUI in ImGui::Shutdown, which
        will be called in destructor of m_GLFWHandle. So we must declare
        m_ImGuiIniFilename before m_GLFWHandle so that m_ImGuiIniFilename
        destructor is called after.
        - m_GLFWHandle must be declared before the creation of any object managing
        OpenGL resources (e.g. GLProgram, GLShader) because it is responsible for
        the creation of a GLFW windows and thus a GL context which must exists
        before most of OpenGL function calls.
        */
};
