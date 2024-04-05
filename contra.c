#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL_mixer.h>
#define MAX_BULLETS 1000
#define MAX_ENEMY_BULLETS 1000

typedef struct
{
  float x, y, dy;
  short life;
  char *name;
  int currentSprite, walking, facingLeft, shooting, visible;
  int alive;
  int health;
  int visibleTime;
  SDL_Texture *sheetTexture;
} Man;

typedef struct
{
  float x, y, dx, dy;
  short active;
} EnemyBullet;
 
static int totalDamageman = 0;     // Counter to track shots that hit man 
static int totalDamageenemy = 0;   // Counter to track shots that hit the enemy

typedef struct
{
  float x, y, dx;
} Bullet;

SDL_Texture *bulletTexture;
SDL_Texture *enemyBulletTexture;
SDL_Texture *backgroundTexture;
Bullet *bullets[MAX_BULLETS] = {NULL};
EnemyBullet enemyBullets[MAX_ENEMY_BULLETS] = {0};
Man enemy;

int globalTime = 0;
Mix_Chunk *shoot;
Mix_Music *bgm;
Mix_Music *WinMusic;
Mix_Music *LossMusic;
Mix_Music *death;
void showStartScreen(SDL_Renderer *renderer)
{
  SDL_Texture *startTexture;
  SDL_Surface *startSurface = IMG_Load("start_screen.png");
  if (!startSurface)
  {
    printf("Cannot find start screen image\n");
    return;
  }

  startTexture = SDL_CreateTextureFromSurface(renderer, startSurface);
  SDL_FreeSurface(startSurface);

  if (!startTexture)
  {
    printf("Unable to create texture from start screen image\n");
    return;
  }

  SDL_Rect startRect = {0, 50, 320, 120}; // Adjust the dimensions as per your start screen image size
  SDL_RenderCopy(renderer, startTexture, NULL, &startRect);
  SDL_RenderPresent(renderer);

  int waiting = 1;
  SDL_Event event;
  while (waiting)
  {
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_KEYDOWN)
      {
        waiting = 0;
      }
    }
    SDL_Delay(10);
  }

  SDL_DestroyTexture(startTexture);
}

void showWinScreen(SDL_Renderer *renderer)
{
  SDL_Texture *winTexture;
  SDL_Surface *winSurface = IMG_Load("win_screen.png"); // Replace "start_screen.png" with your actual start screen image

  if (!winSurface)
  {
    printf("Cannot find start screen image\n");
    return;
  }

  winTexture = SDL_CreateTextureFromSurface(renderer, winSurface);
  SDL_FreeSurface(winSurface);

  if (!winSurface)
  {
    printf("Unable to create texture from start screen image\n");
    return;
  }

  SDL_Rect winRect = {0, 0, 320, 240}; // Adjust the dimensions as per your start screen image size
  SDL_RenderCopy(renderer, winTexture, NULL, &winRect);
  SDL_RenderPresent(renderer);
  const Uint8 *state = SDL_GetKeyboardState(NULL);

  int waiting = 1;
  SDL_Event event;
  while (waiting)
  {
    while (SDL_PollEvent(&event))
    {
      if (state[SDL_SCANCODE_KP_ENTER])
      {
        waiting = 0;
        SDL_Quit(); // End the game
        exit(0);
      }
    }
    SDL_Delay(10);
  }

  SDL_DestroyTexture(winTexture);
}

void showLossScreen(SDL_Renderer *renderer)
{
  SDL_Texture *lossTexture;
  SDL_Surface *lossSurface = IMG_Load("loss_screen.png"); // Replace "start_screen.png" with your actual start screen image

  if (!lossSurface)
  {
    printf("Cannot find start screen image\n");
    return;
  }

  lossTexture = SDL_CreateTextureFromSurface(renderer, lossSurface);
  SDL_FreeSurface(lossSurface);

  if (!lossSurface)
  {
    printf("Unable to create texture from start screen image\n");
    return;
  }

  SDL_Rect lossRect = {0, 0, 320, 240}; // Adjust the dimensions as per your start screen image size
  SDL_RenderCopy(renderer, lossTexture, NULL, &lossRect);
  SDL_RenderPresent(renderer);
  const Uint8 *state = SDL_GetKeyboardState(NULL);

  int waiting = 1;
  SDL_Event event;
  while (waiting)
  {
    while (SDL_PollEvent(&event))
    {
      if(state[SDL_SCANCODE_KP_ENTER])
      {
        waiting = 0;
        SDL_Quit();
        exit(0);
      }
    }
    SDL_Delay(10);
  }

  SDL_DestroyTexture(lossTexture);
}

