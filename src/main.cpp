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

void set_spotLight(Shader& shader);

void renderQuad();
void renderQuad1();
void renderCube();

unsigned int loadCubemap(vector<std::string> faces);
unsigned int loadTexture(const char *path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
float heightScale = 0.005f;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
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
    glm::vec3 statuePosition = glm::vec3(0.0f);
    glm::vec3 pedestalPosition = glm::vec3(0.0f);
    float statueScale = 0.5f;
    float pedestalScale=0.006f;
    PointLight pointLight0;
    PointLight pointLight1;
    PointLight pointLight2;
    DirLight dirLight;
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
glm::vec3 lightPosition=glm::vec3(3.0f,0.8f,1.0f);
bool spotLightActivated=false;
bool bloom = false;
bool bloomKeyPressed = false;
float exposure = 0.5f;
int ind=0;
bool greyKeyPressed=false;
bool inverseKeyPressed=false;
bool blurKeyPressed=false;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
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
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs","resources/shaders/skybox.fs");

    Shader lightingShader("resources/shaders/lightCube.vs","resources/shaders/lightCube.fs");

    Shader normalShader("resources/shaders/normalmapping.vs","resources/shaders/normalmapping.fs");
    Shader parallaxShader("resources/shaders/parallax_mapping.vs","resources/shaders/parallax_mapping.fs");
    Shader blendingShader("resources/shaders/blending.vs","resources/shaders/blending.fs");

    //TO DO:doraditi "ourShader" kao "shader" da bi obradio reagovanje na bloom efekat
    Shader shaderBloom("resources/shaders/bloom.vs", "resources/shaders/light_box.fs");
    Shader shaderBlur("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    Shader shaderBloomFinal("resources/shaders/bloom_final.vs", "resources/shaders/bloom_final.fs");
    Shader b2Shader("resources/shaders/blending2.vs", "resources/shaders/blending2.fs");

    // load models
    // -----------
    Model statuaModel("resources/objects/LibertyStatue/LibertStatue.obj");
    Model postoljeModel("resources/objects/10421_square_pedastal_iterations-2.obj");

    statuaModel.SetShaderTextureNamePrefix("material.");
    postoljeModel.SetShaderTextureNamePrefix("material.");

    float cubeVertices[] = {
            // positions          // texture Coords
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
            0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    

    float vertices[] = {
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
            0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
            0.5f,  -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
            -0.5f,  -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
            -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f,  -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, 0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, 0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f,  -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
            0.5f, -0.5f, 0.5f,  0.0f, -1.0f,  0.0f,
            0.5f, -0.5f,  -0.5f,  0.0f, -1.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
            -0.5f, -0.5f,  -0.5f,  0.0f, -1.0f,  0.0f,
            -0.5f, -0.5f, 0.5f,  0.0f, -1.0f,  0.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };
    glFrontFace(GL_CW);


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

    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    glm::vec3 pointLightPositions[] = {
            glm::vec3( -4.0f,  4.0f,  2.0f),
            glm::vec3(4.0f, 4.0, -2.0),
            glm::vec3(2.0f, 4.0, -4.0)
    };
    
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

//

    // first, configure the cube's VAO (and VBO)
    unsigned int VBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
//Blending VAO

    // cube VAO
    unsigned int blendingVAO, blendingVBO;
    glGenVertexArrays(1, &blendingVAO);
    glGenBuffers(1, &blendingVBO);
    glBindVertexArray(blendingVAO);
    glBindBuffer(GL_ARRAY_BUFFER, blendingVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    
//Blending VAO    

//BLOOM/HDR
    // configure (floating point) framebuffers
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
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    //moramo da kazemo da cemo koristiit 2 color attachmenata za FRAGCOLOR i BRIGHTCOLOR
    glDrawBuffers(2, attachments); //koristicemo 2bafera za crtanje
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0); //Deaktiviramo FB

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2]; //za color attachmente(teksture)
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }
//BLOOM/HDR

    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);


    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skyboxtexture/px.jpg"),
                    FileSystem::getPath("resources/textures/skyboxtexture/nx.jpg"),
                    FileSystem::getPath("resources/textures/skyboxtexture/py.jpg"),
                    FileSystem::getPath("resources/textures/skyboxtexture/ny.jpg"),
                    FileSystem::getPath("resources/textures/skyboxtexture/pz.jpg"),
                    FileSystem::getPath("resources/textures/skyboxtexture/nz.jpg")
            };


    unsigned int cubemapTexture = loadCubemap(faces);

    unsigned int n_diffuseMap = loadTexture(FileSystem::getPath("resources/textures/marble_01_diff_4k.jpg").c_str());
    unsigned int n_normalMap  = loadTexture(FileSystem::getPath("resources/textures/marble_01_nor_gl_4k.jpg").c_str());

    unsigned int p_diffuseMap = loadTexture(FileSystem::getPath("resources/textures/floor_tiles_08_diff_4k.jpg").c_str());
    unsigned int p_normalMap = loadTexture(FileSystem::getPath("resources/textures/floor_tiles_08_nor_gl_4k.jpg").c_str());
    unsigned int p_heightMap = loadTexture(FileSystem::getPath("resources/textures/floor_tiles_08_disp_4k.jpg").c_str());

    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/11_ccexpress.png").c_str());
    unsigned int windowTexture = loadTexture(FileSystem::getPath("resources/textures/window.png").c_str());


    vector<glm::vec3> vegetation
            {
                    glm::vec3(-0.27f, -0.28f, -0.5f),
                    glm::vec3( -0.27f, -0.28f, 0.5f),

            };

    vector<glm::vec3> brickPos
    {
            glm::vec3(0.0f,-0.5,1.4f),
            glm::vec3(1.4f,-0.5,0.0f),
            glm::vec3(1.4f,-0.5,1.40),
            glm::vec3(0.0f,-0.5,-1.4f),
            glm::vec3(-1.4f,-0.5,0.0f),
            glm::vec3(-1.4f,-0.5,-1.40),
            glm::vec3(1.4f,-0.5,-1.4f),
            glm::vec3(-1.4f,-0.5,1.4f),

    };



    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    normalShader.use();
    normalShader.setInt("diffuseMap", 0);
    normalShader.setInt("normalMap", 1);

    parallaxShader.use();
    parallaxShader.setInt("diffuseMap", 0);
    parallaxShader.setInt("normalMap", 1);
    parallaxShader.setInt("depthMap", 2);

    shaderBlur.use();
    shaderBlur.setInt("image", 0);
    shaderBloomFinal.use();
    shaderBloomFinal.setInt("scene", 0);
    shaderBloomFinal.setInt("bloomBlur", 1);

    blendingShader.use();
    blendingShader.setInt("texture1",0);

    b2Shader.use();
    b2Shader.setInt("texture1", 0);

    PointLight& pointLight0 = programState->pointLight0;
    pointLight0.position = glm::vec3(pointLightPositions[0]);
    pointLight0.ambient = glm::vec3(0.6, 0.6, 0.6);
    pointLight0.diffuse = glm::vec3(0.8, 0.8, 0.8);
    pointLight0.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight0.constant = 1.0f;
    pointLight0.linear = 0.09f;
    pointLight0.quadratic = 0.032f;

    PointLight& pointLight1 = programState->pointLight1;
    pointLight1.position = glm::vec3(pointLightPositions[1]);
    pointLight1.ambient = glm::vec3(0.6, 0.6, 0.6);
    pointLight1.diffuse = glm::vec3(0.8, 0.8, 0.8);
    pointLight1.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight1.constant = 1.0f;
    pointLight1.linear = 0.09f;
    pointLight1.quadratic = 0.032f;


    PointLight& pointLight2 =programState->pointLight2;
    pointLight2.position =  glm::vec3(pointLightPositions[2]);
    pointLight2.ambient = glm::vec3(0.6, 0.6, 0.6);
    pointLight2.diffuse = glm::vec3(0.8, 0.8, 0.8);
    pointLight2.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight2.constant = 1.0f;
    pointLight2.linear = 0.09f;
    pointLight2.quadratic = 0.032f;

    DirLight& dirLight= programState->dirLight;
    dirLight.direction = glm::vec3(0.2f, 1.0f, 0.3f);
    dirLight.ambient = glm::vec3(0.2, 0.2, 0.2f);
    dirLight.diffuse = glm::vec3(0.4, 0.4, 0.4);
    dirLight.specular = glm::vec3(0.5, 0.5, 0.5);

    lightPosition.y=pointLight0.position.y;
    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);
        // render

