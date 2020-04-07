#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace glimac {

	struct ShapeVertex {
	    glm::vec3 position;
	    glm::vec3 normal;
	    glm::vec2 texCoords;
	};

	struct CubeAtom {
		glm::vec3 position;
		GLuint tex_id;
	};

}
