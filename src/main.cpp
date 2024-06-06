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

unsigned int loadTexture(char const * path);

unsigned int loadCubemap(vector<std::string> faces);

void renderQuad();

void renderKvadrat();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

bool blinn = false;
float heightScale = 0.0;
bool hdr = true;
float exposure = 0.25f;
bool bloom = false;
bool grayscale = false;

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
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "pcelica i palcica", NULL, NULL);
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

    //face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    //blend za bele cvetove po livadi i za fenjer okacen o trem iznad zeca
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------
    Shader shaderModel("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader shaderSkybox("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader shaderCube("resources/shaders/cube.vs", "resources/shaders/cube.fs");
    Shader shaderNormal("resources/shaders/normal.vs", "resources/shaders/normal.fs");
    Shader shaderBlur("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    Shader shaderFinal("resources/shaders/final.vs", "resources/shaders/final.fs");

    float cubeVertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, // bottom-left
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  // bottom-right
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  // bottom-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  // top-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  // top-right
            // right face
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  // top-left
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  // bottom-right
            1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  // top-right
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  // bottom-right
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  // top-left
            1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  // bottom-left
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,  // top-right
            1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,  // top-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,  // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,  // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,  // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,  // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,  // top-left
            1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f,  // bottom-right
            1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,  // top-right
            1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,  // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,   // bottom-left

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

    //stranice za skybox
    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/left.jpg"),
                    FileSystem::getPath("resources/textures/skybox/right.jpg"),
                    FileSystem::getPath("resources/textures/skybox/top.jpg"),
                    FileSystem::getPath("resources/textures/skybox/bottom.jpg"),
                    FileSystem::getPath("resources/textures/skybox/front.jpg"),
                    FileSystem::getPath("resources/textures/skybox/back.jpg")
            };


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
    Model modelFenjer("resources/objects/lantern/scene.gltf");
    Model modelOstrvo("resources/objects/floating_island/scene.gltf");
    //Model modelZabica("resources/objects/frog/scene.gltf");
    Model modelGaleb("resources/objects/seagull/scene.gltf");
    Model modelGaleb2("resources/objects/derpy_seagull/scene.gltf");

    modelPcele.SetShaderTextureNamePrefix("material.");
    modelZec.SetShaderTextureNamePrefix("material.");
    modelCveta.SetShaderTextureNamePrefix("material.");
    modelTrellis.SetShaderTextureNamePrefix("material.");
    modelDrvo.SetShaderTextureNamePrefix("material.");
    modelCvece.SetShaderTextureNamePrefix("material.");
    modelPalcica.SetShaderTextureNamePrefix("material.");
    modelFenjer.SetShaderTextureNamePrefix("material.");
    modelOstrvo.SetShaderTextureNamePrefix("material.");
    modelGaleb.SetShaderTextureNamePrefix("material.");
    modelGaleb2.SetShaderTextureNamePrefix("material.");


    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/ocean_colour_plain.png").c_str());
    unsigned int normalMap  = loadTexture(FileSystem::getPath("resources/textures/ocean_normal.png").c_str());
    //unsigned int specularMap = loadTexture(FileSystem::getPath("resources/textures/specular.png").c_str());
    unsigned int depthMap = loadTexture(FileSystem::getPath("resources/textures/ocean_spray.png").c_str());

    shaderNormal.use();
    shaderNormal.setInt("diffuseMap", 0);
    shaderNormal.setInt("normalMap", 1);
   // shaderNormal.setInt("specularMap", 2);
    shaderNormal.setInt("depthMap", 3);

    //lights
//directional
    DirLight directional;
    directional.direction = glm::vec3(5.50f, 5.0f, 5.50f);
    directional.ambient = glm::vec3(1.0f, 1.0f, 0.7);
    directional.diffuse = glm::vec3(1.0f, 1.0f, 0.7);
    directional.specular = glm::vec3(1.0f, 1.0f, 0.7);

//spotlight
    SpotLight spotlight;
    spotlight.position = programState->camera.Position;
    spotlight.direction = programState->camera.Front;
    spotlight.ambient = glm::vec3(0.f);
    spotlight.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    spotlight.specular = glm::vec3(2.0f, 2.0f, 2.0f);
    spotlight.cutOff = glm::cos(glm::radians(12.5f));
    spotlight.outerCutOff = glm::cos(glm::radians(15.0f));
    spotlight.constant = 1.0f;
    spotlight.linear = 0.09f;
    spotlight.quadratic = 0.032f;

//pointlights
    PointLight pointLight; //pointlight za palcicu
    pointLight.position = glm::vec3(-15.0f, abs(sin(glfwGetTime())), 3.0f);
    pointLight.ambient = glm::vec3(3.0f, 3.0f, 3.0f);
    pointLight.diffuse = glm::vec3(3.0f, 3.0f, 3.0f);
    pointLight.specular = glm::vec3(3.000f, 3.0f, 3.0f);
    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    PointLight pointLight2; //pointlight za kocku u fenjeru
    pointLight2.position = glm::vec3(3.09f, 1.35f, -3.8f);
    pointLight2.ambient = glm::vec3(03.0f, 3.0f, 3.0f);
    pointLight2.diffuse = glm::vec3(3.0f, 3.0f, 3.0f);
    pointLight2.specular = glm::vec3(3.0f, 3.0f, 3.0f);
    pointLight2.constant = 1.0f;
    pointLight2.linear = 0.09f;
    pointLight2.quadratic = 0.032f;

    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    //check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        //check if framebuffers are complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }


    shaderFinal.use();
    shaderFinal.setInt("scene", 0);
    shaderFinal.setInt("bloomBlur", 1);
    shaderBlur.use();
    shaderBlur.setInt("image", 0);


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


        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glm::mat4 view = programState->camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 model = glm::mat4(1.0f);

        shaderModel.use();
        model = glm::mat4(1.0f);

        shaderModel.setMat4("model", model);
        shaderModel.setMat4("view", view);
        shaderModel.setMat4("projection", projection);

        // don't forget to enable shader before setting uniforms

        shaderModel.use();

        //POINTLIGHTs
        shaderModel.setVec3("pointLight.position", pointLight.position);
        shaderModel.setVec3("pointLight.ambient", pointLight.ambient);
        shaderModel.setVec3("pointLight.diffuse", pointLight.diffuse);
        shaderModel.setVec3("pointLight.specular", pointLight.specular);
        shaderModel.setFloat("pointLight.constant", pointLight.constant);
        shaderModel.setFloat("pointLight.linear", pointLight.linear);
        shaderModel.setFloat("pointLight.quadratic", pointLight.quadratic);
        shaderModel.setVec3("viewPosition", programState->camera.Position);
        shaderModel.setFloat("material.shininess", 32.0f);

        shaderModel.setVec3("pointLight2.position", pointLight2.position);
        shaderModel.setVec3("pointLight2.ambient", pointLight2.ambient);
        shaderModel.setVec3("pointLight2.diffuse", pointLight2.diffuse);
        shaderModel.setVec3("pointLight2.specular", pointLight2.specular);
        shaderModel.setFloat("pointLight2.constant", pointLight2.constant);
        shaderModel.setFloat("pointLight2.linear", pointLight2.linear);
        shaderModel.setFloat("pointLight2.quadratic", pointLight2.quadratic);
        shaderModel.setVec3("viewPosition", programState->camera.Position);
        shaderModel.setFloat("material.shininess", 32.0f);

        //spotlight
        shaderModel.setVec3("spotLight.position", programState->camera.Position);
        shaderModel.setVec3("spotLight.direction", programState->camera.Front);
        shaderModel.setVec3("spotLight.ambient", 1.0f, 1.0f, 1.0f);
        shaderModel.setVec3("spotLight.diffuse", 4.5f, 4.5f, 4.5f);
        shaderModel.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        shaderModel.setFloat("spotLight.constant", 1.0f);
        shaderModel.setFloat("spotLight.linear", 0.09);
        shaderModel.setFloat("spotLight.quadratic", 0.032);
        shaderModel.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        shaderModel.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

        //dirLight => ako je mrak, nema dirlight-a, ako je dan onda ima
        if (nightMode) {
            directional.direction = glm::vec3(5.50f, 5.0f, 5.50f);
            directional.ambient = glm::vec3(0.00f);
            directional.diffuse = glm::vec3(0.0f);
            directional.specular = glm::vec3(0.0f);
        }
        else{
            directional.direction = glm::vec3(5.50f, 5.0f, 5.50f);
            directional.ambient = glm::vec3(3.0f, 3.0f, 2.7);
            directional.diffuse = glm::vec3(3.0f, 3.0f, 2.7);
            directional.specular = glm::vec3(3.0f, 3.0f, 2.7);
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

        //pcelica
        model = glm::translate(model, glm::vec3(3*cos(glfwGetTime()),  abs(sin(glfwGetTime()))+0.4f,  3*sin(glfwGetTime()))); // translate it down so it's at the center of the scene
        model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0f, -1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.00015f));    // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelPcele.Draw(shaderModel);

        //prvi i drugi cve dodiruje pcela
        std::vector<float> z_coords_narcis = {0.0f, 0.0f, 1.0f, -2.0f};
        std::vector<float> x_coords_narcis = {3.0f, -3.0f, 0.0f, -2.0f};

        for(int i = 0; i < 4; i++){
            model = glm::mat4(1.0f);
            model= glm::translate(model, glm::vec3(x_coords_narcis[i], -0.4f, z_coords_narcis[i]));
            if(i == 1){
                //drugi cvet malo zarotiramo da bi pcela stala tacno na laticu
                model = glm::rotate(model, 90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
            }
            model = glm::scale(model, glm::vec3(0.01f));
            shaderModel.setMat4("model", model);
            modelCveta.Draw(shaderModel);
        }
        //drvo
        std::vector<float> z_coords_drvo = {-3.0f, 3.0f};
        std::vector<float> x_coords_drvo = {-4.0f, -21.0f};

        for(int i = 0; i < 2; i++){
            model = glm::mat4(1.0f);
            model= glm::translate(model, glm::vec3(x_coords_drvo[i], -0.4f, z_coords_drvo[i])); // translate it down so it's at the center of the scene
            model = glm::rotate(model, 1.571f, glm::vec3(-1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.03f));    // it's a bit too big for our scene, so scale it down
            shaderModel.setMat4("model", model);
            modelDrvo.Draw(shaderModel);
        }

        //zec
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(4.6f, -0.4f, -5.2f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.3f));   // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelZec.Draw(shaderModel);

        //trem iznad zeke
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(4.6f, -0.4f, -5.2f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.01f));   // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelTrellis.Draw(shaderModel);

        //cvece
        std::vector<float> z_coords_bele_rade = {-3.0f, -1.5f, 3.0f};
        std::vector<float> x_coords_bele_rade = {1.0f, 7.0f, -15.0f};
        std::vector<float> y_coords_bele_rade = {-0.4f, -0.4f, 0.1f};

        for(int i = 0; i < 3; i++) {
            model = glm::mat4(1.0f);
            model= glm::translate(model, glm::vec3(x_coords_bele_rade[i], y_coords_bele_rade[i], z_coords_bele_rade[i])); // translate it down so it's at the center of the scene
            model = glm::rotate(model, 1.57f, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.05f));   // it's a bit too big for our scene, so scale it down
            shaderModel.setMat4("model", model);
            modelCvece.Draw(shaderModel);
        }

        //palcica
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(-15.0f, abs(sin(glfwGetTime()))-0.6f, 3.0f)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, 8*abs(sin((float)glfwGetTime())), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f));   // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelPalcica.Draw(shaderModel);

        //zaba -> ipak necu i zabu
        /*std::vector<float> z_coords_zabice = {0.0f, 0.1f, 2.50f};
        std::vector<float> x_coords_zabice = {-15.0f, -7.0f, 3.09f};
        std::vector<float> y_coords_zabice = {0.1f, -0.75f, -0.4f};

        for(int i = 0; i < 1; i++) {
            model = glm::mat4(1.0f);
            model= glm::translate(model, glm::vec3(x_coords_zabice[i], y_coords_zabice[i], z_coords_zabice[i])); // translate it down so it's at the center of the scen
            model = glm::rotate(model, 3.14f, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.01f));   // it's a bit too big for our scene, so scale it down
            shaderModel.setMat4("model", model);
            modelZabica.Draw(shaderModel);
        }*/

        //dva galeba bela, prvi na mosticu, drugi na nebu
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(-7.0f, -0.2f, 0.1f)); // translate it down so it's at the center of the scen
        model = glm::rotate(model, 0.8f, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));   // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelGaleb.Draw(shaderModel);

        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(3*cos(glfwGetTime())+3.0f, 8.0f, 3*sin(glfwGetTime())+3.0f)); // translate it down so it's at the center of the scen
        model = glm::rotate(model, 1.57f, glm::vec3(-1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, 1.57f, glm::vec3(0.0f, 0.0f, -1.0f));
        model = glm::rotate(model, 1.57f, glm::vec3(0.0f, 0.0f, -1.0f));
        model = glm::rotate(model, 0.1f, glm::vec3(0.0f, -1.0f, 0.0f));
        model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0f, 0.0f, -1.0f));
        model = glm::scale(model, glm::vec3(0.1f));   // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelGaleb2.Draw(shaderModel);

        //kocka u fenjeru koja svetli
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        shaderCube.use();
        shaderCube.setMat4("view", view);
        shaderCube.setMat4("projection", projection);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(3.09f, 1.35f, -3.8f));
        model = glm::scale(model, glm::vec3(0.02f));
        shaderCube.setMat4("model", model);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDisable(GL_CULL_FACE);


        //cube mora pre fenjera da bi se prikazivala kocka unutra fenjera
        shaderModel.use();
        //fenjer okacen o trem
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(3.09f, 1.222f, -3.8f)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, 1.57f, glm::vec3(-1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(2.9f));   // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelFenjer.Draw(shaderModel);

        //ostrvo
        model = glm::mat4(1.0f);
        model= glm::translate(model, glm::vec3(-1.09f, -3.9f, -3.8f)); // translate it down so it's at the center of the scen
        model = glm::scale(model, glm::vec3(0.2));   // it's a bit too big for our scene, so scale it down
        shaderModel.setMat4("model", model);
        modelOstrvo.Draw(shaderModel);

        shaderNormal.use();
        shaderNormal.setMat4("projection", projection);
        shaderNormal.setMat4("view", view);
        model = glm::mat4(1.0f);

        shaderNormal.setMat4("model", model);
        shaderNormal.setVec3("viewPos", programState->camera.Position);

        shaderNormal.setVec3("pointLight.position", pointLight.position);
        shaderNormal.setVec3("pointLight.ambient", pointLight.ambient);
        shaderNormal.setVec3("pointLight.diffuse", pointLight.diffuse);
        shaderNormal.setVec3("pointLight.specular", pointLight.specular);
        shaderNormal.setFloat("pointLight.constant", pointLight.constant);
        shaderNormal.setFloat("pointLight.linear", pointLight.linear);
        shaderNormal.setFloat("pointLight.quadratic", pointLight.quadratic);

        //spotlight
        shaderNormal.setVec3("spotLight.position", programState->camera.Position);
        shaderNormal.setVec3("spotLight.direction", programState->camera.Front);
        shaderNormal.setVec3("spotLight.ambient", 1.0f, 1.0f, 1.0f);
        shaderNormal.setVec3("spotLight.diffuse", 4.5f, 4.5f, 4.5f);
        shaderNormal.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        shaderNormal.setFloat("spotLight.constant", 1.0f);
        shaderNormal.setFloat("spotLight.linear", 0.09);
        shaderNormal.setFloat("spotLight.quadratic", 0.032);
        shaderNormal.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        shaderNormal.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

        //dirLight
        if (nightMode) {
            directional.direction = glm::vec3(5.50f, 5.0f, 5.50f);
            directional.ambient = glm::vec3(0.00f);
            directional.diffuse = glm::vec3(0.0f);
            directional.specular = glm::vec3(0.0f);
        }
        else{
            directional.direction = glm::vec3(5.50f, 5.0f, 5.50f);
            directional.ambient = glm::vec3(3.0f, 3.0f, 2.7);
            directional.diffuse = glm::vec3(3.0f, 3.0f, 2.7);
            directional.specular = glm::vec3(3.0f, 3.0f, 2.7);
        }
        shaderNormal.setVec3("dirLight.direction", directional.direction);
        shaderNormal.setVec3("dirLight.ambient", directional.ambient);
        shaderNormal.setVec3("dirLight.diffuse", directional.diffuse);
        shaderNormal.setVec3("dirLight.specular", directional.specular);
        // view/projection transformations

        shaderNormal.setBool("blinn", blinn);

        shaderNormal.setFloat("heightScale", heightScale);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalMap);
        glActiveTexture(GL_TEXTURE2);
        //glBindTexture(GL_TEXTURE_2D, specularMap);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMap);

        renderQuad();

        //skybox => ako je noc, nebo je crno, ako je dan onda imamo skybox
        if(!nightMode) {
            glDepthFunc(GL_LEQUAL);
            shaderSkybox.use();
            shaderSkybox.setMat4("view", glm::mat4(glm::mat3(view)));
            shaderSkybox.setMat4("projection", projection);
            glBindVertexArray(skyboxVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
            glDepthFunc(GL_LESS);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //--------------------------------------------------------------------------------------------

        // blur bright fragments with two-pass Gaussian Blur
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        shaderBlur.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            shaderBlur.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);
            renderKvadrat();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderFinal.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        shaderFinal.setInt("bloom", bloom);
        shaderFinal.setInt("hdr", hdr);
        shaderFinal.setFloat("exposure", exposure);
        shaderFinal.setFloat("grayscale", grayscale);

        renderKvadrat();

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);


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
    if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS){
        if(heightScale > 0.0f){
            heightScale -= 0.0005f;
        } else {
            heightScale = 0.0f;
        }
    }
    if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS){
        if(heightScale < 1.0f){
            heightScale += 0.0005f;
        } else {
            heightScale = 1.0f;
        }
    }

    if(glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS){
        hdr = !hdr;
    }

    if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS){
        exposure -= 0.05;
    }

    if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS){
        exposure += 0.05;
    }

    if(glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS){
        bloom = !bloom;
    }

    if(glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS){
        grayscale = !grayscale;
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

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        // positions
        glm::vec3 pos1(100.0f,  -0.8f, 100.0f);
        glm::vec3 pos2(100.0f, -0.8f, -100.0f);
        glm::vec3 pos3( -100.0f, -0.8f, -100.0f);
        glm::vec3 pos4( -100.0f,  -0.8f, 100.0f);
        // texture coordinates
        glm::vec2 uv1(0.0f, 1.0f);
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(1.0f, 0.0f);
        glm::vec2 uv4(1.0f, 1.0f);
        // normal vector
        glm::vec3 nm(0.0f, 0.0f, 1.0f);

        // calculate tangent/bitangent vectors of both triangles
        glm::vec3 tangent1, bitangent1;
        glm::vec3 tangent2, bitangent2;
        // triangle 1
        // ----------
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        // triangle 2
        // ----------
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);


        bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);


        float quadVertices[] = {
                // positions            // normal         // texcoords  // tangent                          // bitangent
                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
        };
        // configure plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

unsigned int kvadVAO = 0;
unsigned int kvadVBO;
void renderKvadrat()
{
    if (kvadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &kvadVAO);
        glGenBuffers(1, &kvadVBO);
        glBindVertexArray(kvadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, kvadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(kvadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}