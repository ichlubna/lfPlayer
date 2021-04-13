#include "camera.h"

Camera::Camera()
{
    update();
}

void Camera::move(Direction direction, float amount)
{
    switch(direction)
    {
        case FRONT:
            position += front*amount;
        break;
        
        case BACK:
            position -= front*amount;
        break;
        
        case RIGHT:
            position += right*amount;
        break;

        case LEFT:
            position -= right*amount;
        break;
            
        case UP:
            position += glm::cross(right, front)*amount;
        break;
        
        case DOWN:
            position -= glm::cross(right, front)*amount;
        break;
        
        default:
        break;
    }
}
void Camera::turn(float p, float y)
{
    pitch += p;
    yaw += y;
    update();
}

glm::mat4 Camera::getViewProjectionMatrix()
{
    update();
    return glm::perspective(fov,aspect,near,far)*glm::lookAt(position, position+front, up);
}

void Camera::update()
    {
    front.x = cos(yaw) * cos(pitch);
	front.y = sin(pitch);
	front.z = sin(yaw) * cos(pitch);
	front = glm::normalize(front);
	right = glm::normalize(glm::cross(front, worldUp));
	up = glm::normalize(glm::cross(right, front));
}