//        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClearColor(0.0f,0.0f,0.0f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//
        // 1. render scene into floating point framebuffer
        // -----------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();
        pointLight0.position=pointLightPositions[0];
//        pointLight0.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
        ourShader.setVec3("dirLight.direction",dirLight.direction );
        ourShader.setFloat("material.shininess", 32.0f);

        // light properties
        ourShader.setVec3("dirLight.ambient",dirLight.ambient );
        ourShader.setVec3("dirLight.diffuse",  dirLight.diffuse);
        ourShader.setVec3("dirLight.specular", dirLight.specular);
        ourShader.setVec3("viewPosition", programState->camera.Position);


        ourShader.setVec3("pointLight0.position", pointLight0.position);
        ourShader.setVec3("pointLight0.ambient", pointLight0.ambient);
        ourShader.setVec3("pointLight0.diffuse", pointLight0.diffuse);
        ourShader.setVec3("pointLight0.specular", pointLight0.specular);
        ourShader.setFloat("pointLight0.constant", pointLight0.constant);
        ourShader.setFloat("pointLight0.linear", pointLight0.linear);
        ourShader.setFloat("pointLight0.quadratic", pointLight0.quadratic);

        ourShader.setVec3("pointLight1.position", pointLight1.position);
        ourShader.setVec3("pointLight1.ambient", pointLight1.ambient);
        ourShader.setVec3("pointLight1.diffuse", pointLight1.diffuse);
        ourShader.setVec3("pointLight1.specular", pointLight1.specular);
        ourShader.setFloat("pointLight1.constant", pointLight1.constant);
        ourShader.setFloat("pointLight1.linear", pointLight1.linear);
        ourShader.setFloat("pointLight1.quadratic", pointLight1.quadratic);

        ourShader.setVec3("pointLight2.position", pointLight2.position);
        ourShader.setVec3("pointLight2.ambient", pointLight2.ambient);
        ourShader.setVec3("pointLight2.diffuse", pointLight2.diffuse);
        ourShader.setVec3("pointLight2.specular", pointLight2.specular);
        ourShader.setFloat("pointLight2.constant", pointLight2.constant);
        ourShader.setFloat("pointLight2.linear", pointLight2.linear);
        ourShader.setFloat("pointLight2.quadratic", pointLight2.quadratic);

        set_spotLight(ourShader);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,programState->statuePosition); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(programState->statueScale));// it's a bit too big for our scene, so scale it down
        model = glm::rotate(model, glm::radians(currentFrame*50.0f),glm::vec3(0.0f,1.0f,0.0f));// it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        statuaModel.Draw(ourShader);


 // render the loaded model(Postolje)
        model = glm::mat4(1.0f);
        model = glm::translate(model,programState->pedestalPosition); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(programState->pedestalScale));// it's a bit too big for our scene, so scale it down
        model = glm::rotate(model, glm::radians(90.0f),glm::vec3(1.0f,0.0f,0.0f));
        ourShader.setMat4("model", model);
        postoljeModel.Draw(ourShader);

