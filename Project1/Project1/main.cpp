#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtc/type_ptr.hpp>

#include "shader_m.h"
#include "camera.h"
#include "model_animation.h"
#include "animdata.h"

#include <map>
#include <cmath>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int setupGroundPlane();

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// camera
Camera camera(glm::vec3(0.0f, 5.0f, 30.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

static inline int boneIdOf(const std::map<std::string, BoneInfo>& m, const char* n)
{
    auto it = m.find(n);
    return (it == m.end() ? -1 : it->second.id);
}

void updateArmHierarchy(
    const std::map<std::string, BoneInfo>& boneInfoMap,
    std::vector<glm::mat4>& boneMatrices,
    float /*t*/)
{
    size_t count = boneMatrices.size();
    for (size_t i = 0; i < count; ++i) {
        boneMatrices[i] = glm::mat4(1.0f);
    }
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Enable multisampling for better transparency quality
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    // enable alpha blending for hair and other transparent materials
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Enable multisampling for better transparency quality
    glEnable(GL_MULTISAMPLE);
    
    // Enable alpha-to-coverage for better hair and foliage rendering
    // This works best with MSAA enabled
    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    // build and compile shaders
    // -------------------------
    Shader ourShader("1.model_loading.vs", "1.model_loading.fs");
    Shader animShader("anim_model.vs", "anim_model.fs");
    Shader groundShader("ground.vs", "ground.fs");
    Shader neonShader("neon_billboard.vs", "neon_billboard.fs"); // 新增：专用霓虹 shader

    // load models
    // -----------
    Model noodleModel("D:/TCD/Computer-Graphics/Project1/resources/models/cyberpunk_noodle/scene.gltf");
    Model transboxModel("D:/TCD/Computer-Graphics/Project1/resources/models/transformer_box/scene.gltf");
    Model VMModel("D:/TCD/Computer-Graphics/Project1/resources/models/ramen-vending-machine-1/scene.gltf");
    Model DJModel("D:/TCD/Computer-Graphics/Project1/resources/models/dji_fpv/scene.gltf");
    

    std::cout << "\n=== Loading Character Model ===" << std::endl;
    Model girlModel("D:/TCD/Computer-Graphics/Project1/resources/models/girl/character.gltf");

    auto& boneInfoMap = girlModel.GetBoneInfoMap();
    int boneCount = girlModel.GetBoneCount();

    std::cout << "Total bones: " << boneCount << std::endl;
    std::cout << "Available bones for animation:" << std::endl;
    for (const auto& bone : boneInfoMap) {
        std::cout << "  - " << bone.first << " (ID: " << bone.second.id << ")" << std::endl;
    }

    // Setup ground plane
    unsigned int groundVAO = setupGroundPlane();

    const int MAX_BONES = 100;
    std::vector<glm::mat4> boneMatrices(std::max(boneCount, MAX_BONES), glm::mat4(1.0f));

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // Render ground
        groundShader.use();
        groundShader.setMat4("projection", projection);
        groundShader.setMat4("view", view);
        glm::mat4 groundModel = glm::mat4(1.0f);
        groundModel = glm::translate(groundModel, glm::vec3(0.0f, -10.0f, 0.0f));
        groundShader.setMat4("model", groundModel);
        groundShader.setVec3("groundColor", glm::vec3(0.4f, 0.3f, 0.2f));
        // Convert light position to direction for the ground shader
        glm::vec3 lightPos = glm::vec3(10.0f, 20.0f, 10.0f);
        glm::vec3 lightDir = normalize(-lightPos); // Directional light pointing toward origin
        groundShader.setVec3("lightDir", lightDir);

        glBindVertexArray(groundVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Render static models
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setFloat("uTime", currentFrame);
        
        // Set lighting uniforms for static models
        ourShader.setVec3("lightPos", glm::vec3(10.0f, 20.0f, 10.0f));
        ourShader.setVec3("lightColor", glm::vec3(1.2f, 1.2f, 1.2f));
        ourShader.setVec3("viewPos", camera.Position);
        ourShader.setFloat("ambientStrength", 0.35f);
        ourShader.setFloat("specularStrength", 0.8f);
        ourShader.setInt("shininess", 32);

        // Model 2: Transformer Box
        glm::mat4 model2 = glm::mat4(1.0f);
        model2 = glm::translate(model2, glm::vec3(20.0f, -10.0f, -10.0f));
        model2 = glm::rotate(model2, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model2 = glm::scale(model2, glm::vec3(5.0f, 5.0f, 5.0f));
        ourShader.setMat4("model", model2);
        // 禁用发光动画
        ourShader.setInt("isEmissive", 0);
        ourShader.setFloat("emissiveIntensity", 0.0f);
        transboxModel.Draw(ourShader);

        // Model 4: Noodle shop 使用普通模型着色器+发光增强，不再完全替换
        glm::mat4 model4 = glm::mat4(1.0f);
        model4 = glm::translate(model4, glm::vec3(-20.0f, -10.0f, -10.0f));
        model4 = glm::scale(model4, glm::vec3(0.08f, 0.08f, 0.08f));
        ourShader.setMat4("model", model4);
        ourShader.setInt("isEmissive", 1);
        ourShader.setVec3("emissiveColor", glm::vec3(1.0f, 0.95f, 1.0f));
        ourShader.setFloat("emissiveIntensity", 0.0f);
        noodleModel.Draw(ourShader);

        // New Model: Ramen Vending Machine (placed to the right of transformer box)
        glm::mat4 modelVM = glm::mat4(1.0f);
        modelVM = glm::translate(modelVM, glm::vec3(45.0f, -10.0f, -10.0f));
        modelVM = glm::scale(modelVM, glm::vec3(10.0f, 10.0f, 10.0f));
        ourShader.setMat4("model", modelVM);
        ourShader.setInt("isEmissive", 1);
        ourShader.setVec3("emissiveColor", glm::vec3(1.0f));
        ourShader.setFloat("emissiveIntensity", 0.0f);
        VMModel.Draw(ourShader);

        // Model 5: DJI FPV Drone（放在面店左侧前方一点）
        glm::mat4 modelDJ = glm::mat4(1.0f);
        modelDJ = glm::translate(modelDJ, glm::vec3(-35.0f, -8.0f, 5.0f));
        modelDJ = glm::scale(modelDJ, glm::vec3(2.5f, 2.5f, 2.5f));
        // modelDJ = glm::rotate(modelDJ, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", modelDJ);
        ourShader.setInt("isEmissive", 0);
        ourShader.setFloat("emissiveIntensity", 0.0f);
        DJModel.Draw(ourShader);

        animShader.use();
        animShader.setMat4("projection", projection);
        animShader.setMat4("view", view);
        animShader.setFloat("uTime", currentFrame);
        
        // Set lighting uniforms for animated models
        animShader.setVec3("lightPos", glm::vec3(10.0f, 20.0f, 10.0f));
        animShader.setVec3("lightColor", glm::vec3(1.2f, 1.2f, 1.2f)); // brighter light color
        animShader.setVec3("viewPos", camera.Position);
        animShader.setFloat("ambientStrength", 0.4f); // increase ambient for character
        animShader.setFloat("specularStrength", 0.85f); // stronger specular
        animShader.setInt("shininess", 32);

        if (boneCount > 0)
        {
            updateArmHierarchy(boneInfoMap, boneMatrices, currentFrame);
            int matricesToSend = std::min((int)boneMatrices.size(), MAX_BONES);
            for (int i = 0; i < matricesToSend; ++i) {
                animShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", boneMatrices[i]);
            }
        }

        glm::mat4 model3 = glm::mat4(1.0f);
        model3 = glm::translate(model3, glm::vec3(10.0f, -10.0f, 25.0f));
        model3 = glm::scale(model3, glm::vec3(10.0f, 10.0f, 10.0f));
        animShader.setMat4("model", model3);

        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE); // 保留 alpha-to-coverage 平滑

        girlModel.DrawAnimated(animShader);


        // glfw: swap buffers and poll IO events
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate
    glfwTerminate();
    return 0;
}

unsigned int setupGroundPlane()
{
    float groundVertices[] = {
        -200.0f, 0.0f, -200.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         200.0f, 0.0f, -200.0f,  0.0f, 1.0f, 0.0f,  50.0f, 0.0f,
         200.0f, 0.0f,  200.0f,  0.0f, 1.0f, 0.0f,  50.0f, 50.0f,
        -200.0f, 0.0f,  200.0f,  0.0f, 1.0f, 0.0f,  0.0f, 50.0f
    };

    unsigned int groundIndices[] = { 0, 1, 2, 2, 3, 0 };

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(groundIndices), groundIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return VAO;
}

void processInput(GLFWwindow* window)
{
    float speedMultiplier = 1.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS)
    {
        speedMultiplier = 2.5f;
    }

    float adjustedDeltaTime = deltaTime * speedMultiplier;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, adjustedDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, adjustedDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, adjustedDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, adjustedDeltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
