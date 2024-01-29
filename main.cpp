#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/utils/logger.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <complex>
#include <chrono>

//test
using namespace std::chrono;
using namespace std;

struct ShaderProgramSource
{
    string VertexSource;
    string FragmentSource;
};

static ShaderProgramSource ParseShader(const string& filepath)
{
    ifstream stream(filepath);

    enum class ShaderType
    {
        NONE = -1, VERTEX = 0, FRAGMENT = 1
    };

    string line;
    stringstream ss[2];
    ShaderType type = ShaderType::NONE;
    while (getline(stream, line))
    {
        if (line.find("#shader") != string::npos)
        {
            if (line.find("vertex") != string::npos)
                type = ShaderType::VERTEX;
            else if (line.find("fragment") != string::npos)
                type = ShaderType::FRAGMENT;
        }
        else
        {
            ss[(int)type] << line << '\n';
        }
    }
    return { ss[0].str(), ss[1].str() };
}

static unsigned int CompileShader(unsigned int type, const string& source)
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << endl;
        cout << message << endl;
        glDeleteShader(id);
        return 0;
    }
    return id;
}

static int CreateShader(const string& vertexShader, const string& fragmentShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

const int width = 1920/2;
const int height = 1080/2;

const int HDwidth = 1920;
const int HDheight = 1080;

const float initialZoom = 2.0;
const int MAXMAXIT = 5000;
const int ITS_PER_COLOR = 30;
const float colStep = .1;
const float M_PI = 3.14159265358979323846;

float* OGColores = new float[3 * MAXMAXIT / ITS_PER_COLOR];
float* Colores = new float[3 * MAXMAXIT];

//Variables en parametros
float xOffset = -2.0;
float yOffset = -1.0;
float yStep = 0.25;
float xStep = (float)width/(float)height/4;
float zoom = initialZoom;
float dy = 1.0 / (float)height;
float dx = dy;
float HDdy = 1.0 / (float)HDheight;
float HDdx =  HDdy;
float maxIt = 30;
float samplesPerPixel = 1;
float R = 1.0;
float G = 1.0;
float B = 1.0;
float centerx = 0;
float centery = 0;
float a1 = 0;
double mouseX;
double mouseY;

//Variables en Julia
float xOffsetJ = -2.0;
float yOffsetJ = -2.0;
float yStepJ = 0.5;
float xStepJ = yStepJ;
float zoomJ = 4.0;
float deltaJ = 0.125 / (float)height;
float HDdeltaJ = 0.125 / (float)HDheight;
float RJ = 0.0;
float GJ = 1.0;
float BJ = 0.0;

bool lbuttonDown = false;
bool picture = false;
bool video = false;
bool Julia = false;
bool fiesta = false;
int pictureCounter = 1;
int frames = 100;

const complex<float> aRealStep(0.0001, 0.0);
const complex<float> aImagStep(0.0, 0.0001);
complex<float> a(0, 0);
complex<float> sqrta = sqrt(a);
complex<float> z0 = (float) 2.0 * sqrta;
complex<float> w0 = -z0;
float theta = 0.01;

cv::Mat M, JuliaMat;
cv::Mat rotMat = (cv::Mat_<float>(3, 3) << 1, 0, 0, 0, 1, 0, 0, 0, 1);
cv::Mat rotMatLR = (cv::Mat_<float>(3, 3) << cos(theta), -sin(theta), 0, sin(theta), cos(theta), 0, 0, 0, 1);
cv::Mat rotMatUD = (cv::Mat_<float>(3, 3) << cos(theta), 0, sin(theta), 0, 1, 0, -sin(theta), 0, cos(theta));
cv::VideoWriter outputVideo;
int codec = cv::VideoWriter::fourcc('m', 'p', '4', 'v');

void CalcColor()
{
    OGColores[0] = 1.0; OGColores[1] = 1.0; OGColores[2] = 1.0;
    for (int i = 1; i <= MAXMAXIT / ITS_PER_COLOR; i++)
    {
        OGColores[i * 3] = (float) rand() / RAND_MAX; 
        OGColores[i * 3 + 1] = (float) rand() / RAND_MAX;
        OGColores[i * 3 + 2] = (float) rand() / RAND_MAX;
    }
    int currColor = -1;
    for (int i = 0; i <= MAXMAXIT; i++)
    {
        float lerp = (float)(i % ITS_PER_COLOR) / (ITS_PER_COLOR - 1);
        if (i % ITS_PER_COLOR == 0) currColor++;
        Colores[i * 3] = OGColores[currColor * 3] * (1 - lerp) + OGColores[currColor * 3 + 3] * lerp;
        Colores[i * 3 + 1] = OGColores[currColor * 3 + 1] * (1 - lerp) + OGColores[currColor * 3 + 3 + 1] * lerp;
        Colores[i * 3 + 2] = OGColores[currColor * 3 + 2] * (1 - lerp) + OGColores[currColor * 3 + 3 + 2] * lerp;
    }
}

void InputMandelbrot(GLFWwindow* window, int key)
{
    switch (key)
    {
    case GLFW_KEY_W:
        yOffset += yStep;
        break;
    case GLFW_KEY_S:
        yOffset -= yStep;
        break;
    case GLFW_KEY_A:
        xOffset -= xStep;
        break;
    case GLFW_KEY_D:
        xOffset += xStep;
        break;
    case GLFW_KEY_EQUAL:
        xOffset += 2.0 * xStep;
        yOffset += 2.0 * yStep;
        zoom /= 2.0;
        xStep /= 2.0;
        yStep /= 2.0;
        dx /= 2.0;
        dy /= 2.0;
        HDdx /= 2.0;
        HDdy /= 2.0;
        break;
    case GLFW_KEY_MINUS:
        zoom *= 2.0;
        xStep *= 2.0;
        yStep *= 2.0;
        xOffset -= 2.0 * xStep;
        yOffset -= 2.0 * yStep;
        dx *= 2.0;
        dy *= 2.0;
        HDdx *= 2.0;
        HDdy *= 2.0;
        break;
    case GLFW_KEY_M:
        maxIt += 20;
        if (maxIt > MAXMAXIT)
        {
            maxIt = MAXMAXIT;
        }
        break;
    case GLFW_KEY_L:
        maxIt -= 20;
        if (maxIt < 0)
        {
            maxIt = 0;
        }
        break;
    case GLFW_KEY_B:
        if (samplesPerPixel == 1)
            samplesPerPixel = 5;
        else
            samplesPerPixel = 1;
        break;
    case GLFW_KEY_I:
        //cout <<rotMat<< endl;
        break;
    case GLFW_KEY_RIGHT:
        a += aRealStep;
        sqrta = sqrt(a);
        z0 = (float)2.0 * sqrta;
        w0 = -z0;
        break;
    case GLFW_KEY_LEFT:
        a -= aRealStep;
        sqrta = sqrt(a);
        z0 = (float)2.0 * sqrta;
        w0 = -z0;
        break;
    case GLFW_KEY_UP:
        a += aImagStep;
        sqrta = sqrt(a);
        z0 = (float)2.0 * sqrta;
        w0 = -z0;
        break;
    case GLFW_KEY_DOWN:
        a -= aImagStep;
        sqrta = sqrt(a);
        z0 = (float)2.0 * sqrta;
        w0 = -z0;
        break;
    case GLFW_KEY_1:
        R += colStep;
        break;
    case GLFW_KEY_2:
        G += colStep;
        break;
    case GLFW_KEY_3:
        B += colStep;
        break;
    case GLFW_KEY_PERIOD:
        R += colStep;
        G += colStep;
        B += colStep;
        break;
    case GLFW_KEY_COMMA:
        R -= colStep;
        G -= colStep;
        B -= colStep;
        break;
    case GLFW_KEY_F:
        fiesta = !fiesta;
        if (!fiesta)
        {
            R = 1;
            G = 1;
            B = 1;
        }
        break;
    case GLFW_KEY_P:
        picture = true;
        break;
    case GLFW_KEY_V:
        video = true;
        break;
    case GLFW_KEY_R:
        xOffset = -2.0;
        yOffset = -1.0;
        yStep = 0.25;
        xStep = (float)width / (float)height / 4;
        zoom = initialZoom;
        dy = 1.0 / (float)height;
        dx = dy;
        HDdy = 1.0 / (float)HDheight;
        HDdx = HDdy;
        maxIt = 100;
        samplesPerPixel = 1;
        R = 1.0;
        G = 1.0;
        B = 1.0;
        a = complex<float>(0, 0);
        sqrta = sqrt(a);
        z0 = (float)2.0 * sqrta;
        w0 = -z0;
        break;
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GL_TRUE);
        break;
    case GLFW_KEY_C:
        cout << "cx:" << endl;
        cin >> centerx;
        cout << "cy:" << endl;
        cin >> centery;
    default:
        break;
    }
}

