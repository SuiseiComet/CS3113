/**
* Author: WIlliam Wei
* Assignment : Pong Clone
* Date due : 2025 - 3 - 01, 11 : 59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
* 
* Resouces Cited:
* Ash Sprite: https://vsbattles.fandom.com/wiki/Ash_Ketchum
* Pikachu Sprite: https://www.pngegg.com/en/png-eqzse
* Pokeball Sprite: https://www.pngegg.com/en/png-cjome
* Ash Win Image :https://www.dexerto.com/anime/how-old-is-ash-ketchum-in-pokemon-2641589/
* Pikachu Win Image: https://pokemon.fandom.com/wiki/Exam_instructor%27s_Pikachu
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>

enum AppStatus { RUNNING, TERMINATED };

constexpr float WINDOW_SIZE_MULT = 1.5f;

constexpr int WINDOW_WIDTH  = 640 * WINDOW_SIZE_MULT,
              WINDOW_HEIGHT = 480 * WINDOW_SIZE_MULT;

constexpr float BG_RED     = 0.9765625f,
                BG_GREEN   = 0.97265625f,
                BG_BLUE    = 0.9609375f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr GLint NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL    = 0;
constexpr GLint TEXTURE_BORDER     = 0;

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char BALL_SPRITE_FILEPATH[] = "pokeball.png",
               PADDLE_RIGHT_SPRITE_FILEPATH[] = "ash.png",
               PADDLE_LEFT_SPRITE_FILEPATH[] = "pikachu.png",
               PLAYER_1_WINS_SPRITE_FILEPATH[] = "ash_wins.png",
               PLAYER_2_WINS_SPRITE_FILEPATH[] = "pikachu_wins.png";

constexpr float MINIMUM_COLLISION_DISTANCE = 1.0f;
constexpr glm::vec3 INIT_SCALE_PADDLE_LEFT  = glm::vec3(1.0f, 0.7f, 0.0f),
                    INIT_POS_PADDLE_LEFT    = glm::vec3(-4.2f, 2.0f, 0.0f),
                    
                    INIT_SCALE_PADDLE_RIGHT = glm::vec3(1.0f, 1.50f, 0.0f),
                    INIT_POS_PADDLE_RIGHT = glm::vec3(4.2f, 2.0f, 0.0f),

                    INIT_POS_BALL = glm::vec3(0.0f, 0.0f, 0.0f),
                    INIT_SCALE_BALL = glm::vec3(0.75f, 0.45f, 1.0f),
                    
                    INIT_SCALE_GAME_OVER_MSG = glm::vec3(6.75f, 4.5f, 1.0f),
                    INIT_POS_GAME_OVER_MSG = glm::vec3(0.0f, 0.0f, 0.0f);

struct ball{
    glm::vec3 INIT_POS = INIT_POS_BALL;
    glm::vec3 INIT_SCALE = INIT_SCALE_BALL;

    glm::vec3 movement;
    glm::mat4 matrix;
    glm::vec3 position = INIT_POS;
    glm::vec3 scale = INIT_SCALE;

} ball_1, ball_2, ball_3;

struct paddle {
    glm::vec3 INIT_POS;
    glm::vec3 INIT_SCALE;

    glm::vec3 movement;
    glm::mat4 matrix;
    glm::vec3 position = INIT_POS;
    glm::vec3 scale = INIT_SCALE;
} paddle_left, paddle_right;

SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();
glm::mat4 g_view_matrix, g_projection_matrix, g_game_over_msg_matrix;

float g_previous_ticks = 0.0f;
float g_paddle_left_frame_count = 0.0f;

GLuint g_ball_1_texture_id;
GLuint g_paddle_left_texture_id;
GLuint g_paddle_right_texture_id;
GLuint player_1_wins_texture_id;
GLuint player_2_wins_texture_id;

constexpr float PADDLE_SPEED = 3.0f,   //speed of paddles
                COMPUTER_PADDLE_SPEED = 1.0f,
                PADDLE_RADIUS = 3.45f,
                BALL_SPEED = 1.5f;

bool in_one_player_mode = false,
     player_1_wins = false,
     game_over = false;

int num_of_balls = 1;


void initialise();
void process_input();
void update();
void render();
void shutdown();

GLuint load_texture(const char* filepath)
{
    //STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    //STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    //STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("User-Input and Collisions Exercise",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);


    if (g_display_window == nullptr) shutdown();

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    ball_1.matrix = glm::mat4(1.0f);
    ball_1.movement = glm::vec3(-1.0f, 1.0f, 0.0f);

    ball_2.matrix = glm::mat4(1.0f);
    ball_2.movement = glm::vec3(1.0f, -1.0f, 0.0f);

    ball_3.matrix = glm::mat4(1.0f);
    ball_3.movement = glm::vec3(-1.0f, -1.0f, 0.0f);

    g_game_over_msg_matrix = glm::mat4(1.0f);
    g_game_over_msg_matrix = glm::scale(g_game_over_msg_matrix, INIT_SCALE_GAME_OVER_MSG);

    paddle_left.movement = glm::vec3(0.0f, 0.0f, 0.0f);
    paddle_left.position = glm::vec3(0.0f, 0.0f, 0.0f);
    paddle_left.INIT_SCALE = INIT_SCALE_PADDLE_LEFT;
    paddle_left.INIT_POS = INIT_POS_PADDLE_LEFT;

    paddle_right.movement = glm::vec3(0.0f, 0.0f, 0.0f);
    paddle_right.position = glm::vec3(0.0f, 0.0f, 0.0f);
    paddle_right.INIT_SCALE = INIT_SCALE_PADDLE_RIGHT;
    paddle_right.INIT_POS = INIT_POS_PADDLE_RIGHT;

    paddle_left.matrix = glm::mat4(1.0f);
    paddle_left.matrix = glm::translate(paddle_left.matrix, glm::vec3(1.0f, 1.0f, 0.0f));
    paddle_left.position += paddle_left.movement;

    paddle_right.matrix = glm::mat4(1.0f);
    paddle_right.matrix = glm::translate(paddle_right.matrix, glm::vec3(1.0f, 1.0f, 0.0f));
    paddle_right.position += paddle_right.movement;


    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_ball_1_texture_id = load_texture(BALL_SPRITE_FILEPATH);
    g_paddle_left_texture_id = load_texture(PADDLE_LEFT_SPRITE_FILEPATH);
    g_paddle_right_texture_id = load_texture(PADDLE_RIGHT_SPRITE_FILEPATH);
    player_1_wins_texture_id = load_texture(PLAYER_1_WINS_SPRITE_FILEPATH);
    player_2_wins_texture_id = load_texture(PLAYER_2_WINS_SPRITE_FILEPATH);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    //Every frame, turn off the x-component of the movement trigger vector only
    //since that is the only component that relies on user-input
    paddle_left.movement.y = 0.0f;
    paddle_right.movement.y = 0.0f;

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
                    case SDLK_q: g_app_status = TERMINATED; break;

                    // If the player presses the lowercase L key, then we're going to
                    // activate the drops' movement trigger vector's y-component in the
                    // downwards (negative) direction
                    case SDLK_t:
                        in_one_player_mode = !in_one_player_mode;
                        g_paddle_left_frame_count = 0.0f;
                        break;

                    default: break;
                }

            default:
                break;
        }
    }


    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    //Controls for player 1 (left)
    if      (key_state[SDL_SCANCODE_W]
                && paddle_left.position.y <= 1.35f
                && !in_one_player_mode 
                && !game_over) 
            paddle_left.movement.y = 1.0f;

    else if (key_state[SDL_SCANCODE_S] 
                && paddle_left.position.y >= -5.35f
                && !in_one_player_mode 
                && !game_over) 
            paddle_left.movement.y = -1.0f;

    // Controls for player 2 (right)
    if      (key_state[SDL_SCANCODE_UP] 
                && paddle_right.position.y <= 0.9f
                && !game_over) 
            paddle_right.movement.y = 1.0f;

    else if (key_state[SDL_SCANCODE_DOWN] 
                && paddle_right.position.y >= -4.95f
                && !game_over) 
            paddle_right.movement.y = -1.0f;

    if (key_state[SDL_SCANCODE_1]
        && !game_over)
        num_of_balls = 1;

    if (key_state[SDL_SCANCODE_2]
        && !game_over)
        num_of_balls = 2;
   
    if (key_state[SDL_SCANCODE_3]
        && !game_over)
        num_of_balls = 3;


    if (glm::length(paddle_left.movement) > 1.0f)
    {
         //In this particular case, I'm not going to normalise because I am simulating gravity
         paddle_left.movement = glm::normalize(paddle_left.movement);
    }
}

bool collision_occured(ball ball, paddle paddle) {
    float x_distance = fabs(paddle.position.x + paddle.INIT_POS.x - ball.position.x) -
        ((paddle.INIT_SCALE.x + ball.scale.x) / 2.0f);

    float y_distance = fabs(paddle.position.y + paddle.INIT_POS.y - ball.position.y) -
        ((paddle.INIT_SCALE.y + ball.scale.y) / 2.0f);

    if (x_distance < 0.0f && y_distance < 0.0f) {
        return true;
    }

    return false;
}


void update()
{
    //--- DELTA TIME CALCULATIONS --- 
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;


    //--- ACCUMULATOR LOGIC --- 
    paddle_left.position += paddle_left.movement * PADDLE_SPEED * delta_time;
    paddle_right.position += paddle_right.movement * PADDLE_SPEED * delta_time;

    if (in_one_player_mode && !game_over) {
        g_paddle_left_frame_count += COMPUTER_PADDLE_SPEED * delta_time;
        paddle_left.position.y = PADDLE_RADIUS * sin(g_paddle_left_frame_count) - 2.0f;
    }


    ball_1.position += ball_1.movement * BALL_SPEED * delta_time;
    if (num_of_balls > 1)
        ball_2.position += ball_2.movement * BALL_SPEED * delta_time;
    if (num_of_balls > 2)
        ball_3.position += ball_3.movement * BALL_SPEED * delta_time;


    if (   ball_1.position.x < -4.0f
        || ball_2.position.x < -4.0f
        || ball_3.position.x < -4.0f  ) {

        ball_1.position = glm::vec3(0.0f, 0.0f, 0.0f);
        ball_1.movement = glm::vec3(0.0f, 0.0f, 0.0f);
        ball_2.position = glm::vec3(0.0f, 0.0f, 0.0f);
        ball_2.movement = glm::vec3(0.0f, 0.0f, 0.0f);
        ball_3.position = glm::vec3(0.0f, 0.0f, 0.0f);
        ball_3.movement = glm::vec3(0.0f, 0.0f, 0.0f);
        game_over = true;
        player_1_wins = true;
    }

    if  (  ball_1.position.x > 4.0f
        || ball_2.position.x > 4.0f
        || ball_3.position.x > 4.0f) {

        ball_1.position = glm::vec3(0.0f, 0.0f, 0.0f);
        ball_1.movement = glm::vec3(0.0f, 0.0f, 0.0f);
        ball_2.position = glm::vec3(0.0f, 0.0f, 0.0f);
        ball_2.movement = glm::vec3(0.0f, 0.0f, 0.0f);
        ball_3.position = glm::vec3(0.0f, 0.0f, 0.0f);
        ball_3.movement = glm::vec3(0.0f, 0.0f, 0.0f);
        game_over = true;
        player_1_wins = false;
    }


    if (ball_1.position.y < -3.45f || ball_1.position.y > 3.45f) {
        ball_1.movement.y = -ball_1.movement.y;
        
        if (ball_1.position.y > 3.45f) ball_1.position.y = 3.45f;
        if (ball_1.position.y < -3.45f) ball_1.position.y = -3.45f;
   
    }
    if (ball_2.position.y < -3.45f || ball_2.position.y > 3.45f) {
        ball_2.movement.y = -ball_2.movement.y;

        if (ball_2.position.y > 3.45f) ball_2.position.y = 3.45f;
        if (ball_2.position.y < -3.45f) ball_2.position.y = -3.45f;

    }
    if (ball_3.position.y < -3.45f || ball_3.position.y > 3.45f) {
        ball_3.movement.y = -ball_1.movement.y;

        if (ball_3.position.y > 3.45f) ball_3.position.y = 3.45f;
        if (ball_3.position.y < -3.45f) ball_3.position.y = -3.45f;

    }

    //--- TRANSLATION --- 
    ball_1.matrix = glm::mat4(1.0f);
    ball_1.matrix = glm::translate(ball_1.matrix, ball_1.INIT_POS);
    ball_1.matrix = glm::translate(ball_1.matrix, ball_1.position);

    ball_2.matrix = glm::mat4(1.0f);
    ball_2.matrix = glm::translate(ball_2.matrix, ball_2.INIT_POS);
    ball_2.matrix = glm::translate(ball_2.matrix, ball_2.position);

    ball_3.matrix = glm::mat4(1.0f);
    ball_3.matrix = glm::translate(ball_3.matrix, ball_3.INIT_POS);
    ball_3.matrix = glm::translate(ball_3.matrix, ball_3.position);

    paddle_left.matrix = glm::mat4(1.0f);
    paddle_left.matrix = glm::translate(paddle_left.matrix, paddle_left.INIT_POS);
    paddle_left.matrix = glm::translate(paddle_left.matrix, paddle_left.position);

    paddle_right.matrix = glm::mat4(1.0f);
    paddle_right.matrix = glm::translate(paddle_right.matrix, paddle_right.INIT_POS);
    paddle_right.matrix = glm::translate(paddle_right.matrix, paddle_right.position);

    //--- SCALING --- 
    ball_1.matrix = glm::scale(ball_1.matrix, ball_1.scale);
    ball_2.matrix = glm::scale(ball_2.matrix, ball_1.scale);
    ball_3.matrix = glm::scale(ball_3.matrix, ball_1.scale);

    paddle_left.matrix  = glm::scale(paddle_left.matrix, paddle_left.INIT_SCALE);

    paddle_right.matrix = glm::scale(paddle_right.matrix, paddle_right.INIT_SCALE);

    //--- COLLISION LOGIC --- 
    // 1. Calculate box-to-box collision values
    if (collision_occured(ball_1, paddle_left)) {
        ball_1.movement.x = -ball_1.movement.x;
        
        
        // Move ball outside of paddles to remove some clipping
        ball_1.position.x = paddle_left.position.x + INIT_POS_PADDLE_LEFT.x + 1.0f;
    }

    if (collision_occured(ball_1, paddle_right)) {
        ball_1.movement.x = -ball_1.movement.x;
       

        // Move ball outside of paddles to hopefully remove some clipping
        ball_1.position.x = paddle_right.position.x + INIT_POS_PADDLE_RIGHT.x - 1.0f;
    }

    if (collision_occured(ball_2, paddle_left)) {
        ball_2.movement.x = -ball_2.movement.x;
     

        // Move ball outside of paddles to remove some clipping
        ball_2.position.x = paddle_left.position.x + INIT_POS_PADDLE_LEFT.x + 1.0f;
    }

    if (collision_occured(ball_2, paddle_right)) {
        ball_2.movement.x = -ball_2.movement.x;
        

        // Move ball outside of paddles to hopefully remove some clipping
        ball_2.position.x = paddle_right.position.x + INIT_POS_PADDLE_RIGHT.x - 1.0f;
    }

    if (collision_occured(ball_3, paddle_left)) {
        ball_3.movement.x = -ball_3.movement.x;


        // Move ball outside of paddles to remove some clipping
        ball_3.position.x = paddle_left.position.x + INIT_POS_PADDLE_LEFT.x + 1.0f;
    }

    if (collision_occured(ball_3, paddle_right)) {
        ball_3.movement.x = -ball_3.movement.x;


        // Move ball outside of paddles to hopefully remove some clipping
        ball_3.position.x = paddle_right.position.x + INIT_POS_PADDLE_RIGHT.x - 1.0f;
    }
}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);  //we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    //Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,   //triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f    //triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,      //triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,      //triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    //Bind texture
    draw_object(paddle_left.matrix, g_paddle_left_texture_id);
    draw_object(paddle_right.matrix, g_paddle_right_texture_id);
    draw_object(ball_1.matrix, g_ball_1_texture_id);
    if (num_of_balls > 1)
        draw_object(ball_2.matrix, g_ball_1_texture_id);
    if (num_of_balls > 2)
        draw_object(ball_3.matrix, g_ball_1_texture_id);

    if (game_over) {
        if (player_1_wins) draw_object(g_game_over_msg_matrix, player_1_wins_texture_id);
        else draw_object(g_game_over_msg_matrix, player_2_wins_texture_id);
    }
        

    //We disable two attribute arrays now
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
