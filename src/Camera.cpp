#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera()
{
    Position = glm::vec3(0,0,3);
    Front = glm::vec3(0,0,-1);
    Up = glm::vec3(0,1,0);
    Yaw = -90.0f;
    Pitch = 0.0f;
}

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::ProcessKeyboard(int key, float dt)
{
    float speed = 2.5f * dt;

    if (key == 'W') Position += speed * Front;
    if (key == 'S') Position -= speed * Front;
}

void Camera::ProcessMouse(float xoffset, float yoffset)
{
    float sens = 0.1f;
    xoffset *= sens;
    yoffset *= sens;

    Yaw += xoffset;
    Pitch += yoffset;

    if (Pitch > 89.0f) Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;

    glm::vec3 dir;
    dir.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    dir.y = sin(glm::radians(Pitch));
    dir.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

    Front = glm::normalize(dir);
}