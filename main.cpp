/**
* Author: Edrick Linardi
* Assignment: Pong Clone
* Date due: 2025-03-01, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH = 640 * 2,
WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED = 0.9765625f,
BG_GREEN = 0.97265625f,
BG_BLUE = 0.9609375f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

//Texture Variables
constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
LEVEL_OF_DETAIL = 0, // mipmap reduction image level
TEXTURE_BORDER = 0; // this value MUST be zero
GLuint ball_texture_id,
paddleL_texture_id,
paddleR_texture_id,
player1_win_texture_id,
player2_win_texture_id,
draw_texture_id;
constexpr char paddleL_sprite_filepath[] = "ryu_paddle.png",
paddleR_sprite_filepath[] = "ken_paddle.png",
ball_sprite_filepath[] = "hadouken_ball.png",
player1_sprite_filepath[] = "player1-win.png",
player2_sprite_filepath[] = "player2-win.png",
draw_sprite_filepath[] = "draw.png";

//Delta Time Variables
constexpr float MILLISECONDS_IN_SECOND = 1000.0f;
float g_previous_ticks = 0.0f;

//Matrices initialization
glm::mat4 g_view_matrix, g_projection_matrix;
glm::mat4 g_paddleL_matrix, g_paddleR_matrix, g_ball1_matrix, g_ball2_matrix, g_ball3_matrix, g_win_matrix;

//Left paddle initialization
constexpr glm::vec3 init_pos_paddleL = glm::vec3(-4.65f, 0.0f, 0.0f);
glm::vec3 paddleL_movement = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 paddleL_scale = glm::vec3(0.75f, 1.5f, 0.0f);

//Right paddle initialization
constexpr glm::vec3 init_pos_paddleR = glm::vec3(4.65f, 0.0f, 0.0f);
glm::vec3 paddleR_movement = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 paddleR_scale = glm::vec3(0.75f, 1.5f, 0.0f);

//Ball initialization
constexpr glm::vec3 init_pos_ball = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 ball1_movement = glm::vec3(-1.0f, 0.5f, 0.0f);
glm::vec3 ball2_movement = glm::vec3(1.0f, -0.5f, 0.0f);
glm::vec3 ball3_movement = glm::vec3(0.5f, 0.8f, 0.0f);
glm::vec3 ball_scale = glm::vec3(0.5f, 0.5f, 0.0f);

//Size and speed variables
const float paddle_width = 0.5f,
paddle_height = 2.0f;
const float ball_width = 0.5f,
ball_height = 0.5f;
const float paddle_speed = 5.0f,
ball_speed = 7.0f;

//Position variables
glm::vec3 paddleL_pos = init_pos_paddleL;
glm::vec3 paddleR_pos = init_pos_paddleR;
glm::vec3 ball1_pos = init_pos_ball;
glm::vec3 ball2_pos = init_pos_ball;
glm::vec3 ball3_pos = init_pos_ball;


//Other variables
bool isRunning = true;
bool isTwoPlayer = true;
bool paddleR_moveUp = true;
bool ballMove = false; //Indicating start of game (ball starts moving)
bool paddleL_collision = false;
bool paddleR_collision = false;
int activeBalls = 1; //Defaults to 1 ball
int ballCount = 1;
bool gameOver = false;
int winner = 0; //1 for Player 1, 2 for Player 2
int player1_score = 0;
int player2_score = 0;
const int winning_score = 2;


GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}


void initialise()
{
    // Initialise video
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Pong Clone",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_paddleL_matrix = glm::translate(glm::mat4(1.0f), init_pos_paddleL);
    g_paddleR_matrix = glm::translate(glm::mat4(1.0f), init_pos_paddleR);
    g_ball1_matrix = glm::translate(glm::mat4(1.0f), init_pos_ball);
    g_ball2_matrix = glm::translate(glm::mat4(1.0f), init_pos_ball);
    g_ball3_matrix = glm::translate(glm::mat4(1.0f), init_pos_ball);

    g_paddleL_matrix = glm::scale(g_paddleL_matrix, paddleL_scale);
    g_paddleR_matrix = glm::scale(g_paddleR_matrix, paddleR_scale);
    g_ball1_matrix = glm::scale(g_ball1_matrix, ball_scale);
    g_ball2_matrix = glm::scale(g_ball2_matrix, ball_scale);
    g_ball3_matrix = glm::scale(g_ball3_matrix, ball_scale);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    paddleL_texture_id = load_texture(paddleL_sprite_filepath);
    paddleR_texture_id = load_texture(paddleR_sprite_filepath);
    ball_texture_id = load_texture(ball_sprite_filepath);
    player1_win_texture_id = load_texture(player1_sprite_filepath);
    player2_win_texture_id = load_texture(player2_sprite_filepath);
    draw_texture_id = load_texture(draw_sprite_filepath);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_t: 
                isTwoPlayer != isTwoPlayer; //Switch between single and multiplayer
                break;
            case SDLK_SPACE: //Press spacebar for ball to start moving
                if (!ballMove) {
                    ballMove = true;

                    ball1_pos = init_pos_ball;
                    ball2_pos = init_pos_ball;
                    ball3_pos = init_pos_ball;

                    ball1_movement = glm::vec3(-1.0f, 0.5f, 0.0f);
                    ball2_movement = glm::vec3(1.0f, -0.5f, 0.0f);
                    ball3_movement = glm::vec3(0.5f, 0.8f, 0.0f);

                    ball1_movement = glm::normalize(ball1_movement);
                    ball2_movement = glm::normalize(ball2_movement);
                    ball3_movement = glm::normalize(ball3_movement);
                }
                break;
            case SDLK_1:
                activeBalls = 1;
                ballCount = 1;
                break;
            case SDLK_2:
                activeBalls = 2;
                ballCount = 2;
                break;
            case SDLK_3:
                activeBalls = 3;
                ballCount = 3;
                break;
            case SDLK_r: //Restart game
                if (gameOver) 
                {
                    gameOver = false;
                    winner = 0;
                    ballMove = false;
                    player1_score = 0;
                    player2_score = 0;
                    activeBalls = ballCount;

                    paddleL_pos = init_pos_paddleL;
                    paddleR_pos = init_pos_paddleR;
                    ball1_pos = init_pos_ball;
                    ball2_pos = init_pos_ball;
                    ball3_pos = init_pos_ball;

                    g_paddleL_matrix = glm::translate(glm::mat4(1.0f), paddleL_pos);
                    g_paddleR_matrix = glm::translate(glm::mat4(1.0f), paddleR_pos);
                    g_ball1_matrix = glm::translate(glm::mat4(1.0f), ball1_pos);
                    g_ball2_matrix = glm::translate(glm::mat4(1.0f), ball2_pos);
                    g_ball3_matrix = glm::translate(glm::mat4(1.0f), ball3_pos);
                    g_ball1_matrix = glm::scale(g_ball1_matrix, ball_scale);
                    g_ball2_matrix = glm::scale(g_ball2_matrix, ball_scale);
                    g_ball3_matrix = glm::scale(g_ball3_matrix, ball_scale);
                }
                break;
            default: break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    paddleL_movement.y = (key_state[SDL_SCANCODE_W]) ? 1.0f :
        (key_state[SDL_SCANCODE_S]) ? -1.0f : 0.0f;
    if (glm::length(paddleL_movement) > 1.0f)
        paddleL_movement = glm::normalize(paddleL_movement);

    if (isTwoPlayer) 
    {
        paddleR_movement.y = (key_state[SDL_SCANCODE_UP]) ? 1.0f :
            (key_state[SDL_SCANCODE_DOWN]) ? -1.0f : 0.0f;
        if (glm::length(paddleR_movement) > 1.0f)
            paddleR_movement = glm::normalize(paddleR_movement);
    }

}

void collisionChecker() { //Collision handler function
    float right_bound = 5.0f;
    float left_bound = -right_bound;
    float top_bound = 3.75f - (ball_height / 2.0f);
    float bottom_bound = -top_bound;

    int ballsRemaining = activeBalls; // Track how many balls are still in play

    //Ball 1 collision
    if (activeBalls >= 1)
    {
        float x_distance_L = fabs(ball1_pos.x - paddleL_pos.x) - ((ball_width + paddle_width) / 2.0f);
        float y_distance_L = fabs(ball1_pos.y - paddleL_pos.y) - ((ball_height + paddle_height) / 2.0f);
        if (x_distance_L < 0 && y_distance_L < 0) ball1_movement.x = fabs(ball1_movement.x);

        float x_distance_R = fabs(ball1_pos.x - paddleR_pos.x) - ((ball_width + paddle_width) / 2.0f);
        float y_distance_R = fabs(ball1_pos.y - paddleR_pos.y) - ((ball_height + paddle_height) / 2.0f);
        if (x_distance_R < 0 && y_distance_R < 0) ball1_movement.x = -fabs(ball1_movement.x);

        if (ball1_pos.y >= top_bound || ball1_pos.y <= bottom_bound) ball1_movement.y = -ball1_movement.y;

        // Check if ball 1 is out of bounds
        if (ball1_pos.x <= left_bound) {
            player2_score++; // Player 2 scores
            activeBalls--;   // Reduce number of active balls
        }
        else if (ball1_pos.x >= right_bound) {
            player1_score++; // Player 1 scores
            activeBalls--;   // Reduce number of active balls
        }
    }

    //Ball 2 collision
    if (activeBalls >= 2)
    {
        float x_distance_L = fabs(ball2_pos.x - paddleL_pos.x) - ((ball_width + paddle_width) / 2.0f);
        float y_distance_L = fabs(ball2_pos.y - paddleL_pos.y) - ((ball_height + paddle_height) / 2.0f);
        if (x_distance_L < 0 && y_distance_L < 0) ball2_movement.x = fabs(ball2_movement.x);

        float x_distance_R = fabs(ball2_pos.x - paddleR_pos.x) - ((ball_width + paddle_width) / 2.0f);
        float y_distance_R = fabs(ball2_pos.y - paddleR_pos.y) - ((ball_height + paddle_height) / 2.0f);
        if (x_distance_R < 0 && y_distance_R < 0) ball2_movement.x = -fabs(ball2_movement.x);

        if (ball2_pos.y >= top_bound || ball2_pos.y <= bottom_bound) ball2_movement.y = -ball2_movement.y;

        if (ball2_pos.x <= left_bound) {
            player2_score++;
            activeBalls--;
        }
        else if (ball2_pos.x >= right_bound) {
            player1_score++;
            activeBalls--;
        }
    }

    //Ball 3 collision
    if (activeBalls == 3)
    {
        float x_distance_L = fabs(ball3_pos.x - paddleL_pos.x) - ((ball_width + paddle_width) / 2.0f);
        float y_distance_L = fabs(ball3_pos.y - paddleL_pos.y) - ((ball_height + paddle_height) / 2.0f);
        if (x_distance_L < 0 && y_distance_L < 0) ball3_movement.x = fabs(ball3_movement.x);

        float x_distance_R = fabs(ball3_pos.x - paddleR_pos.x) - ((ball_width + paddle_width) / 2.0f);
        float y_distance_R = fabs(ball3_pos.y - paddleR_pos.y) - ((ball_height + paddle_height) / 2.0f);
        if (x_distance_R < 0 && y_distance_R < 0) ball3_movement.x = -fabs(ball3_movement.x);

        if (ball3_pos.y >= top_bound || ball3_pos.y <= bottom_bound) ball3_movement.y = -ball3_movement.y;

        if (ball3_pos.x <= left_bound) {
            player2_score++;
            activeBalls--;
        }
        else if (ball3_pos.x >= right_bound) {
            player1_score++;
            activeBalls--;
        }
    }

    // **Check if a player has won**
    if (ballCount == 1)
    {
        if (player1_score == 1)
        {
            gameOver = true;
            winner = 1;
        }
        else if (player2_score == 1)
        {
            gameOver = true;
            winner = 2;
        }
    }
    else if (ballCount == 2)
    {
        if (player1_score == 2)
        {
            gameOver = true;
            winner = 1;
        }
        else if (player2_score == 2)
        {
            gameOver = true;
            winner = 2;
        }
        else if(activeBalls == 0)
        {
            gameOver = true;
            winner = 0;
        }
    }
    else if (ballCount == 3)
    {
        if (player1_score >= 2)
        {
            gameOver = true;
            winner = 1;
        }
        else if (player2_score >= 2)
        {
            gameOver = true;
            winner = 2;
        }
        else if (activeBalls == 0)
        {
            gameOver = true;
            winner = 0;
        }
    }

    // **If all balls are gone and game isn't over, reset for next round**
    if (activeBalls == 0 && !gameOver) {
        activeBalls = ballCount;

        ball1_pos = init_pos_ball;
        ball2_pos = init_pos_ball;
        ball3_pos = init_pos_ball;
        ball1_movement = glm::vec3(-1.0f, 0.5f, 0.0f);
        ball2_movement = glm::vec3(1.0f, -0.5f, 0.0f);
        ball3_movement = glm::vec3(0.5f, 0.8f, 0.0f);
        activeBalls = 3; // Reset balls for next round
    }
}


void update() 
{
    //Delta time calculations
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    //Clamp paddles within the screen bounds
    float upper_bound = 3.75f - (paddle_height / 2.0f);
    float lower_bound = -3.75f + (paddle_height / 2.0f);

    //Move left paddle
    paddleL_pos += paddleL_movement * paddle_speed * delta_time;
    g_paddleL_matrix = glm::translate(glm::mat4(1.0f), paddleL_pos);
    g_paddleL_matrix = glm::scale(g_paddleL_matrix, paddleL_scale);

    //Move right paddle if two-player mode is active
    if (isTwoPlayer) 
    {
        paddleR_pos += paddleR_movement * paddle_speed * delta_time;
    }
    else //AI for single player mode
    {
        if (paddleR_moveUp) 
        {
            paddleR_pos.y += paddle_speed * delta_time;
        }
        else
        {
            paddleR_pos.y -= paddle_speed * delta_time;
        }

        if (paddleR_pos.y >= upper_bound) 
        {
            paddleR_moveUp = false;
        }
        else if (paddleR_pos.y <= lower_bound) 
        {
            paddleR_moveUp = true;
        }
    }
    g_paddleR_matrix = glm::translate(glm::mat4(1.0f), paddleR_pos);
    g_paddleR_matrix = glm::scale(g_paddleR_matrix, paddleR_scale);

    //Left paddle clamping
    if (paddleL_pos.y > upper_bound) {
        paddleL_pos.y = upper_bound;
    }
    else if (paddleL_pos.y < lower_bound) {
        paddleL_pos.y = lower_bound;
    }

    //Right paddle clamping
    if (paddleR_pos.y > upper_bound) {
        paddleR_pos.y = upper_bound;
    }
    else if (paddleR_pos.y < lower_bound) {
        paddleR_pos.y = lower_bound;
    }

    //Ball movement
    if (ballMove) 
    {
        if (activeBalls >= 1) 
        {
            ball1_pos += ball1_movement * ball_speed * delta_time;
            float flip_x1 = (ball1_movement.x < 0) ? -0.5f : 0.5f;
            g_ball1_matrix = glm::translate(glm::mat4(1.0f), ball1_pos);
            g_ball1_matrix = glm::scale(g_ball1_matrix, glm::vec3(flip_x1, ball_scale.y, ball_scale.z));
        }

        if (activeBalls >= 2) 
        {
            ball2_pos += ball2_movement * ball_speed * delta_time;
            float flip_x2 = (ball2_movement.x < 0) ? -0.5f : 0.5f;
            g_ball2_matrix = glm::translate(glm::mat4(1.0f), ball2_pos);
            g_ball2_matrix = glm::scale(g_ball2_matrix, glm::vec3(flip_x2, ball_scale.y, ball_scale.z));
        }

        if (activeBalls == 3) 
        {
            ball3_pos += ball3_movement * ball_speed * delta_time;
            float flip_x3 = (ball3_movement.x < 0) ? -0.5f : 0.5f;
            g_ball3_matrix = glm::translate(glm::mat4(1.0f), ball3_pos);
            g_ball3_matrix = glm::scale(g_ball3_matrix, glm::vec3(flip_x3, ball_scale.y, ball_scale.z));
        }
    }

    //Collision handling
    collisionChecker();


}


void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_paddleL_matrix, paddleL_texture_id);
    draw_object(g_paddleR_matrix, paddleR_texture_id);

    //Draw ball(s) depending on active balls variable

    if (gameOver)
    {
        g_win_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        g_win_matrix = glm::scale(g_win_matrix, glm::vec3(3.0f, 3.0f, 0.0f));
        if (winner == 1) draw_object(g_win_matrix, player1_win_texture_id);
        else if (winner == 2) draw_object(g_win_matrix, player2_win_texture_id);
        else draw_object(g_win_matrix, draw_texture_id);
    }
    else
    {
        if (activeBalls >= 1) draw_object(g_ball1_matrix, ball_texture_id);
        if (activeBalls >= 2) draw_object(g_ball2_matrix, ball_texture_id);
        if (activeBalls == 3) draw_object(g_ball3_matrix, ball_texture_id);
    }

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}