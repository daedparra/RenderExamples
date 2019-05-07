//=============================================================================
// VFSRenderingEnginesAndShaders
//=============================================================================

#include "model.h"
#include "shader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <vector>
#include <iostream>

//=============================================================================

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const float FLOOR_SIZE = 50.0f;
const float FLOOR_HALF_SIZE = FLOOR_SIZE * 0.5f;

//=============================================================================

struct Object
{
    virtual ~Object() {};
    virtual void Update( float const deltaTime ) {};
    virtual void Render() {};
};

//=============================================================================

struct Prop : public Object
{
    Prop( const std::shared_ptr<Model>& model, const std::shared_ptr<Shader>& shader, float const scale );
    virtual ~Prop() {};
    virtual void Update( float const deltaTime ) override;
    virtual void Render() override;

    std::shared_ptr<Model> mModel;
    std::shared_ptr<Shader> mShader;
    glm::mat4 mTransform;
    glm::vec2 mPosXZ;
    glm::vec2 mVelocityXZ;
    float mScale;
    float mOverrideDist;
    uint32_t mUpdateFrame;
};

//=============================================================================

struct Floor : public Object
{
    Floor( const std::shared_ptr<Model>& model, const std::shared_ptr<Shader>& shader );
    virtual ~Floor() {};
    virtual void Render() override;

    std::shared_ptr<Model> mModel;
    std::shared_ptr<Shader> mShader;
    glm::mat4 mTransform;
};

//=============================================================================

struct Camera : public Object
{
    Camera();
    virtual ~Camera() {};
    virtual void Update( float const deltaTime ) override;

    glm::vec3 mPosition;
    glm::vec2 mPitchYaw;
};

//=============================================================================

struct Light : public Object
{
    Light( const glm::vec3& color );
    virtual ~Light() {};
    virtual void Update( float const deltaTime ) override;

    glm::vec2 mPosXZ;
    glm::vec2 mVelocityXZ;
    glm::vec3 mColor;
    float mRadius;
};

//=============================================================================

struct GameState
{
    enum
    {
        BUTTON_UP = 1 << 0,
        BUTTON_LEFT = 1 << 1,
        BUTTON_DOWN = 1 << 2,
        BUTTON_RIGHT = 1 << 3,
    };

    GLFWwindow* mWindow;
    glm::mat4 mViewMatrix;
    glm::mat4 mCameraMatrix;
    glm::mat4 mProjectionMatrix;
    std::vector<std::shared_ptr<Object>> mObjects;
    std::vector<std::shared_ptr<Light>> mLights;
    uint32_t mButtonMask;
    glm::vec2 mPrevMousePos;
    glm::vec2 mCurMousePos;
    uint32_t mFrame;
    bool mPauseKey;
    bool mPaused;
};

//=============================================================================

static std::shared_ptr<GameState> gGameState;

//=============================================================================

Prop::Prop( const std::shared_ptr<Model>& model, const std::shared_ptr<Shader>& shader, float const scale ):
    mModel( model ),
    mShader( shader ),
    mScale( scale ),
    mOverrideDist( 0.0f ),
    mUpdateFrame( 0 )
{
    mPosXZ.x = -FLOOR_HALF_SIZE + ((float)(rand() % 101) / 100.0f * FLOOR_SIZE);
    mPosXZ.y = -FLOOR_HALF_SIZE + ((float)(rand() % 101) / 100.0f * FLOOR_SIZE);
    mVelocityXZ.x = -1.0f + ((float)(rand() % 101) / 100.0f * 2.0f);
    mVelocityXZ.y = -1.0f + ((float)(rand() % 101) / 100.0f * 2.0f);
    mVelocityXZ = glm::normalize( mVelocityXZ );
}

//=============================================================================

