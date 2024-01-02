#pragma once

#include "Graphics.h"
#include "Grid.h"
#include "GridAstar.h"
#include "Mesh.h"
#include "Pool.h"
#include "Rect.h"
#include "SharedState.h"
#include "Utility.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <unordered_map>

namespace gfx = engine::graphics;

namespace state {

////////////////////////////////////////////////////////////////////////////////

enum InputStates {
    kInputLeft = 1 << 0,
    kInputRight = 1 << 1,
    kInputUp = 1 << 2,
    kInputDown = 1 << 3,
    kInputRun = 1 << 4,
    kInputMouseDown = 1 << 5,
};

struct Input {
    int states;

    int mouseX;
    int mouseY;

    float yaw;
    float pitch;
};

struct Player {
    glm::vec3 pos;
    glm::vec3 forward;
    glm::vec3 right;

    float pitch;
    float yaw;

    bool running;
    float runningMultiplier;
};

struct Camera {
    glm::vec3 pos;
    float pitch;
    float yaw;
    float roll;

    glm::vec3 forward;
    glm::vec3 right;
};

struct Animations {
    utils::ManualTimer cameraBobTimer;
    bool cameraBobEnable;
    float cameraBobOffset;
    float cameraBobPeriod;
};

struct Settings {
    bool enableCameraBobbing;
};

struct DefaultShader {
    gl::ShaderProgram program;
    int uProj;
    int uView;
    int uTint;
    int uEyePos;
    int uUseTexture;
    int uTexture;
    int uLightTransform;
    int uLightTexture;
    int uLightDirection;
};

struct IdentityShader {
    gl::ShaderProgram program;
    int uTexture;
};

struct Models {
    engine::Mesh cube;
    engine::Mesh room;
    engine::Mesh computer;
    engine::Mesh computerScreen;

    engine::Mesh screenQuad;
};

struct Textures {
    gl::Texture jahy;
    gl::Texture lastFrame;
};

struct Framebuffers {
    gl::Texture frambufferTexture1;
    gl::Texture frambufferDepthTexture1;
    gl::Framebuffer framebuffer1;
};

struct TransformEditInfo {
    int transformId;
    int rotationTicks;
    glm::mat4 initialModelTransform;
    glm::mat4 initialViewTransform;
};

struct Light {
    glm::mat4 transform;
    glm::vec3 direction;
};

// all internal rendering state
struct Rendering {
    int renderWidth;
    int renderHeight;

    int subRenderWidth;
    int subRenderHeight;

    int pixelelizeFactor;

    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 uiProjectionMatrix;

    glm::mat4 invViewMatrix;

    Models models;
    Textures textures;
    Framebuffers framebuffers;

    Light light;

    gfx::Buffer defaultBuffer;
    gfx::Buffer computerBuffer;
    gfx::Buffer computerScreenBuffer;

    gfx::Buffer screenQuadBuffer;

    DefaultShader defaultShader;
    IdentityShader identityShader;
};

struct Position {
    glm::vec3 pos;
};

struct PhysicsBody {};

struct Model {};

struct Pickups {
    data::Pool pool;
    data::List< Position > positions;
    data::List< PhysicsBody > bodies;
    data::List< Model > models;
};

struct EditMode {
    TransformEditInfo editInfo;
    bool editingEnabled;
};

////////////////////////////////////////////////////////////////////////////////

enum Tool : int {
    TOOL_TABLE = 0,
    TOOL_WALL,
    TOOL_HUMAN,
    TOOL_ERASE,
    TOOL_SETWAYPOINT,
};

struct DefaultShader2D {
    gl::ShaderProgram program;
    int uMat;
    int uTint;
    int uTexture;
    int uUseTexture;
};

struct BlurShader {
    gl::ShaderProgram program;
    int uTexture;
    int uBlurStep;
};

struct ConsoleShader {
    gl::ShaderProgram program;
    int uTexture;
    int uLastFrameTexture;
    int uGlowFactor;
    int uWipeHeight;
};

struct TycoonTextures {
    gl::Texture table;
    gl::Texture kitchen;
    gl::Texture miku;

    glm::vec2 tableHSize;
};

struct Wall {
    Rect rect;
};

using index_t = int;
using id_t = long;

struct Console {
    std::vector< std::string > lines;
    float timeLeft;
};

struct Humans {
    std::vector< id_t > ids;