void addBullet(float x, float y, float dx)
{
  int found = -1;
  for (int i = 0; i < MAX_BULLETS; i++)
  {
    if (bullets[i] == NULL)
    {
      found = i;
      break;
    }
  }

  if (found >= 0)
  {
    int i = found;
    bullets[i] = (Bullet *)malloc(sizeof(Bullet));
    bullets[i]->x = x;
    bullets[i]->y = y;
    bullets[i]->dx = dx;
  }
}

void removeBullet(int i)
{
  if (bullets[i])
  {
    free(bullets[i]);
    bullets[i] = NULL;
  }
}

void addEnemyBullet(float x, float y, float dx, float dy)
{
  for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
  {
    if (!enemyBullets[i].active)
    {
      enemyBullets[i].x = x;
      enemyBullets[i].y = y;
      enemyBullets[i].dx = dx;
      enemyBullets[i].dy = dy;
      enemyBullets[i].active = 1;
      break;
    }
  }
}

// Function to render enemy bullets
void renderEnemyBullets(SDL_Renderer *renderer)
{
  SDL_Surface *enemyBulletSurface = IMG_Load("enemy_bullet.png"); // Replace "enemy_bullet.png" with the actual filename of your enemy bullet image
  if (!enemyBulletSurface)
  {
    printf("Unable to load enemy bullet image: %s\n", IMG_GetError());
    // Handle error
  }
  enemyBulletTexture = SDL_CreateTextureFromSurface(renderer, enemyBulletSurface);
  SDL_FreeSurface(enemyBulletSurface); // Free the surface as it's no longer needed
  if (!enemyBulletTexture)
  {
    printf("Unable to create texture from enemy bullet image: %s\n", SDL_GetError());
    // Handle error
  }

  for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
  {
    if (enemyBullets[i].active)
    {
      SDL_Rect bulletRect = {(int)enemyBullets[i].x, (int)enemyBullets[i].y, 8, 8}; // Adjust bullet width and height as needed
      SDL_RenderCopy(renderer, enemyBulletTexture, NULL, &bulletRect);
    }
  }
}

void updateEnemyBullets()
{
  for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
  {
    if (enemyBullets[i].active)
    {
      enemyBullets[i].x += enemyBullets[i].dx;
      enemyBullets[i].y += enemyBullets[i].dy;

      // Add boundary check or collision detection logic here
    }
  }
}

int processEvents(SDL_Window *window, Man *man)
{
  SDL_Event event;
  int done = 0;

  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
    case SDL_WINDOWEVENT_CLOSE:
    {
      if (window)
      {
        SDL_DestroyWindow(window);
        window = NULL;
        done = 1;
      }
    }
    break;
    case SDL_KEYDOWN:
    {
      switch (event.key.keysym.sym)
      {
      case SDLK_ESCAPE:
        done = 1;
        break;
      case SDLK_SPACE:                 // Check for the spacebar key
        Mix_PlayChannel(-1, shoot, 0); // Play the shoot sound
        break;
      }
    }
    break;
    case SDL_QUIT:
      // quit out of the game
      done = 1;
      break;
    }
  }

  const Uint8 *state = SDL_GetKeyboardState(NULL);
  if (!man->shooting)
  {
    if (state[SDL_SCANCODE_LEFT])
    {
      man->x -= 3;
      man->walking = 1;
      man->facingLeft = 1;

      if (globalTime % 6 == 0)
      {
        man->currentSprite++;
        man->currentSprite %= 4;
      }
    }
    else if (state[SDL_SCANCODE_RIGHT])
    {
      man->x += 3;
      man->walking = 1;
      man->facingLeft = 0;

      if (globalTime % 6 == 0)
      {
        man->currentSprite++;
        man->currentSprite %= 4;
      }
    }
    else
    {
      man->walking = 0;
      man->currentSprite = 4;
    }
  }

  if (!man->walking)
  {
    if (state[SDL_SCANCODE_SPACE]) // && !man->dy)
    {
      if (globalTime % 6 == 0)
      {
        if (man->currentSprite == 4)
          man->currentSprite = 5;
        else
          man->currentSprite = 4;

        if (!man->facingLeft)
        {
          addBullet(man->x + 35, man->y + 20, 3);
        }
        else
        {
          addBullet(man->x + 5, man->y + 20, -3);
        }
        Mix_PlayChannel(-1, shoot, 0);
      }

      man->shooting = 1;
    }
    else
    {
      man->currentSprite = 4;
      man->shooting = 0;
    }
  }

  if (state[SDL_SCANCODE_UP] && !man->dy)
  {
    man->dy = -8;
  }
  if (state[SDL_SCANCODE_DOWN])
  {
    //man->y += 10;
  }

  return done;
}