//Blending
        blendingShader.use();
        blendingShader.setMat4("projection",projection);
        blendingShader.setMat4("view",view);

        glDisable(GL_CULL_FACE);
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        for (unsigned int i = 0; i < vegetation.size(); i++)
        {

            model = glm::mat4(1.0f);
            model = glm::translate(model, vegetation[i]);
            model = glm::scale(model, glm::vec3(0.5f));

            blendingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glEnable(GL_CULL_FACE);

//BLENDING 2
        b2Shader.use();
        projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        model = glm::mat4(1.0f);
        b2Shader.setMat4("projection", projection);
        b2Shader.setMat4("view", view);

        glDisable(GL_CULL_FACE);
        glBindVertexArray(blendingVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, windowTexture);
        model = glm::translate(model, programState->statuePosition+glm::vec3(0.0,0.40,0.0));
        model = glm::scale(model,glm::vec3(0.3,0.75,0.3));
        b2Shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glEnable(GL_CULL_FACE);

//Normal mapping
        normalShader.use();
        normalShader.setMat4("projection", projection);
        normalShader.setMat4("view", view);
        model = glm::mat4(1.0f);

        model = glm::translate(model,glm::vec3(0.0f,-0.5,1.40));
        model = glm::scale(model, glm::vec3(0.7f));
        model = glm::rotate(model, glm::radians(90.0f), glm::normalize(glm::vec3(1.0, 0.0, 0.0))); // rotate the quad to show normal mapping from multiple directions
        model = glm::rotate(model, glm::radians(180.0f),glm::normalize(glm::vec3(0.0f,1.0f,.0f)));
        normalShader.setMat4("model", model);
        normalShader.setVec3("viewPos", programState->camera.Position);
//DODATO :PRITISKOM NA SPACE TJ AKTIVACIJOM BLOOM-A SVETLO BLOOM KOCKE SE PALI/GASI
        glm :: vec3 p;
        if (bloom){
           p=lightPosition;
        }
        else{
            p=pointLight0.position;
        }
//        normalShader.setVec3("lightPos",pointLight0.position );
        normalShader.setVec3("lightPos",p );
//
        glDisable(GL_CULL_FACE);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, n_diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, n_normalMap);
        for(unsigned int i=0;i < brickPos.size();i++) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, brickPos[i]);
            model = glm::rotate(model, glm::radians(90.0f), glm::normalize(glm::vec3(1.0, 0.0, 0.0))); // rotate the quad to show normal mapping from multiple directions
            model = glm::rotate(model, glm::radians(180.0f),glm::normalize(glm::vec3(0.0f,1.0f,.0f)));
            model = glm::scale(model, glm::vec3(0.7f));

            normalShader.setMat4("model", model);
            renderQuad();
        }
        glEnable(GL_CULL_FACE);