    std::vector< glm::vec2 > walkers;
};

struct KitchenTypes {
    std::vector< id_t > ids;
    std::vector< id_t > recipes;
};

struct Kitchens {
    std::vector< id_t > ids;

    std::vector< glm::vec2 > positions;
    std::vector< id_t > types;
};

struct Tables {
    std::vector< id_t > ids;

    std::vector< glm::vec2 > positions;
    std::vector< int > customerCounts;
    std::vector< int > states;
};

struct Recipes {
    std::vector< id_t > ids;

    std::vector< int > durations;
    std::vector< int > prices;
};

struct ChefSkillTable {
    std::vector< id_t > chefs;
    std::vector< id_t > recipes;

    std::vector< int > skillLevels;
};

struct Chefs {
    std::vector< id_t > ids;

    std::vector< id_t > kitchens;
    std::vector< id_t > recipes;
    std::vector< id_t > customers;
    std::vector< int > states;
    std::vector< float > timers;
};

struct TargetEntry {
    id_t targeter;
    id_t target;
};

struct Customers {
    std::vector< id_t > id;

    std::vector< glm::vec2 > position;
    std::vector< glm::vec2 > velocity;
    std::vector< glm::vec2 > target;
    std::vector< glm::vec2 > subtarget;

    std::vector< float > repathTimer;

    std::vector< astar::Path * > path;
    std::vector< index_t > pathIndex;

    std::vector< id_t > orders;
    std::vector< id_t > tables;
    std::vector< int > states;
};

struct TycoonSimTransient {
    std::vector< index_t > idleChefs;
    std::vector< index_t > busyChefs;
};

struct TycoonSim {
    Kitchens kitchens;
    Tables tables;
    Chefs chefs;
    Customers customers;
    KitchenTypes kitchenTypes;

    std::vector< id_t > customersAtTarget;
    std::vector< TargetEntry > customersTargetingTables;
    std::vector< TargetEntry > customersTargetingKitchens;
};

struct Tycoon {
    DefaultShader2D defaultShader;
    BlurShader blurShader;
    ConsoleShader consoleShader;

    engine::BitmapFont font;
    gfx::TextBuffer textBuffer;
    gfx::TextBuffer moneyTextBuffer;
    gfx::TextBuffer consoleTextBuffer;

    gl::Texture pickupTexture;

    gfx::Buffer2D unitQuad;
    gfx::Buffer2D topLeftUnitQuad;
    gfx::Buffer2D arrowHead;

    gl::Texture lastFrame;

    gl::Framebuffer framebuffer1;
    gl::Texture frambufferTexture1;

    gl::Framebuffer framebuffer2;
    gl::Texture frambufferTexture2;

    glm::mat4 projMatrix;

    float elapsed;

    float moveTimer;
    int lastIndex;

    float runningAnimationTimer;
    int runningAnimationIndex;

    int consoleFactor;
    int consoleRenderWidth;
    int consoleRenderHeight;

    TycoonTextures textures;

    std::vector< glm::vec2 > tablePositions;
    std::vector< glm::vec2 > kitchenPositions;

    std::vector< glm::vec2 > tablePositionsAnim;

    std::vector< Rect > walls;

    glm::vec2 mousePosition;

    std::vector< Rect > selectorButtons;
    Tool selectedTool;
    int toolRotation;

    int waypointNum;
    glm::vec2 pathEndpoints[ 2 ];
    std::vector< grid::Coord > waypoints;
    std::vector< grid::Coord > debugPoints;

    Console console;
    Humans humans;

    TycoonSim tycoonSim;

    grid::Info collisionGridInfo;
    std::vector< int > collisionGridData;

    // TODO: this is just for testing path grid stuff
    std::vector< int > pathGridCosts;
    std::vector< unsigned char > pathGridField;

    int money;
    int moneyDisplayed;
};

////////////////////////////////////////////////////////////////////////////////

struct GameState {
    SharedState shared;
    Player player;
    Camera camera;
    Animations animations;
    Settings settings;
    Rendering rendering;
    Input input;

    std::vector< glm::mat4 > transforms;

    EditMode editMode;

    float renderTimestep;
    float tickTimestep;

    Tycoon tycoon;

    float inputLag;
};

} // namespace state