void Prop::Update( float const deltaTime )
{
    if (gGameState->mPaused)
        return;

    // If somebody else updated us ignore position update.
    if (mUpdateFrame != gGameState->mFrame)
    {
        // Calc new pos.
        float const speed = 2.5f;  // meters per second
        float const dist = deltaTime * speed;
        glm::vec2 const newPos = mPosXZ + (mVelocityXZ * dist);
        mOverrideDist = glm::max( mOverrideDist - dist, 0.0f );
        
        // Have we hit a wall.
        bool collision = newPos.x < -FLOOR_HALF_SIZE || newPos.x > FLOOR_HALF_SIZE ||
                         newPos.y < -FLOOR_HALF_SIZE || newPos.y > FLOOR_HALF_SIZE;

        // If we haven't hit a wall see if we hit anybody else.
        Prop* prop = nullptr;
        if (!collision && mOverrideDist == 0.0f)
        {
            for (const auto& obj : gGameState->mObjects)
            {
                prop = dynamic_cast<Prop*>( obj.get() );
                if (prop != nullptr && prop != this && prop->mUpdateFrame != gGameState->mFrame)
                {
                    collision = length( newPos - prop->mPosXZ ) < 0.5f;
                    if (collision)
                    {
                        mOverrideDist = 0.5f;
                        break;
                    }
                }
            }
        }

        if (collision)
        {
            mVelocityXZ = -mVelocityXZ;
            if (prop != nullptr)
            {
                prop->mVelocityXZ = -prop->mVelocityXZ;
                mOverrideDist = 0.5f;
                mUpdateFrame = gGameState->mFrame;
            }
        }
        else
        {
            mPosXZ = newPos;
        }
        mUpdateFrame = gGameState->mFrame;
    }

    glm::mat4 rot = glm::lookAt( glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( -mVelocityXZ.x, 0.0f, -mVelocityXZ.y ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
    rot = glm::inverse( rot );

    mTransform = glm::mat4( 1.0f );
    mTransform = glm::translate( mTransform, glm::vec3( mPosXZ.x, 0.0f, mPosXZ.y ) );
    mTransform *= rot;
    mTransform = glm::scale( mTransform, glm::vec3( mScale ) );
}

//=============================================================================

void Prop::Render()
{
    if (mModel != nullptr && mShader != nullptr)
    {
        glm::mat3 itModelMatrix( 1.0f );
        itModelMatrix[0] = normalize( glm::vec3( mTransform[0] ) );
        itModelMatrix[1] = normalize( glm::vec3( mTransform[1] ) );
        itModelMatrix[2] = normalize( glm::vec3( mTransform[2] ) );

        mShader->use();
        mShader->setMat4( "model", mTransform );
        mShader->setMat3( "itModel", itModelMatrix );
        mShader->setFloat( "shininess", 100.0f );
        mShader->setFloat( "diffuseScale", 1.0f );
        mShader->setFloat( "specularScale", 1.0f );
        mModel->Draw( *mShader );
    }
}

//=============================================================================

Floor::Floor( const std::shared_ptr<Model>& model, const std::shared_ptr<Shader>& shader ):
    mModel( model ),
    mShader( shader )
{
    mTransform = glm::mat4( 1.0f );
    mTransform = glm::scale( mTransform, glm::vec3( FLOOR_SIZE, 1.0f, FLOOR_SIZE ) );
}

//=============================================================================

void Floor::Render()
{
    if (mModel != nullptr && mShader != nullptr)
    {
        glm::mat3 itModelMatrix( 1.0f );
        itModelMatrix[0] = normalize( glm::vec3( mTransform[0] ) );
        itModelMatrix[1] = normalize( glm::vec3( mTransform[1] ) );
        itModelMatrix[2] = normalize( glm::vec3( mTransform[2] ) );

        mShader->use();
        mShader->setMat4( "model", mTransform );
        mShader->setMat3( "itModel", itModelMatrix );
        mShader->setFloat( "shininess", 100.0f );
        mShader->setFloat( "diffuseScale", 1.0f );
        mShader->setFloat( "specularScale", 0.0f );
        mModel->Draw( *mShader );
    }
}

//=============================================================================

Camera::Camera():
    mPosition( 0.0f, 13.0f, 23.0f ),
    mPitchYaw( 0.0f, -28.0f )
{
}

//=============================================================================

void Camera::Update( float const deltaTime )
{
    // Get window size.
    int wd;
    int ht;
    glfwGetWindowSize( gGameState->mWindow, &wd, &ht );
    glm::vec2 const windowSize = glm::vec2( (float)wd, (float)ht );
    float const aspectRatio = windowSize.x / windowSize.y;

    // Increment pitch yaw.
    glm::vec2 const rateOfRotation = glm::vec2( 90.0f * aspectRatio, 90.0f ); // degrees per normalized mouse movement
    glm::vec2 const normalizedMouseDelta = (gGameState->mCurMousePos - gGameState->mPrevMousePos) / windowSize;
    glm::vec2 const rotationDelta = -normalizedMouseDelta * rateOfRotation;
    mPitchYaw += rotationDelta;
    mPitchYaw.x = glm::mod( mPitchYaw.x, 360.0f );
    mPitchYaw.y = glm::clamp( mPitchYaw.y, -90.0f, 90.0f );

    // Calculate orientation.
    glm::mat4 transform( 1.0f );
    transform = glm::rotate( transform, glm::radians( mPitchYaw.x ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
    transform = glm::rotate( transform, glm::radians( mPitchYaw.y ), glm::vec3( 1.0f, 0.0f, 0.0f ) );

    // Update translation.
    float const speed = 5.0f;  // meters per second
    mPosition += (gGameState->mButtonMask & GameState::BUTTON_UP) ? -((speed * deltaTime) * glm::vec3( transform[2] )) : glm::vec3( 0.0f );
    mPosition += (gGameState->mButtonMask & GameState::BUTTON_DOWN) ? ((speed * deltaTime) * glm::vec3( transform[2] )) : glm::vec3( 0.0f );
    mPosition += (gGameState->mButtonMask & GameState::BUTTON_LEFT) ? -((speed * deltaTime) * glm::vec3( transform[0] )) : glm::vec3( 0.0f );
    mPosition += (gGameState->mButtonMask & GameState::BUTTON_RIGHT) ? ((speed * deltaTime) * glm::vec3( transform[0] )) : glm::vec3( 0.0f );
    transform[3] = glm::vec4( mPosition, 1.0f );

    gGameState->mCameraMatrix = transform;
    gGameState->mViewMatrix = glm::inverse( transform );

    // build projection matrix wd / ht aspect ratio with 45 degree field of view
    gGameState->mProjectionMatrix = glm::perspective( glm::radians( 45.0f ), windowSize.x / windowSize.y, 0.1f, 100.0f );
    //gGameState->mProjectionMatrix = glm::ortho( -10 * aspectRatio, 10.0f * aspectRatio, -FLOOR_HALF_SIZE, 10.0f, 0.1f, 100.0f );
}

//=============================================================================

Light::Light( const glm::vec3& color ):
    mColor( color ),
    mRadius( 10.0f )
{
    mPosXZ.x = -FLOOR_HALF_SIZE + ((float)(rand() % 101) / 100.0f * FLOOR_SIZE);
    mPosXZ.y = -FLOOR_HALF_SIZE + ((float)(rand() % 101) / 100.0f * FLOOR_SIZE);
    mVelocityXZ.x = -1.0f + ((float)(rand() % 101) / 100.0f * 2.0f);
    mVelocityXZ.y = -1.0f + ((float)(rand() % 101) / 100.0f * 2.0f);
    mVelocityXZ = glm::normalize( mVelocityXZ );
}

//=============================================================================

void Light::Update( float const deltaTime )
{
    if (gGameState->mPaused)
        return;

    float const speed = 5.0f;  // meters per second
    mPosXZ += mVelocityXZ * deltaTime * speed;
    if (mPosXZ.x < -FLOOR_HALF_SIZE || mPosXZ.x > FLOOR_HALF_SIZE ||
        mPosXZ.y < -FLOOR_HALF_SIZE || mPosXZ.y > FLOOR_HALF_SIZE)
    {
        mVelocityXZ.x = -1.0f + ((float)(rand() % 101) / 100.0f * 2.0f);
        mVelocityXZ.y = -1.0f + ((float)(rand() % 101) / 100.0f * 2.0f);
        mVelocityXZ = glm::normalize( mVelocityXZ );
        mPosXZ = glm::clamp( mPosXZ, -FLOOR_HALF_SIZE, FLOOR_HALF_SIZE );
    }
}

//=============================================================================

void ProcessInput()
{
    if (glfwGetKey( gGameState->mWindow, GLFW_KEY_ESCAPE ) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose( gGameState->mWindow, true );
    }

    gGameState->mButtonMask = 0;
    gGameState->mButtonMask |= (glfwGetKey( gGameState->mWindow, GLFW_KEY_UP ) == GLFW_PRESS || glfwGetKey( gGameState->mWindow, GLFW_KEY_W ) == GLFW_PRESS) ? GameState::BUTTON_UP : 0;
    gGameState->mButtonMask |= (glfwGetKey( gGameState->mWindow, GLFW_KEY_LEFT ) == GLFW_PRESS || glfwGetKey( gGameState->mWindow, GLFW_KEY_A ) == GLFW_PRESS) ? GameState::BUTTON_LEFT : 0;
    gGameState->mButtonMask |= (glfwGetKey( gGameState->mWindow, GLFW_KEY_DOWN ) == GLFW_PRESS || glfwGetKey( gGameState->mWindow, GLFW_KEY_S ) == GLFW_PRESS) ? GameState::BUTTON_DOWN : 0;
    gGameState->mButtonMask |= (glfwGetKey( gGameState->mWindow, GLFW_KEY_RIGHT ) == GLFW_PRESS || glfwGetKey( gGameState->mWindow, GLFW_KEY_D ) == GLFW_PRESS) ? GameState::BUTTON_RIGHT : 0;

    double xpos, ypos;
    glfwGetCursorPos( gGameState->mWindow, &xpos, &ypos );
    gGameState->mPrevMousePos = gGameState->mCurMousePos;
    gGameState->mCurMousePos.x = (float)xpos;
    gGameState->mCurMousePos.y = (float)ypos;

    bool const pauseKey = glfwGetKey( gGameState->mWindow, GLFW_KEY_P ) == GLFW_PRESS ? true : false;
    if (!pauseKey && gGameState->mPauseKey)
    {
        gGameState->mPaused = !gGameState->mPaused;
    }
    gGameState->mPauseKey = pauseKey;
}

//=============================================================================

void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // glfw: whenever the window size changed (by OS or user resize) this callback function executes
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    (void)window;
    glViewport(0, 0, width, height);
}

//=============================================================================

bool Init()
{

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    gGameState->mWindow = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", nullptr, nullptr);
    if (gGameState->mWindow == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(gGameState->mWindow);
    glfwSetFramebufferSizeCallback(gGameState->mWindow, FramebufferSizeCallback);

    // glad: load all OpenGL function pointers (extensions)
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwSetInputMode( gGameState->mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED );

    double xpos, ypos;
    glfwGetCursorPos( gGameState->mWindow, &xpos, &ypos );
    gGameState->mCurMousePos.x = (float)xpos;
    gGameState->mCurMousePos.y = (float)ypos;

    gGameState->mPauseKey = false;
    gGameState->mPaused = false;

    gGameState->mFrame = 1;

    srand( (uint32_t)(glfwGetTime() * 10000) );

    return true;
}

//=============================================================================

void Update( float const deltaTime )
{
    // process Input, AI, Physics, Collision Detection / Resolution, etc.
    
    // pump events
    glfwPollEvents();

    // process input
    ProcessInput();

    // update objects
    for (const auto& obj : gGameState->mObjects)
    {
        obj->Update( deltaTime );
    }
}

//=============================================================================

void PrepareShader( const std::shared_ptr<Shader>& shader )
{
    shader->use();

    // Set projection and view matrix.
    shader->setMat4( "projection", gGameState->mProjectionMatrix );
    shader->setMat4( "view", gGameState->mViewMatrix );

    // Set camera position.
    shader->setVec3( "cameraPos", gGameState->mCameraMatrix[3] );

    // Set lighting state.
    char nameStr[64];
    for (uint32_t i = 0; i < gGameState->mLights.size(); i++)
    {
        sprintf( nameStr, "lightPositions[%d]", i );
        shader->setVec3( nameStr, glm::vec3( gGameState->mLights[i]->mPosXZ.x, 2.0f, gGameState->mLights[i]->mPosXZ.y ) );
        sprintf( nameStr, "lightColors[%d]", i );
        shader->setVec3( nameStr, gGameState->mLights[i]->mColor );
        sprintf( nameStr, "lightRadii[%d]", i );
        shader->setFloat( nameStr, gGameState->mLights[i]->mRadius );
    }

}

//=============================================================================

void Render( const std::shared_ptr<Shader>& shader )
{
    //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable( GL_DEPTH_TEST );

    // Set shader constants.
    PrepareShader( shader );

    // Render objects
    for (const auto& obj : gGameState->mObjects)
    {
        obj->Render();
    }

    // Swap buffers.
    glfwSwapBuffers( gGameState->mWindow );
}

//=============================================================================

int main()
{
    // initialize OpenGL (3.3 Core Profile)
    gGameState = std::shared_ptr<GameState>( new GameState );
    Init();
    if (gGameState->mWindow == nullptr)
    {
        return -1;
    }

    // create shader program
    std::shared_ptr<Shader> modelShader( new Shader( "shaders/model.vs", "shaders/model.fs" ) );

    // load models
    // -----------
    std::shared_ptr<Model> propModelA( new Model( "objects/nanosuit/nanosuit.obj" ) );
    std::shared_ptr<Model> propModelB( new Model( "objects/cyborg/cyborg.obj" ) );

    // create floor mesh
    std::shared_ptr<Model> floorModel( new Model( "objects/floor/floor.obj" ) );

    // create camera object
    gGameState->mObjects.push_back( std::shared_ptr<Object>( new Camera() ) );

    // create floor object
    gGameState->mObjects.push_back( std::shared_ptr<Object>( new Floor( floorModel, modelShader ) ) );

    // create prop object
    uint32_t const numProps = 150;
    for (uint32_t i = 0; i < numProps; i++)
    {
        uint32_t const modelIndex = rand() % 2;
        gGameState->mObjects.push_back( std::shared_ptr<Object>( new Prop( modelIndex == 0 ? propModelA : propModelB, modelShader, modelIndex == 0 ? 0.125f : 0.5f ) ) );
    }

    // create lights
    uint32_t const numColors = 6;
    float const lightPower = 10.0f;
    glm::vec3 const colors[numColors] = 
    {
        glm::vec3( 0.25f, 1.0f, 0.25f ),
        glm::vec3( 0.25f, 0.25f, 1.0f ),
        glm::vec3( 1.0f, 0.25f, 0.25f ),
        glm::vec3( 1.0f, 1.0f, 0.25f ),
        glm::vec3( 0.25f, 1.0f, 1.0f ),
        glm::vec3( 1.0f, 0.25f, 1.0f ),
    };
    uint32_t const numLights = 10;
    for (uint32_t i = 0; i < numLights; i++)
    {
        std::shared_ptr<Light> light( new Light( colors[rand() % numColors] * lightPower ) );
        gGameState->mObjects.push_back( light );
        gGameState->mLights.push_back( light );
    }

    // game loop
    // -----------
    double t0 = glfwGetTime();
    while (!glfwWindowShouldClose(gGameState->mWindow))
    {
        // update
        double const t1 = glfwGetTime();
        Update( (float)(t1 - t0) );
        t0 = t1;

        // render objects (View Frustum Culling, Occlusion Culling, Draw Order Sorting, etc)
        Render( modelShader );

        gGameState->mFrame++;
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

//=============================================================================