//Parallax Mapping
        parallaxShader.use();
        parallaxShader.setMat4("projection", projection);
        parallaxShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(0.0f,-0.5,0));
        model = glm::scale(model, glm::vec3(0.7f));
        model = glm::rotate(model, glm::radians(90.0f),glm::normalize(glm::vec3(1.0f,0.0f,.0f)));
        model = glm::rotate(model, glm::radians(180.0f),glm::normalize(glm::vec3(0.0f,1.0f,.0f)));
        parallaxShader.setMat4("model", model);
        parallaxShader.setVec3("viewPos", programState->camera.Position);
//        parallaxShader.setVec3("lightPos",pointLight0.position );
        parallaxShader.setVec3("lightPos",p );
        parallaxShader.setFloat("heightScale", heightScale);
        
        glDisable(GL_CULL_FACE);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, p_diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, p_normalMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, p_heightMap);
        renderQuad();
        glEnable(GL_CULL_FACE);


//drawing a light cubes(light bulbs)
        lightingShader.use();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        
        // we now draw as many light bulbs as we have point lights.
        glCullFace(GL_BACK);
        glBindVertexArray(cubeVAO);
        for (unsigned int i = 0; i < 2; i++)
        {
            model = glm::mat4(1.0f);
            glm::vec3 pos;
            if(i==0)
                pos = pointLight0.position;
            else
                pos = pointLight1.position;

            model = glm::translate(model, pos);
            model = glm::scale(model, glm::vec3(0.5f)); // Make it a smaller cube
            lightingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
// drawing a bloom light bulb
        shaderBloom.use();
        shaderBloom.setMat4("projection", projection);
        shaderBloom.setMat4("view", view);
        // world transformation
        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3( pointLight2.position));
        model = glm::scale(model, glm::vec3(0.3f)); // a smaller cube
        shaderBloom.setMat4("model", model);
        shaderBloom.setVec3("lightColor", glm::vec3(8.5f,  8.0f, 1.0f));
        glDisable(GL_CULL_FACE);
        renderCube();
        // render the cube
