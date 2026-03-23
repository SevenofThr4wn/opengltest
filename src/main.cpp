#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include <iostream>

// ================= SETTINGS =================
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool showMenu = true;
bool mouseLocked = true;

// ================= CAMERA =================
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2;
float lastY = SCR_HEIGHT / 2;
bool firstMouse = true;

// ================= TIME =================
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ================= STATE =================
bool wireframe = false;
bool paused = false;

// ================= INPUT =================

/**
 * @brief Processes all keyboard input for camera movement and engine controls.
 *
 * Handles:
 * - WASD movement (forward, backward, strafing)
 * - Shift for speed boost
 * - Wireframe toggle (F key)
 * - Pause toggle (P key)
 * - Mouse unlock (ESC key)
 *
 * Movement is frame-rate independent via deltaTime scaling.
 * Input is ignored when mouse is not locked (UI mode).
 *
 * @param window Pointer to the active GLFW window.
 */
void processInput(GLFWwindow *window)
{
    float speed = 2.5f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;

    static bool fPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fPressed)
    {
        wireframe = !wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        fPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE)
        fPressed = false;

    static bool escPressed = false;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !escPressed)
    {
        mouseLocked = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        escPressed = true;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE)
        escPressed = false;

    static bool pPressed = false;

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pPressed)
    {
        paused = !paused;
        pPressed = true;
    }

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE)
        pPressed = false;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed *= 2.0f;
}

// ================= MOUSE =================

/**
 * @brief Handles mouse movement to control camera orientation.
 *
 * Converts raw cursor movement into yaw/pitch rotation using
 * a sensitivity multiplier. Updates the camera front vector
 * for directional movement and view matrix calculation.
 *
 * Includes:
 * - First-mouse protection to prevent sudden jumps
 * - Pitch clamping to avoid gimbal lock
 *
 * Input is ignored when mouse is unlocked.
 *
 * @param window Pointer to the GLFW window.
 * @param xpos Current X position of the cursor.
 * @param ypos Current Y position of the cursor.
 */
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (!mouseLocked)
        return;
    
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !mouseLocked)
    {
        mouseLocked = true;
        firstMouse = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float sensitivity = 0.1f;
    float xoffset = (xpos - lastX) * sensitivity;
    float yoffset = (lastY - ypos) * sensitivity;

    lastX = xpos;
    lastY = ypos;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

// ================= SHADER =================

/**
 * @brief Compiles and links a basic vertex and fragment shader program.
 *
 * The shader pipeline:
 * - Vertex Shader: Applies model, view, and projection transforms
 * - Fragment Shader: Outputs a constant color
 *
 * No error checking is currently implemented (can be added later).
 *
 * @return OpenGL shader program ID.
 */
unsigned int createShader()
{
    const char *vs = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            gl_Position = projection * view * model * vec4(aPos,1.0);
        }
    )";

    const char *fs = R"(
        #version 330 core
        out vec4 FragColor;

        void main() {
            FragColor = vec4(1.0,0.5,0.2,1.0);
        }
    )";

    unsigned int v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vs, NULL);
    glCompileShader(v);

    unsigned int f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fs, NULL);
    glCompileShader(f);

    unsigned int p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);

    glDeleteShader(v);
    glDeleteShader(f);

    return p;
}

// ================= ImGui Setup =================

/**
 * @brief Initializes and configures Dear ImGui for rendering.
 *
 * Sets up:
 * - ImGui context
 * - Input handling (keyboard/gamepad navigation)
 * - Dark theme styling
 * - GLFW + OpenGL3 backend bindings
 *
 * Must be called after GLFW window creation and context setup.
 *
 * @param window Pointer to the GLFW window.
 */
void setup_imgui(GLFWwindow *window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

// ================= MAIN =================

/**
 * @brief Updates per-frame logic and processes input.
 *
 * Acts as the main update step of the engine loop.
 * Currently handles only input, but can be expanded to include:
 * - Physics updates
 * - Game logic
 * - Scene updates
 *
 * @param window Pointer to the GLFW window.
 */
void update(GLFWwindow *window)
{
    processInput(window);
}

/**
 * @brief Renders the 3D scene
 *
 * Performs:
 * - Screen clearing (color + depth buffer)
 * - Shader activation
 * - Model and view matrix calculation
 * - Uniform updates
 * - Cube draw call
 *
 * Rotation is time-based for smooth animation.
 *
 * @param shader Compiled shader program ID.
 * @param VAO Vertex Array Object containing cube data.
 * @param modelLoc Location of the model matrix uniform.
 * @param viewLoc Location of the view matrix uniform.
 */
void render(unsigned int shader, unsigned int VAO, int modelLoc, int viewLoc)
{
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader);

    float current = glfwGetTime();

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)SCR_WIDTH / SCR_HEIGHT,
        0.1f,
        100.0f);

    int projLoc = glGetUniformLocation(shader, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glm::mat4 model = glm::rotate(
        glm::mat4(1.0f),
        current,
        glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

/**
 * @brief Renders the ImGui debug interface.
 *
 * Displays:
 * - Current FPS
 * - Wireframe toggle checkbox
 *
 * Only called when UI is enabled (showMenu = true).
 * Assumes ImGui frame has already been started.
 */
void renderUI()
{
    float fps = deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f;

    ImGui::Begin("Debug");

    ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
    ImGui::Checkbox("Wireframe", &wireframe);
    ImGui::End();
}

/**
 * @brief Entry point of the application.
 *
 * Responsible for:
 * - Initializing GLFW and OpenGL context
 * - Setting up ImGui
 * - Creating shaders and buffers
 * - Running the main game loop
 * - Handling update/render cycles
 * - Cleaning up resources on exit
 *
 * Main Loop Structure:
 * 1. Calculate delta time
 * 2. Update logic (input, state)
 * 3. Render scene
 * 4. Render UI (optional)
 * 5. Swap buffers and poll events
 *
 * @return 0 on successful execution.
 */
int main()
{
    glfwInit();
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Engine", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    setup_imgui(window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    unsigned int shader = createShader();

    int modelLoc = glGetUniformLocation(shader, "model");
    int viewLoc = glGetUniformLocation(shader, "view");
    int projLoc = glGetUniformLocation(shader, "projection");

    // Cube Vertices
    float vertices[] = {
        -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5,
        -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5,
        -0.5, 0.5, 0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5,
        0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5,
        -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5,
        -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, 0.5, -0.5};

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(window))
    {
        float current = glfwGetTime();
        deltaTime = current - lastFrame;
        lastFrame = current;

        // ===== TERMINATE KEYBIND ====

        if(glfwGetKey(window, GLFW_KEY_F7) == GLFW_PRESS)
            glfwTerminate();

        // ===== UPDATE =====
        update(window);

        // ===== RENDER =====
        render(shader, VAO, modelLoc, viewLoc);

        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
            showMenu = !showMenu;

        if (showMenu)
        {
            static float f = 0.0f;
            static int counter = 0;
        }

        if (showMenu)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            renderUI();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ImGUI Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    ImPlot::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}