void doRender(SDL_Renderer *renderer, Man *man)
{
  // set the drawing color to blue
  SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

  // Clear the screen (to blue)
  SDL_RenderClear(renderer);

  // set the drawing color to white
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

  // SDL_RenderFillRect(renderer, &rect);
  SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);

  // warrior

  SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue color
  SDL_Rect playerHealthBar = {10, 10, (40 - totalDamageman) * 3, 10}; // Adjust size and position as needed
  SDL_RenderFillRect(renderer, &playerHealthBar);


  SDL_Rect srcRect = {40 * man->currentSprite, 0, 40, 50};
  SDL_Rect rect = {(int)man->x, (int)man->y, 40, 50};
  SDL_RenderCopyEx(renderer, man->sheetTexture, &srcRect, &rect, 0, NULL, man->facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);

  // enemy

  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color
  SDL_Rect enemyHealthBar = { 190, 10, (40 - totalDamageenemy) * 3, 10}; // Adjust size and position as needed
  SDL_RenderFillRect(renderer, &enemyHealthBar);

  SDL_Rect eSrcRect = {40 * enemy.currentSprite, 0, 40, 50};
  SDL_Rect eRect = {(int)enemy.x, (int)enemy.y, 40, 50};
  SDL_RenderCopyEx(renderer, enemy.sheetTexture, &eSrcRect, &eRect, 0, NULL, enemy.facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);

  // Render enemy bullets
  renderEnemyBullets(renderer);

  for (int i = 0; i < MAX_BULLETS; i++)
  {
    if (bullets[i])
    {
      SDL_Rect rect = {(int)bullets[i]->x, (int)bullets[i]->y, 8, 8};
      SDL_RenderCopy(renderer, bulletTexture, NULL, &rect);
    }
  }

  // We are done drawing, "present" or show to the screen what we've drawn
  SDL_RenderPresent(renderer);
}

void checkBulletPlayerCollision(Man *man)
{
  man->health = 20;
  for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
  {
    if (enemyBullets[i].active)
    {
      // Check if the bullet intersects with the player's position
      if (enemyBullets[i].x < man->x + 40 &&
          enemyBullets[i].x + 8 > man->x &&
          enemyBullets[i].y < man->y + 50 &&
          enemyBullets[i].y + 8 > man->y)
      {
        // Reduce player's health
        totalDamageman += 1; // Adjust damage as needed

        // Remove the bullet after hitting the player
        enemyBullets[i].active = 0;

        // Check if player is dead
        if (totalDamageman >= 40)
        {
          death = Mix_LoadMUS("Death.WAV");
          if (!death)
          {
            printf("Error loading Win music: %s\n", Mix_GetError());
          }
          man->alive = 0; // Player is dead
          Mix_PlayMusic(death, 1);
          SDL_DestroyTexture(man->sheetTexture);
          man->visibleTime = globalTime;
          // Perform any other actions for player death, such as showing game over screen, etc.
        }
      }
    }
  }
}

