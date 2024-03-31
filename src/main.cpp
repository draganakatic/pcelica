#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadCubemap(vector<std::string> faces);
// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

bool blinn = false;
bool BKeyPressed = false;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);
bool nightMode = false;
int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__/home/gaga
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "zasto vredna pcelica kidise na cveT", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    //face culling <3
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    //blend <3
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------
    Shader shaderModel("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader shaderSkybox("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader shaderCube("resources/shaders/cube.vs", "resources/shaders/cube.fs");

    float cubeVertices[] = {
            // positions          // normals
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
            0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // cube VAO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));


    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/meadow_lf.jpg"),
                    FileSystem::getPath("resources/textures/skybox/meadow_rt.jpg"),
                    FileSystem::getPath("resources/textures/skybox/meadow_up.jpg"),
                    FileSystem::getPath("resources/textures/skybox/meadow_dn.jpg"),
                    FileSystem::getPath("resources/textures/skybox/meadow_ft.jpg"),
                    FileSystem::getPath("resources/textures/skybox/meadow_bk.jpg")
            };

    vector<std::string> faces2
            {
                    FileSystem::getPath("resources/textures/skybox/meadow_lf.jpg"),
                    FileSystem::getPath("resources/textures/skybox/meadow_rt.jpg"),
                    FileSystem::getPath("resources/textures/skybox/meadow_up.jpg"),
                    FileSystem::getPath("resources/textures/skybox/meadow_dn.jpg"),
                    FileSystem::getPath("resources/textures/skybox/meadow_ft.jpg"),
                    FileSystem::getPath("resources/textures/skybox/meadow_bk.jpg")
            };



    unsigned int cubemapTexture2 = loadCubemap(faces2);
    unsigned int cubemapTexture = loadCubemap(faces);


    shaderSkybox.use();
    shaderSkybox.setInt("skybox", 0);
    // load models
    // -----------
    Model modelPcele("resources/objects/bee/scene.gltf");
    Model modelZec("resources/objects/rabbit/scene.gltf");
    Model modelCveta("resources/objects/flower/scene.gltf");
    Model modelTrellis("resources/objects/trellis/scene.gltf");
    Model modelDrvo("resources/objects/tree/scene.gltf");
    Model modelCvece("resources/objects/flowers/scene.gltf");
    Model modelPalcica("resources/objects/yona_jinn/scene.gltf");
    //Model modelBalon("resources/objects/classic_muscle_car/scene.gltf");

    modelPcele.SetShaderTextureNamePrefix("material.");
    modelZec.SetShaderTextureNamePrefix("material.");
    modelCveta.SetShaderTextureNamePrefix("material.");
    modelTrellis.SetShaderTextureNamePrefix("material.");
    modelDrvo.SetShaderTextureNamePrefix("material.");
    modelCvece.SetShaderTextureNamePrefix("material.");
//    modelBalon.SetShaderTextureNamePrefix("material.");

    //lights
//directional

    DirLight directional;
    directional.direction = glm::vec3(5.50f, 5.0f, 5.50f);
    directional.ambient = glm::vec3(0.09f);
    directional.diffuse = glm::vec3(0.4f);
    directional.specular = glm::vec3(0.5f);

//spotlight
    SpotLight spotlight;
    spotlight.position = programState->camera.Position;
    spotlight.direction = programState->camera.Front;
    spotlight.ambient = glm::vec3(0.f);
    spotlight.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    spotlight.specular = glm::vec3(1.0f, 1.0f, 1.0f);
    spotlight.cutOff = glm::cos(glm::radians(12.5f));
    spotlight.outerCutOff = glm::cos(glm::radians(15.0f));
    spotlight.constant = 1.0f;
    spotlight.linear = 0.09f;
    spotlight.quadratic = 0.032f;

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(7.0f, abs(sin(glfwGetTime()))-2.0f, -1.5f);
    pointLight.ambient = glm::vec3(0.2, 0.2, 0.2);
    pointLight.diffuse = glm::vec3(0.90, 0.80, 0.0);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        // input
        // -----
        processInput(window);
        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //shaderModel.use();
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);

        shaderModel.setMat4("model", model);
        shaderModel.setMat4("view", view);
        shaderModel.setMat4("projection", projection);



        // don't forget to enable shader before setting uniforms

        shaderModel.use();

        //pointlight
        pointLight.position = glm::vec3(7.0f, abs(sin(glfwGetTime())) , -1.5f); //svetli palcica koja se pomera gore dole
        shaderModel.setVec3("pointLight.position", pointLight.position);
        shaderModel.setVec3("pointLight.ambient", pointLight.ambient);
        shaderModel.setVec3("pointLight.diffuse", pointLight.diffuse);
        shaderModel.setVec3("pointLight.specular", pointLight.specular);
        shaderModel.setFloat("pointLight.constant", pointLight.constant);
        shaderModel.setFloat("pointLight.linear", pointLight.linear);
        shaderModel.setFloat("pointLight.quadratic", pointLight.quadratic);
        shaderModel.setVec3("viewPosition", programState->camera.Position);
        shaderModel.setFloat("material.shininess", 32.0f);

        //spotlight
        shaderModel.setVec3("spotLight.position", programState->camera.Position);
        shaderModel.setVec3("spotLight.direction", programState->camera.Front);
        shaderModel.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        shaderModel.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        shaderModel.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        shaderModel.setFloat("spotLight.constant", 1.0f);
        shaderModel.setFloat("spotLight.linear", 0.09);
        shaderModel.setFloat("spotLight.quadratic", 0.032);
        shaderModel.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        shaderModel.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

        //dirLight
        if (nightMode) {
            directional.direction = glm::vec3(5.50f, 5.0f, 5.50f);
            directional.ambient = glm::vec3(0.00f);
            directional.diffuse = glm::vec3(0.0f);
            directional.specular = glm::vec3(0.0f);
        }
        else{
            directional.direction = glm::vec3(5.50f, 5.0f, 5.50f);
            directional.ambient = glm::vec3(0.80f);
            directional.diffuse = glm::vec3(0.4f);
            directional.specular = glm::vec3(0.5f);
        }
        shaderModel.setVec3("dirLight.direction", directional.direction);
        shaderModel.setVec3("dirLight.ambient", directional.ambient);
        shaderModel.setVec3("dirLight.diffuse", directional.diffuse);
        shaderModel.setVec3("dirLight.specular", directional.specular);
        // view/projection transformations

        shaderModel.setBool("blinn", blinn);

        shaderModel.setMat4("projection", projection);
        shaderModel.setMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);

        //pcela
        model = glm::translate(model, glm::vec3(3*cos(glfwGetTime()),  abs(sin(glfwGetTime())),  3*sin(glfwGetTime()))); // translate it down so it's at the center of the scene
        model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0f, -1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.00015f));    // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelPcele.Draw(shaderModel);


        //prvi cvet
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(3.0f, -0.8f, 0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.01f));    // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelCveta.Draw(shaderModel);

        //drugi cvet
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(-3.0f, -0.8f, 0.0f)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, 90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.01f));    // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelCveta.Draw(shaderModel);

        //treci cvet
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(0.0f, -0.8f, 1.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.01f));    // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelCveta.Draw(shaderModel);

        //cetvrti cvet
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(-2.0f, -0.8f, -2.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.01f));    // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelCveta.Draw(shaderModel);

        //drvo
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(-4.0f, -0.8f, -3.0f)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, 1.571f, glm::vec3(-1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.03f));    // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelDrvo.Draw(shaderModel);

        //zec
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(4.6f, -0.8f, -5.2f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.3f));   // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelZec.Draw(shaderModel);

        //trellis
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(4.6f, -0.8f, -5.2f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.01f));   // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelTrellis.Draw(shaderModel);

        //cvece
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(1.0f, -0.8f, -3.0f)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, 1.57f, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.05f));   // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelCvece.Draw(shaderModel);

        //cvece 2
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(-3.0f, -0.8f, -5.0f)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, 1.57f, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.05f));   // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelCvece.Draw(shaderModel);

        //cvece 3
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(7.0f, -0.8f, -1.5f)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, 1.57f, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.05f));   // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelCvece.Draw(shaderModel);
//
        //palcica
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(7.0f, abs(sin(glfwGetTime()))-2.0f, -1.5f)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, 2*abs(sin((float)glfwGetTime())), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f));   // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelPalcica.Draw(shaderModel);

        //cube
        shaderCube.use();
        shaderCube.setMat4("view", view);
        shaderCube.setMat4("projection", projection);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.2f));
        shaderCube.setMat4("model", model);

        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        //skybox

        glDepthFunc(GL_LEQUAL);
        shaderSkybox.use();
        shaderSkybox.setMat4("view", glm::mat4(glm::mat3(view)));
        shaderSkybox.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        if(nightMode) {
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        }
        else{
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture2);
        }
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
        nightMode = !nightMode;
    }
    //blinn
    if(glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS){
        blinn = !blinn;
    }
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

