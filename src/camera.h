//module;

#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

//export module camera;
//export
class Camera
{
public:
    //relative movement
    enum Direction {FRONT, LEFT, RIGHT, BACK, UP, DOWN};
    void move(Direction direction, float amount);
    void move(glm::vec3 offset);
    void turn(float pitch, float yaw);

    glm::vec3 position{0.5f, 0.5f, 0.0f};
    //TODO quaternion
    float yaw{0}, pitch{0};
    float fov{glm::radians(45.0f)};
    float aspect{16.0f / 9.0f};
    float near{0.1f};
    float far{100.0f};
    glm::mat4 getViewProjectionMatrix();
    void clampPosition(glm::vec2 range);
    Camera();
private:
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 front;
    glm::vec3 worldUp{0.0f, 1.0f, 0.0};
    void update();
};