void updateLogic(Man *man)
{
  man->y += man->dy;
  man->dy += 0.5;
  if (man->y > 60)
  {
    man->y = 60;
    man->dy = 0;
  }

  for (int i = 0; i < MAX_BULLETS; i++)
  {
    if (bullets[i])
    {
      bullets[i]->x += bullets[i]->dx;

      // Simple collision detection
      if (bullets[i]->x > enemy.x && bullets[i]->x < enemy.x + 40 &&
          bullets[i]->y > enemy.y && bullets[i]->y < enemy.y + 50)
      {
        
         // Adjust damage as needed

        // Remove the bullet after hitting the enemy
        removeBullet(i);

        // Increment the shotsHit counter
        totalDamageenemy += 1;

        // Check if the enemy should become invisible
        if (totalDamageenemy >= 40)
        {
          death = Mix_LoadMUS("Death.WAV");
          if (!death)
          {
            printf("Error loading Win music: %s\n", Mix_GetError());
          }
          Mix_PlayMusic(death, 1);
          enemy.alive = 0;
          SDL_DestroyTexture(enemy.sheetTexture);
          enemy.visibleTime = globalTime; // Store the time when the enemy became invisible
        }

        // Break out of the loop after hitting the enemy
        break;
      }

      if (bullets[i]->x < -1000 || bullets[i]->x > 1000)
        removeBullet(i);
    }
  }

  if (enemy.alive == 0 && globalTime % 6 == 0)
  {
    if (enemy.currentSprite < 6)
      enemy.currentSprite = 6;
    else if (enemy.currentSprite >= 6)
    {
      enemy.currentSprite++;
      if (enemy.currentSprite > 7)
      {
        enemy.visible = 0;
        enemy.currentSprite = 7;
      }
    }
  }
  // Generate a random number between -1 and 1 for movement direction
  if (enemy.alive == 1)
  {
    // Random movement logic
    if (globalTime % 60 == 0)
    {
      int randomMove = rand() % 2; // Generate a random number (0 or 1)
      if (randomMove == 0)
      {
        enemy.currentSprite++;
        enemy.currentSprite %= 4;
        enemy.x -= 40;
      }

      // Move left
      else
      {
        enemy.currentSprite++;
        enemy.currentSprite %= 4;
        enemy.x += 40;
      }
      // Move right

      // Ensure the enemy stays within the screen's horizontal boundaries
      if (enemy.x <= 0)
        enemy.x = 0;
      else if (enemy.x >= 320 - 40)
        enemy.x = 320 - 40;
    }

    // Jump logic
    if (globalTime % 20 == 0)
    { // Adjust jump frequency as needed
      if (enemy.dy == 0)
      {                 // Only jump if not already jumping
        enemy.dy = -10; // Set the initial jump velocity
      }
    }

    // Update enemy's vertical position based on jump
    enemy.y += enemy.dy;
    enemy.dy += 0.4; // Apply gravity to the jump

    // Ensure the enemy stays within the screen's vertical boundaries
    if (enemy.y > 60)
    {
      enemy.y = 60;
      enemy.dy = 0; // Reset jump velocity when reaching the ground
    }
    
  }


  if (man->y < -10)
  {
    man->dy = 4;
  }

  if (globalTime % 6 == 0)
  {
    enemy.currentSprite++;
    enemy.currentSprite %= 4;
  }

  if (enemy.alive)
  {

    // Add a timer for enemy shooting
    static int enemyShootTimer = 0;
    enemyShootTimer++;

    // Check if it's time for the enemy to shoot
    if (enemyShootTimer >= 11) // Adjust the interval as needed (20 frames per second)
    {

      addEnemyBullet(enemy.x + 5, enemy.y + 20, -3, 0);
      enemyShootTimer = 0; // Reset the timer
    }

    // Other enemy behavior updates...
  }

  checkBulletPlayerCollision(man);

  globalTime++;
}