void InputJulia(GLFWwindow* window, int key)
{
    switch (key)
    {
    case GLFW_KEY_W:
        yOffsetJ += yStepJ;
        break;
    case GLFW_KEY_S:
        yOffsetJ -= yStepJ;
        break;
    case GLFW_KEY_A:
        xOffsetJ -= xStepJ;
        break;
    case GLFW_KEY_D:
        xOffsetJ += xStepJ;
        break;
    case GLFW_KEY_EQUAL:
        xOffsetJ += 2.0 * xStepJ;
        yOffsetJ += 2.0 * yStepJ;
        zoomJ /= 2.0;
        xStepJ /= 2.0;
        yStepJ /= 2.0;
        deltaJ /= 2.0;
        HDdeltaJ /= 2.0;
        break;
    case GLFW_KEY_MINUS:
        zoomJ *= 2.0;
        xStepJ *= 2.0;
        yStepJ *= 2.0;
        xOffsetJ -= 2.0 * xStepJ;
        yOffsetJ -= 2.0 * yStepJ;
        deltaJ *= 2.0;
        HDdeltaJ *= 2.0;
        break;
    case GLFW_KEY_M:
        maxIt += 20;
        if (maxIt > MAXMAXIT)
        {
            maxIt = MAXMAXIT;
        }
        break;
    case GLFW_KEY_L:
        maxIt -= 20;
        if (maxIt < 0)
        {
            maxIt = 0;
        }
        break;
    case GLFW_KEY_B:
        if (samplesPerPixel == 1)
            samplesPerPixel = 5;
        else
            samplesPerPixel = 1;
        break;
    case GLFW_KEY_I:
        //cout <<rotMat<< endl;
        break;
    case GLFW_KEY_RIGHT:
        a += aRealStep;
        sqrta = sqrt(a);
        z0 = (float)2.0 * sqrta;
        w0 = -z0;
        break;
    case GLFW_KEY_LEFT:
        a -= aRealStep;
        sqrta = sqrt(a);
        z0 = (float)2.0 * sqrta;
        w0 = -z0;
        break;
    case GLFW_KEY_UP:
        a += aImagStep;
        sqrta = sqrt(a);
        z0 = (float)2.0 * sqrta;
        w0 = -z0;
        break;
    case GLFW_KEY_DOWN:
        a -= aImagStep;
        sqrta = sqrt(a);
        z0 = (float)2.0 * sqrta;
        w0 = -z0;
        break;
    case GLFW_KEY_1:
        RJ += colStep;
        break;
    case GLFW_KEY_2:
        GJ += colStep;
        break;
    case GLFW_KEY_3:
        BJ += colStep;
        break;
    case GLFW_KEY_PERIOD:
        RJ += colStep;
        GJ += colStep;
        BJ += colStep;
        break;
    case GLFW_KEY_COMMA:
        RJ -= colStep;
        GJ -= colStep;
        BJ -= colStep;
        break;
    case GLFW_KEY_P:
        picture = true;
        break;
    case GLFW_KEY_V:
        video = true;
        break;
    case GLFW_KEY_R:
        xOffsetJ = -2.0;
        yOffsetJ = -2.0;
        yStepJ = 0.5;
        xStepJ = (float)width / (float)height / 4;
        zoomJ = 4.0;
        deltaJ = 2.0 / (float)height;
        HDdeltaJ = 2.0 / (float)HDheight;
        RJ = 0.0;
        GJ = 1.0;
        BJ = 0.0;
        break;
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GL_TRUE);
        break;
    default:
        break;
    }
}

void ManageUserInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_RELEASE) return;
    if (Julia)
    {
        InputJulia(window, key);
    }
    else
    {
        InputMandelbrot(window, key);
    }
}

void ManageMouseInput(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        lbuttonDown = true;
    else
        lbuttonDown = false;
}

void CalcMouseCoords(GLFWwindow* window)
{
    if (lbuttonDown == false)
        return;

    glfwGetCursorPos(window, &mouseX, &mouseY);

    if (mouseX >= width)
    {
        Julia = true;
        return;
    }

    Julia = false;
    mouseY -= height;
    mouseY *= -1;
    centerx = mouseX * zoom / height + xOffset;
    centery = mouseY * zoom / height + yOffset;
}

float calcR(float t)
{
    return -2 / (1 + exp(-2 * t)) + 2;
}

int main(void)
{
    cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_SILENT);

    srand(290122);
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(width + height, height, "Mandelbrot", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    //glfwSwapInterval(1);

    if (glewInit() != GLEW_OK)
        cout << "Error!" << endl;

    float positions[] =
    {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    unsigned int indices[] =
    {
        0, 1, 2,
        2, 3, 0
    };

    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), positions, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, 0);

    unsigned int ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW);

    unsigned int fbo, render_buf;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &render_buf);
    glBindRenderbuffer(GL_RENDERBUFFER, render_buf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, HDwidth + HDheight, HDheight);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, render_buf);

    ShaderProgramSource  source = ParseShader("Basic.shader");

    unsigned int shader = CreateShader(source.VertexSource, source.FragmentSource);

    glUseProgram(shader);

    //CalcColor();
    //int ColorArrLocation = glGetUniformLocation(shader, "colorArr");
    //if (ColorArrLocation == -1) return -1;
    //glUniform3fv(ColorArrLocation, MAXMAXIT, Colores);

    M.create(HDheight, HDwidth + HDheight, CV_8UC3);
    JuliaMat.create(HDheight, HDheight, CV_8UC3);
    /*Mup.create(HDheight, HDwidth, CV_8UC3);
    Mup.data = M.data;
    MupReal.create(HDheight, HDwidth, CV_8UC3);
    Mdown.create(HDheight, HDwidth, CV_8UC3);
    Mdown.data = &M.data[HDheight * HDwidth * 3];*/

    int offLocation = glGetUniformLocation(shader, "offset");
    if (offLocation == -1) return -2;

    int zoomLocation = glGetUniformLocation(shader, "zoom");
    if (zoomLocation == -1) return -3;

    int maxItLocation = glGetUniformLocation(shader, "maxIt");
    if (maxItLocation == -1) return -4;

    int deltaLocation = glGetUniformLocation(shader, "delta");
    if (deltaLocation == -1) return -5;

    int SPPLocation = glGetUniformLocation(shader, "samplesPerPixel");
    if (SPPLocation == -1) return -6;

    int aLocation = glGetUniformLocation(shader, "a");
    //if (aLocation == -1) return -7;

    int colorLocation = glGetUniformLocation(shader, "inputColor");
    //if (colorLocation == -1) return -8;

    int z0Location = glGetUniformLocation(shader, "z0input");
    //if (z0Location == -1) return -9;

    int w0Location = glGetUniformLocation(shader, "w0input");
    //if (w0Location == -1) return -10;

    int heightLocation = glGetUniformLocation(shader, "height");
    if (heightLocation == -1) return -11;

    int centerLocation = glGetUniformLocation(shader, "center");
    //if (centerLocation == -1) return -12;

    int JuliaLocation = glGetUniformLocation(shader, "Julia");
    //if (JuliaLocation == -1) return -13;

    int rotLocation = glGetUniformLocation(shader, "rotMat");
    //if (rotLocation == -1) return -13;

    int a1Location = glGetUniformLocation(shader, "a1");
    //if (a1Location == -1) return -14;

    int widthLocation = glGetUniformLocation(shader, "width");
    if (widthLocation == -1) return -15;

    int offJLocation = glGetUniformLocation(shader, "offsetJ");
    if (offJLocation == -1) return -16;

    int zoomJLocation = glGetUniformLocation(shader, "zoomJ");
    if (zoomJLocation == -1) return -17;

    int deltaJLocation = glGetUniformLocation(shader, "deltaJ");
    if (deltaJLocation == -1) return -18;

    int colorJLocation = glGetUniformLocation(shader, "inputColorJ");
    if (colorJLocation == -1) return -19;

    glfwSetKeyCallback(window, ManageUserInput);
    glfwSetMouseButtonCallback(window, ManageMouseInput);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window) && !video)
    {
        cout << "a: " << a << "c: (" << centerx << ", " << centery << ")                          \r";

        if (fiesta)
        {
            R += ((float)rand() / RAND_MAX - 0.5) / 10;
            G += ((float)rand() / RAND_MAX - 0.5) / 10;
            B += ((float)rand() / RAND_MAX - 0.5) / 10;
        }

        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        if (picture)
        {
            glViewport(0, 0, HDwidth + HDheight, HDheight);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
            glUniform2f(deltaLocation, HDdx, HDdy);
            glUniform1f(deltaJLocation, HDdeltaJ);
            glUniform1f(heightLocation, HDheight);
            glUniform1f(widthLocation, HDwidth);
        }
        else
        {
            glUniform1f(deltaJLocation, deltaJ);
            glUniform2f(deltaLocation, dx, dy);
            glUniform1f(heightLocation, height);
            glUniform1f(widthLocation, width);
        }

        CalcMouseCoords(window);
        glUniform2f(offLocation, xOffset, yOffset);
        glUniform2f(offJLocation, xOffsetJ, yOffsetJ);
        glUniform1f(zoomLocation, zoom);
        glUniform1f(zoomJLocation, zoomJ);
        glUniform1f(maxItLocation, maxIt);
        glUniform1f(SPPLocation, samplesPerPixel);
        glUniform2f(aLocation, real(a), imag(a));
        glUniform2f(z0Location, real(z0), imag(z0));
        glUniform2f(w0Location, real(w0), imag(w0));
        glUniform3f(colorLocation, R, G, B);
        glUniform3f(colorJLocation, RJ, GJ, BJ);
        //glUniform1f(JuliaLocation, false);
        //glUniformMatrix3fv(rotLocation, 1, GL_FALSE, (float*)rotMat.data);
        glUniform2f(centerLocation, centerx, centery);
        //glUniform1f(a1Location, a1);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        if (picture)
        {
            cout << centerx << ", " << centery << endl;
            glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glReadPixels(0, 0, HDwidth + HDheight, HDheight, GL_BGR, GL_UNSIGNED_BYTE, M.data);
            cv::flip(M, M, 0);

            cv::imwrite("MandelbrotImg" + std::to_string(pictureCounter) + ".png", M);
            std::cout << "Imagen numero " << pictureCounter << " guardada!" << std::endl << std::endl;
            pictureCounter++;
            picture = false;
            glViewport(0, 0, width + height, height);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        }
        else
        {
            /* Swap front and back buffers */
            glfwSwapBuffers(window);
        }

        /* Poll for and process events */
        glfwWaitEvents();
        //glfwPollEvents();
    }

    if (!video) return 0;

    if (!outputVideo.open("VideoMandelbrot.mp4", codec, 30.0, JuliaMat.size(), true))
    {
        cout << "Problema al abrir el archivo" << endl;
        return -1;
    }

    glDeleteProgram(shader);

    source = ParseShader("OnlyJulia.shader");

    shader = CreateShader(source.VertexSource, source.FragmentSource);

    glUseProgram(shader);

    glViewport(0, 0, HDheight, HDheight);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

    auto start = high_resolution_clock::now();

    int radiusframes = 210;
    int aframes = 60;
    int totalCycles = 20;

    theta = 0;
    float thetaStep = M_PI / (10 * (aframes - 1));
    complex<float> c(0.25, 0);
    float t = 0;
    float r = calcR(t);

    frames = (aframes + 2*radiusframes) * totalCycles - aframes;

    float tStep = 5.0 / (radiusframes - 1);

    a = r * complex<float>(cos(theta), sin(theta));

    glUniform1f(glGetUniformLocation(shader, "delta"), HDdeltaJ);
    glUniform1f(glGetUniformLocation(shader, "height"), HDheight);
    glUniform2f(glGetUniformLocation(shader, "offset"), -2, -2);
    glUniform1f(glGetUniformLocation(shader, "zoom"), 4.0);
    glUniform1f(glGetUniformLocation(shader, "maxIt"), maxIt);
    glUniform1f(glGetUniformLocation(shader, "samplesPerPixel"), 9);
    glUniform3f(glGetUniformLocation(shader, "inputColor"), RJ, GJ, BJ);
    glUniform2f(glGetUniformLocation(shader, "c"), real(c), imag(c));

    cout << "                                                          \r";

    for (int cycle = 0; cycle < totalCycles; cycle++)
    {
        for (int i = 1; i <= radiusframes; i++)
        {
            cout << "Progress: " << (cycle * (aframes + 2 * radiusframes) + i) * 100 / frames << "%\r";

            glUniform2f(glGetUniformLocation(shader, "a"), real(a), imag(a));

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glReadPixels(0, 0, HDheight, HDheight, GL_BGR, GL_UNSIGNED_BYTE, JuliaMat.data);

            cv::flip(JuliaMat, JuliaMat, 0);

            outputVideo << JuliaMat;

            t += tStep;
            r = calcR(t);
            a = r * complex<float>(cos(theta), sin(theta));
        }

        t = 5;
        r = calcR(t);
        a = r * complex<float>(cos(theta), sin(theta));

        for (int i = 1; i <= radiusframes; i++)
        {
            cout << "Progress: " << (cycle * (aframes + 2*radiusframes) + i + radiusframes) * 100 / frames << "%\r";

            glUniform2f(glGetUniformLocation(shader, "a"), real(a), imag(a));

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glReadPixels(0, 0, HDheight, HDheight, GL_BGR, GL_UNSIGNED_BYTE, JuliaMat.data);

            cv::flip(JuliaMat, JuliaMat, 0);

            outputVideo << JuliaMat;

            t -= tStep;
            r = calcR(t);
            a = r * complex<float>(cos(theta), sin(theta));
        }

        t = 0;
        r = calcR(t);
        a = r * complex<float>(cos(theta), sin(theta));

        if (cycle == totalCycles - 1) break;

        for (int i = 1; i <= aframes; i++)
        {
            theta += thetaStep;
            a = r * complex<float>(cos(theta), sin(theta));

            cout << "Progress: " << (cycle * (aframes + 2*radiusframes) + 2*radiusframes + i) * 100 / frames << "%\r";

            glUniform2f(glGetUniformLocation(shader, "a"), real(a), imag(a));

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glReadPixels(0, 0, HDheight, HDheight, GL_BGR, GL_UNSIGNED_BYTE, JuliaMat.data);

            cv::flip(JuliaMat, JuliaMat, 0);

            outputVideo << JuliaMat;
        }
    }
    auto stop = high_resolution_clock::now();

    cout << endl << "Done in " << duration_cast<seconds>(stop - start).count() << " seconds!" << endl;

    glDeleteProgram(shader);

    glfwTerminate();



    return 0;
}