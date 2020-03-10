#include "cameras.hpp"
#include "glfw.hpp"

#include <iostream>

// Good reference here to map camera movements to lookAt calls
// http://learnwebgl.brown37.net/07_cameras/camera_movement.html

using namespace glm;

struct ViewFrame {
    vec3 left;
    vec3 up;
    vec3 front;
    vec3 eye;

    ViewFrame(vec3 l, vec3 u, vec3 f, vec3 e) : left(l), up(u), front(f), eye(e) {}
};

ViewFrame fromViewToWorldMatrix(const mat4 &viewToWorldMatrix) {
    return ViewFrame{-vec3(viewToWorldMatrix[0]), vec3(viewToWorldMatrix[1]), -vec3(viewToWorldMatrix[2]), vec3(viewToWorldMatrix[3])};
}

bool FirstPersonCameraController::update(float elapsedTime) {
    if (glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_LEFT) && !m_LeftButtonPressed) {
        m_LeftButtonPressed = true;
        glfwGetCursorPos(m_pWindow, &m_LastCursorPosition.x, &m_LastCursorPosition.y);
    }
    else if (!glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_LEFT) && m_LeftButtonPressed) {
        m_LeftButtonPressed = false;
    }

    const auto cursorDelta = ([&]() {
        if (m_LeftButtonPressed) {
            dvec2 cursorPosition;
            glfwGetCursorPos(m_pWindow, &cursorPosition.x, &cursorPosition.y);
            const auto delta = cursorPosition - m_LastCursorPosition;
            m_LastCursorPosition = cursorPosition;
            return delta;
        }
        return dvec2(0);
    })();

    float truckLeft = 0.f;
    float pedestalUp = 0.f;
    float dollyIn = 0.f;
    float rollRightAngle = 0.f;


 // add speed up with Ctrl
  if (glfwGetKey(m_pWindow, GLFW_KEY_LEFT_CONTROL)) {
    increaseSpeed(m_fSpeed*4.f);
  } else {
    m_fSpeed = default_speed;
  }



    if (glfwGetKey(m_pWindow, GLFW_KEY_W)) {
        dollyIn += m_fSpeed * elapsedTime;
    }

    // Truck left
    if (glfwGetKey(m_pWindow, GLFW_KEY_A)) {
        truckLeft += m_fSpeed * elapsedTime;
    }

    // Pedestal up
    if (glfwGetKey(m_pWindow, GLFW_KEY_UP)) {
        pedestalUp += m_fSpeed * elapsedTime;
    }

    // Dolly out
    if (glfwGetKey(m_pWindow, GLFW_KEY_S)) {
        dollyIn -= m_fSpeed * elapsedTime;
    }

    // Truck right
    if (glfwGetKey(m_pWindow, GLFW_KEY_D)) {
        truckLeft -= m_fSpeed * elapsedTime;
    }

    // Pedestal down
    if (glfwGetKey(m_pWindow, GLFW_KEY_DOWN)) {
        pedestalUp -= m_fSpeed * elapsedTime;
    }

    if (glfwGetKey(m_pWindow, GLFW_KEY_Q)) {
        rollRightAngle -= 0.001f;
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_E)) {
        rollRightAngle += 0.001f;
    }
    m_fSpeed = default_speed;

    // cursor going right, so minus because we want pan left angle:
    const float panLeftAngle = -0.01f * float(cursorDelta.x);
    const float tiltDownAngle = 0.01f * float(cursorDelta.y);

    const auto hasMoved = truckLeft || pedestalUp || dollyIn || panLeftAngle || tiltDownAngle || rollRightAngle;
    if (!hasMoved) {
        return false;
    }

    m_camera.moveLocal(truckLeft, pedestalUp, dollyIn);
    m_camera.rotateLocal(rollRightAngle, tiltDownAngle, 0.f);
    m_camera.rotateWorld(panLeftAngle, m_worldUpAxis);
    //remetre la vitesse par default

    return true;
}