//        glBindVertexArray(cubeVAO);
//        glDrawArrays(GL_TRIANGLES, 0, 36);
        glEnable(GL_CULL_FACE);

//set transformation matrices
//        aashader.use();
//        projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
//        aashader.setMat4("projection", projection);
//        aashader.setMat4("view", programState->camera.GetViewMatrix());
//        model=glm::mat4(1.0f);
//        model = glm::translate(model, glm::vec3(2.1f,0.0f,0.5f)); // translate it down so it's at the center of the scene
//        model=glm::scale(model,glm::vec3(0.25f));
//        aashader.setMat4("model", model);
//        glBindVertexArray(cubeVAO);
//        glDrawArrays(GL_TRIANGLES, 0, 36);

        glCullFace(GL_FRONT);
// draw skybox as last

        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
//

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. blur bright fragments with two-pass Gaussian Blur
        // --------------------------------------------------
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        shaderBlur.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            shaderBlur.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad1();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 3. now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
        // --------------------------------------------------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaderBloomFinal.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        shaderBloomFinal.setInt("bloom", bloom);
        shaderBloomFinal.setFloat("exposure", exposure);
        shaderBloomFinal.setInt("ind", ind);
        renderQuad1();

        std::cout << "bloom: " << (bloom ? "on" : "off") << "| exposure: " << exposure << std::endl;


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
    //ind=0;
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}
//

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO1 = 0;
unsigned int cubeVBO1 = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO1 == 0)
    {
        float vertices[] = {
                // back face
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
                // front face
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                // left face
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                // right face
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
                // bottom face
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                // top face
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
                1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO1);
        glGenBuffers(1, &cubeVBO1);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO1);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO1);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO1);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}



// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO1 = 0;
unsigned int quadVBO1;
void renderQuad1()
{
    if (quadVAO1 == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO1);
        glGenBuffers(1, &quadVBO1);
        glBindVertexArray(quadVAO1);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO1);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}


unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        // positions
        glm::vec3 pos1(-1.0f,  1.0f, 0.0f);
        glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
        glm::vec3 pos3( 1.0f, -1.0f, 0.0f);
        glm::vec3 pos4( 1.0f,  1.0f, 0.0f);
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

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        programState->pointLight2.position+=glm::vec3(0.0f,0.0f,-1.0f) * 4.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        programState->pointLight2.position+=glm::vec3(0.0f,0.0f,1.0f)* 4.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        programState->pointLight2.position+=glm::vec3(-1.0f,0.0f,0.0f)*4.0f*deltaTime;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        programState->pointLight2.position+=glm::vec3(1.0f,0.0f,0.0f)*4.0f*deltaTime;



    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        if (heightScale > 0.0f)
            heightScale -= 0.0005f;
        else
            heightScale = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        if (heightScale < 1.0f)
            heightScale += 0.0005f;
        else
            heightScale = 1.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !bloomKeyPressed)
    {
        bloom = !bloom;
        bloomKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        bloomKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
    {
        if (exposure > 0.0f)
            exposure -= 0.01f;
        else
            exposure = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        exposure += 0.01f;
    }
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
        ImGui::Begin("Podesavanja");
        ImGui::Text("Podesavanje statue");

        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);

        ImGui::DragFloat3("Statue position", (float*)&programState->statuePosition);
        ImGui::DragFloat("Statue scale", &programState->statueScale, 0.05, 0.1, 4.0);

        ImGui::Text("Podesavanje postolja");
        ImGui::DragFloat3("Pedestal position", (float*)&programState->pedestalPosition);
        ImGui::DragFloat("Pedestal scale", &programState->pedestalScale, 0.05, 0.006, 4.0);


        ImGui::Text("Podesavanje pointLight0");
        ImGui::DragFloat("pointLight0.constant", &programState->pointLight0.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight0.linear", &programState->pointLight0.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight0.quadratic", &programState->pointLight0.quadratic, 0.05, 0.0, 1.0);

        ImGui::Text("Podesavanje pointLight1");
        ImGui::DragFloat("pointLight1.constant", &programState->pointLight1.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight1.linear", &programState->pointLight1.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight1.quadratic", &programState->pointLight1.quadratic, 0.05, 0.0, 1.0);

        ImGui::Text("Podesavanje pointLight2");
        ImGui::DragFloat("pointLight2.constant", &programState->pointLight2.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight2.linear", &programState->pointLight2.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight2.quadratic", &programState->pointLight2.quadratic, 0.05, 0.0, 1.0);

        ImGui::Text("Podesavanje direkcionog svetla");
        ImGui::DragFloat3("dirLight.direction", (float*)&programState->dirLight.direction,  0.05,0.0, 1.0);
        ImGui::DragFloat("dirLight.ambient", (float*)&programState->dirLight.ambient, 0.05, 0.0, 1.0);
        ImGui::DragFloat("dirLight.diffuse", (float*)&programState->dirLight.diffuse, 0.05, 0.0, 1.0);
        ImGui::DragFloat("dirLight.specular ", (float*)&programState->dirLight.specular, 0.05, 0.0, 1.0);



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


void set_spotLight(Shader& shader){
    shader.setVec3("spotLight.position",programState->camera.Position);
    shader.setVec3("spotLight.direction",programState->camera.Front);

    if(spotLightActivated){
        shader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        shader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        shader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
    }
    else{
        shader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        shader.setVec3("spotLight.diffuse", 0.0f, 0.0f, 0.0f);
        shader.setVec3("spotLight.specular", 0.0f, 0.0f, 0.0f);
    }
    shader.setFloat("spotLight.constant", 1.0f);
    shader.setFloat("spotLight.linear", 0.07);
    shader.setFloat("spotLight.quadratic", 0.001);
    shader.setFloat("spotLight.cutOff", glm::cos(glm::radians(3.0f)));
    shader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(21.0f)));

    shader.setVec3("viewPos",programState->camera.Position);
    shader.setFloat("material.shininess", 32.0f);

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
    else if(key == GLFW_KEY_F && action == GLFW_PRESS){
        spotLightActivated=!spotLightActivated;
    }

    else if(key == GLFW_KEY_I && action == GLFW_PRESS && !inverseKeyPressed){
        inverseKeyPressed=!inverseKeyPressed;
        ind= 1 ;
    }
    else if(key == GLFW_KEY_I && action == GLFW_PRESS && inverseKeyPressed){
        inverseKeyPressed=!inverseKeyPressed;
        ind= 0 ;
    }

    else if(key == GLFW_KEY_G && action == GLFW_PRESS && !greyKeyPressed){
        greyKeyPressed=!greyKeyPressed;
        ind= 2 ;
    }
    else if(key == GLFW_KEY_G && action == GLFW_PRESS && greyKeyPressed){
        greyKeyPressed=!greyKeyPressed;
        ind= 0 ;
    }

    else if(key == GLFW_KEY_B && action == GLFW_PRESS && !blurKeyPressed){
        blurKeyPressed=!blurKeyPressed;
        ind= 3 ;
    }
    else if(key == GLFW_KEY_B && action == GLFW_PRESS && blurKeyPressed){
        blurKeyPressed=!blurKeyPressed;
        ind= 0 ;
    }

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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
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