int main(int argc, char *argv[])
{
  SDL_Window *window; // Declare a window
  SDL_Renderer *renderer;
  // Declare a renderer

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  // INITIALIZING A MIXER              // Initialize SDL2
  Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
  {
    printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
    return 1;
  }

  // LOAD audio files
  shoot = Mix_LoadWAV("bullet1.mp3");
  if (!shoot)
  {
    printf("Error loading sound file: %s\n", Mix_GetError());
    return 1;
  }

  bgm = Mix_LoadMUS("contrabg.mp3");
  if (!bgm)
  {
    printf("Error loading background music: %s\n", Mix_GetError());
    return 1;
  }

  WinMusic = Mix_LoadMUS("gameover_win.mp3");
  if (!WinMusic)
  {
    printf("Error loading Win music: %s\n", Mix_GetError());
  }
  LossMusic = Mix_LoadMUS("gameover_loss.mp3");
  if (!LossMusic)
  {
    printf("Error loading Win music: %s\n", Mix_GetError());
  }

  SDL_Init(SDL_INIT_VIDEO); // Initialize SDL2

  Man man;
  man.x = 50;
  man.y = 0;
  man.currentSprite = 4;
  man.alive = 1;
  man.visible = 1;
  man.facingLeft = 0;
  man.health = 100;

  enemy.x = 250;
  enemy.y = 60;
  enemy.health = 5;
  enemy.currentSprite = 4;
  enemy.facingLeft = 1;
  enemy.alive = 1;
  enemy.visible = 1;

  // Create an application window with the following settings:
  window = SDL_CreateWindow("Game Window",           // window title
                            SDL_WINDOWPOS_UNDEFINED, // initial x position
                            SDL_WINDOWPOS_UNDEFINED, // initial y position
                            960,                     // width, in pixels
                            720,                     // height, in pixels
                            0                        // flags
  );
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  SDL_RenderSetLogicalSize(renderer, 320, 240);
  showStartScreen(renderer);
  Mix_PlayMusic(bgm, -1);

  SDL_Surface *sheet = IMG_Load("sheet.png");
  if (!sheet)
  {
    printf("Cannot find sheet\n");
    return 1;
  }

  man.sheetTexture = SDL_CreateTextureFromSurface(renderer, sheet);
  SDL_FreeSurface(sheet);

  // load enemy
  sheet = IMG_Load("badman_sheet.png");
  if (!sheet)
  {
    printf("Cannot find sheet\n");
    return 1;
  }

  enemy.sheetTexture = SDL_CreateTextureFromSurface(renderer, sheet);
  SDL_FreeSurface(sheet);

  // load the bg
  SDL_Surface *bg = IMG_Load("background.png");

  if (!sheet)
  {
    printf("Cannot find background\n");
    return 1;
  }

  backgroundTexture = SDL_CreateTextureFromSurface(renderer, bg);
  SDL_FreeSurface(bg);

  // load the bullet
  SDL_Surface *bullet = IMG_Load("bullet.png");

  if (!bullet)
  {
    printf("Cannot find bullet\n");
    return 1;
  }

  bulletTexture = SDL_CreateTextureFromSurface(renderer, bullet);
  SDL_FreeSurface(bullet);

  // The window is open: enter program loop (see SDL_PollEvent)
  int done = 0;

  // Event loop
  while (!done)
  {
    // Check for events
    done = processEvents(window, &man);

    // Update logic
    updateLogic(&man);

    // Render display
    doRender(renderer, &man);

    // Update enemy bullets
    updateEnemyBullets();

    if (enemy.alive == 0)
    {
      SDL_Delay(1000);
      Mix_PauseMusic();
      SDL_Delay(1000);
      Mix_PlayMusic(WinMusic, -1);

      showWinScreen(renderer);
    }

    if (man.alive == 0)
    {
      SDL_Delay(1000);
      Mix_PauseMusic();
      SDL_Delay(1000);
      Mix_PlayMusic(LossMusic, 1);

      showLossScreen(renderer);
    }

    // don't burn up the CPU
    SDL_Delay(10);
  }

  // Close and destroy the window
  Mix_FreeChunk(shoot);

  Mix_CloseAudio();
  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyTexture(man.sheetTexture);
  SDL_DestroyTexture(backgroundTexture);
  SDL_DestroyTexture(bulletTexture);
  SDL_DestroyTexture(enemy.sheetTexture);

  for (int i = 0; i < MAX_BULLETS; i++)
    removeBullet(i);

  // Clean up
  SDL_Quit();
  return 0;
}