bool TrackballCameraController::update(float elapsedTime) {
    // Le bouton du milieu au lieu du bouton gauche de la souris
    // De GLFW_MOUSE_BUTTON_LEFT à GLFW_MOUSE_BUTTON_MIDDLE
    if (glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_MIDDLE) && !m_MiddleButtonPressed) {
        m_MiddleButtonPressed = true;
        glfwGetCursorPos(m_pWindow, &m_LastCursorPosition.x, &m_LastCursorPosition.y);
    }
    else if (!glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_MIDDLE) && m_MiddleButtonPressed) {
        m_MiddleButtonPressed = false;
    }

    const auto cursorDelta = ([&]() {
        if (m_MiddleButtonPressed) {
            dvec2 cursorPosition;
            glfwGetCursorPos(m_pWindow, &cursorPosition.x, &cursorPosition.y);
            const auto delta = cursorPosition - m_LastCursorPosition;
            m_LastCursorPosition = cursorPosition;
            return delta;
        }
        return dvec2(0);
    })();

    // Panoramique
    if (glfwGetKey(m_pWindow, GLFW_KEY_LEFT_SHIFT)) {
        const auto truckLeft = 0.01f * float(cursorDelta.x);
        const auto pedestalUp = 0.01f * float(cursorDelta.y);
        const auto hasMoved = truckLeft || pedestalUp;
        // Test des mouvements horizontaux et verticaux
        if (!hasMoved) {
          return false;
        }
        // TODO implement pan
        // Mouvement uniquement sur les axes x et y de la caméra, pas de déplacement sur z
        m_camera.moveLocal(truckLeft, pedestalUp, 0.f);

        return true;
    }

    // Zoom / Dé-zoom
    if (glfwGetKey(m_pWindow, GLFW_KEY_LEFT_CONTROL)) {
        auto mouseOffset = 0.01f * float(cursorDelta.x);
        // Test du mouvement horizontal
        if (mouseOffset == 0.f) {
          return false;
        }
        // TODO implement zoom
        // Déplacement le long du vecteur de vue de la caméra
        const auto viewVector = m_camera.center() - m_camera.eye();
        const auto l = glm::length(viewVector);
        if (mouseOffset > 0.f) {
            // Déplacement pas plus que la longueur du vecteur de vue (ne peut pas dépasser la cible)
            mouseOffset = glm::min(mouseOffset, l - 1e-4f);
        }
        // Normaliser le vecteur de vue pour la traduction
        const auto front = viewVector / l;
        const auto translationVector = mouseOffset * front;

        // Mise à jour de la caméra avec une nouvelle position des yeux
        const auto newEye = m_camera.eye() + translationVector;
        m_camera = Camera(newEye, m_camera.center(), m_worldUpAxis);

        return true;
    }

    // Rotation autour de la cible
    const auto longitudeAngle = 0.01f * float(cursorDelta.y); // Vertical angle
    const auto latitudeAngle = -0.01f * float(cursorDelta.x); // Horizontal angle
    const auto hasMoved = longitudeAngle || latitudeAngle;
    // Test des mouvements horizontaux et verticaux
    if (!hasMoved) {
        return false;
    }
    // TODO implement rotate
    // Pivoter l'œil autour du centre, faire pivoter le vecteur [centre, œil] (= profondeurAxe) afin de calculer une nouvelle position de l'œil
    const auto depthAxis = m_camera.eye() - m_camera.center();

    // Rotation verticale, qui se fait autour de l'axe horizontal de la caméra et peut être obtenue avec left()
    const auto horizontalAxis = m_camera.left();
    const auto longitudeRotationMatrix = rotate(mat4(1), longitudeAngle, horizontalAxis);
    auto rotatedDepthAxis = vec3(longitudeRotationMatrix * vec4(depthAxis, 0));

    // Rotation horizontale, qui se fait autour de m_worldUpAxis
    const auto latitudeRotationMatrix = rotate(mat4(1), latitudeAngle, m_worldUpAxis);
    const auto finalDepthAxis = vec3(latitudeRotationMatrix * vec4(rotatedDepthAxis, 0));

    // Mettre à jour la caméra avec une nouvelle position des yeux
    const auto newEye = m_camera.center() + finalDepthAxis;
    m_camera = Camera(newEye, m_camera.center(), m_worldUpAxis);

    return true;
}
