#ifndef GAME_H
#define GAME_H

#include <Arduboy.h>

/* Bitmaps */
#include "bitmaps/tank.h"
#include "bitmaps/bat.h"
#include "bitmaps/pew.h"
#include "bitmaps/boom.h"
#include "bitmaps/dracula.h"
#include "bitmaps/megadracula.h"

const uint8_t frameRate = FRAMERATE;
const uint8_t screenWidth = SCREEN_WIDTH;
const uint8_t screenHeight = SCREEN_HEIGHT;

uint16_t score = 0;

struct t_boundingBox {
  uint8_t topLeftXOffset;
  uint8_t topLeftYOffset;
  uint8_t bottomRightXOffset;
  uint8_t bottomRightYOffset;
};

t_boundingBox tankBoundingBox = {
  0, 0,
  8, 7
};

struct t_tank {
  uint8_t X;
  uint8_t Y;
  uint8_t idleAnimationFrame;
  uint8_t deathAnimationFrame;
  const t_boundingBox *boundingBox;
  uint8_t spriteSizePx;
  boolean isDead;
};

t_tank player;

const uint8_t shootCooldown = 30;
uint8_t currentShotCooldown = 0;

const uint8_t maxEnemies = 20;
const uint8_t numBats = 3;

const uint16_t spawnRate = 6 * frameRate;
const uint8_t pointsPerWave = 20;

const int maxPaths = 8;
const int pathLength = 4;
int8_t paths[][pathLength][2] = {
  {{-1, -1}, {-2, 0}, {-1, 1}, {-1, 0}},
  {{-1, -1}, {0, 0}, {-1, -1}, {-2, 0}},
  {{0, 2}, {-1, 1}, {-1, 2}, {-1, 0}},
  {{-1, 0}, {0, 0}, {-3, 0}, {0, 0}},
  {{-1, -2}, {0, -1}, {-1, -1}, {-2, 0}},
  {{0, -2}, {-1, 0}, {0, -2}, {-2, 0}},
  {{-1, 1}, {-1, 1}, {-1, 1}, {-1, 1}},
  {{1, -1}, {2, 0}, {3, 0}, {1, 1}},
};

struct t_spaceBat {
  uint8_t X;
  uint8_t Y;
  boolean isActive;
  uint8_t idleAnimationFrame;
  uint8_t hitAnimationFrame;
  const t_boundingBox *boundingBox;
  uint8_t spriteSizePx;
  uint8_t currentPathIndex;
  uint8_t currentPathStepIndex;
  uint8_t health;
};

const t_boundingBox regularBatBoundingBox = {
  1, 2,
  6, 4
};

const t_boundingBox draculaBoundingBox = {
  0, 0,
  15, 15
};

const t_boundingBox megadraculaBoundingBox = {
  1, 1,
  30, 30
};

t_spaceBat spaceBats[maxEnemies];

t_spaceBat *currentHitSpaceBat;

boolean megadraculaLives;
uint16_t nextDraculaAtScore;
uint16_t nextMegadraculaAtScore;

void spawnEnemy(t_spaceBat *enemy) {
  enemy->isActive = true;
  enemy->X = rand() % 48 + 64;
  enemy->Y = rand() % (64 - enemy->spriteSizePx);
  enemy->currentPathIndex = rand() % maxPaths;
  enemy->currentPathStepIndex = rand() % pathLength;
}

void spawnBat(t_spaceBat *spaceBat) {
  spawnEnemy(spaceBat);
  spaceBat->spriteSizePx = 8;
  spaceBat->boundingBox = &regularBatBoundingBox;
  spaceBat->health = 1;
}

void spawnDracula(t_spaceBat *dracula) {
  spawnEnemy(dracula);
  dracula->spriteSizePx = 16;
  dracula->boundingBox = &draculaBoundingBox;
  dracula->health = 3;
}

void spawnMegaDracular(t_spaceBat *megadracula) {
  spawnEnemy(megadracula);
  megadracula->spriteSizePx = 32;
  megadracula->boundingBox = &megadraculaBoundingBox;
  megadracula->health = 10;

  megadracula->X = 72;
  megadracula->Y = 16;
}

void initEnemies() {
  megadraculaLives = false;
  nextDraculaAtScore = 20;
  nextMegadraculaAtScore = 40;

  for (int i = 0; i < maxEnemies; i++) {
    spaceBats[i] = {0, 0, false, i % 3, 0, &regularBatBoundingBox, 8, 0, 0, 1};
  }
  for (int i = 0; i < numBats; i++) {
    spawnBat(&spaceBats[i]);
  }
}

void initPlayer() {
  player = {16, 48, 0, 0, &tankBoundingBox, 8, false};
  score = 0;
}

void initGame() {
  arduboy.audio.on();
  initEnemies();
  initPlayer();
}

boolean isPointInBox(uint8_t pointX, uint8_t pointY, uint8_t spriteX, uint8_t spriteY, const t_boundingBox *boundingBox) {
  uint8_t leftX = spriteX + boundingBox->topLeftXOffset;
  uint8_t topY = spriteY + boundingBox->topLeftYOffset;

  uint8_t rightX = spriteX + boundingBox->bottomRightXOffset;
  uint8_t bottomY = spriteY + boundingBox->bottomRightYOffset;

  return pointX >= leftX && pointX <= rightX &&
         pointY >= topY && pointY <= bottomY;
}

boolean hasBoundingBoxOverlap(t_spaceBat *spaceBat) {
  return isPointInBox(player.X + player.boundingBox->topLeftXOffset, player.Y + player.boundingBox->topLeftYOffset, spaceBat->X, spaceBat->Y, spaceBat->boundingBox) ||
         isPointInBox(player.X + player.boundingBox->bottomRightXOffset, player.Y + player.boundingBox->topLeftYOffset, spaceBat->X, spaceBat->Y, spaceBat->boundingBox) ||
         isPointInBox(player.X + player.boundingBox->topLeftXOffset, player.Y + player.boundingBox->bottomRightYOffset, spaceBat->X, spaceBat->Y, spaceBat->boundingBox) ||
         isPointInBox(player.X + player.boundingBox->bottomRightXOffset, player.Y + player.boundingBox->bottomRightYOffset, spaceBat->X, spaceBat->Y, spaceBat->boundingBox);
}

void calculatePlayerCollision() {
  for (int i = 0; i < maxEnemies; i++) {
    t_spaceBat *spaceBat = &spaceBats[i];
    if (!spaceBat->isActive) { continue; }

    if (!player.deathAnimationFrame && hasBoundingBoxOverlap(spaceBat)) {
      player.deathAnimationFrame = 2 * frameRate;
      break;
    }
  }
}

void handleInput() {
  if (player.deathAnimationFrame) { return; }

  if (arduboy.pressed(UP_BUTTON)) {
      arduboy.setCursor(62, 4);

      if (player.Y > 0) {
        player.Y--;
      }
  }

  if (arduboy.pressed(DOWN_BUTTON)) {
      arduboy.setCursor(62, 52);

      if (player.Y < screenHeight - player.spriteSizePx) {
        player.Y++;
      }
  }

  if (arduboy.pressed(LEFT_BUTTON)) {
      arduboy.setCursor(30, 30);

      if (player.X > 0) {
        player.X--;
      }
  }

  if (arduboy.pressed(RIGHT_BUTTON)) {
      arduboy.setCursor(92, 30);

      if (player.X < screenWidth - player.spriteSizePx) {
        player.X++;
      }
  }

  if (arduboy.pressed(A_BUTTON) || arduboy.pressed(B_BUTTON)) {
      if (currentShotCooldown == 0 ) {
        currentShotCooldown = shootCooldown;
      }
  }
}

void drawScoreAndSevenYearsAgo() {
  arduboy.setCursor(100, 1);
  char scoreBuffer[16];
  sprintf(scoreBuffer, "% 3d", score);
  arduboy.print(scoreBuffer);
}

void drawTank() {
  if (!player.deathAnimationFrame || player.deathAnimationFrame % 2 == 0) {
    arduboy.drawBitmap(player.X, player.Y, tankFrames[player.idleAnimationFrame], player.spriteSizePx, player.spriteSizePx, WHITE);
  }

  if (arduboy.everyXFrames(16)) {
    player.idleAnimationFrame++;
    player.idleAnimationFrame = player.idleAnimationFrame % 3;
  }

  if (player.deathAnimationFrame) {
    player.deathAnimationFrame--;
    if (!player.deathAnimationFrame) {
      player.isDead = true;
    }
  }
}


void drawBats() {
  for (int i = 0; i < maxEnemies; i++) {
    t_spaceBat *spaceBat = &spaceBats[i];
    if (!spaceBat->isActive) { continue; }

    if (arduboy.everyXFrames(24)) {
      spaceBat->idleAnimationFrame++;
      spaceBat->idleAnimationFrame = spaceBat->idleAnimationFrame % 2;
    }

    if (spaceBat->isActive) {
      if (spaceBat->spriteSizePx == 32) {
        arduboy.drawBitmap(spaceBat->X, spaceBat->Y, megadraculaFrames[spaceBat->idleAnimationFrame], spaceBat->spriteSizePx, spaceBat->spriteSizePx, WHITE);
      } else if (spaceBat->spriteSizePx == 16) {
        arduboy.drawBitmap(spaceBat->X, spaceBat->Y, draculaFrames[spaceBat->idleAnimationFrame], spaceBat->spriteSizePx, spaceBat->spriteSizePx, WHITE);
      } else {
        arduboy.drawBitmap(spaceBat->X, spaceBat->Y, batFrames[spaceBat->idleAnimationFrame], spaceBat->spriteSizePx, spaceBat->spriteSizePx, WHITE);
      }
    }

    // draw explosion over hit bat
    if (currentHitSpaceBat && currentHitSpaceBat->hitAnimationFrame > 0) {
      arduboy.drawBitmap(
          currentHitSpaceBat->X - 4,
          currentHitSpaceBat->Y - 4,
          boomFrames[5 - currentHitSpaceBat->hitAnimationFrame],
          16, 16, WHITE
      );

      currentHitSpaceBat->hitAnimationFrame--;

      if (currentHitSpaceBat->hitAnimationFrame == 0) {
        if (currentHitSpaceBat->health == 0) {
          currentHitSpaceBat->isActive = false;
          if (megadraculaLives) {
            megadraculaLives = false;
            nextMegadraculaAtScore *= 2;
          }
        }
      }
    }
  }
}

boolean hasLaserHit(uint8_t laserOriginX, uint8_t y, t_spaceBat *spaceBat) {
  return laserOriginX < spaceBat->X + spaceBat->boundingBox->topLeftXOffset &&
      y >= spaceBat->Y + spaceBat->boundingBox->topLeftYOffset &&
      y <= spaceBat->Y + spaceBat->boundingBox->bottomRightYOffset;
}

void drawShootyShootyBoom() {
  if (currentShotCooldown > 0) {
    if (currentShotCooldown == shootCooldown) {
      currentHitSpaceBat = NULL;
    }

    if (currentShotCooldown == 20) {
      arduboy.setRGBled(0, 0, 0);
    }

    if (currentShotCooldown > 20) {
      arduboy.setRGBled(25 * (shootCooldown - 20), 0, 0);

      int laserY = player.Y + 3;
      int laserWidth = screenWidth - (player.X + player.spriteSizePx);

      if (!currentHitSpaceBat) {
        for (int i = 0; i < maxEnemies; i++) {
          t_spaceBat *spaceBat = &spaceBats[i];
          if (!spaceBat->isActive) { continue; }

          if (hasLaserHit(player.X + player.spriteSizePx, laserY, spaceBat)) {

            if (!currentHitSpaceBat || spaceBat->X < currentHitSpaceBat->X) {
              currentHitSpaceBat = spaceBat;
            }
          }
        }

        if (currentHitSpaceBat) {
          laserWidth = currentHitSpaceBat->X - (player.X + player.spriteSizePx) + 2;

          currentHitSpaceBat->hitAnimationFrame = 5;
          score++;
          arduboy.tunes.tone(300, 50);
          if (currentHitSpaceBat->health > 0) { currentHitSpaceBat->health--; }
        }
      } else {
        laserWidth = currentHitSpaceBat->X - (player.X + player.spriteSizePx) + 2;
      }

      arduboy.drawFastHLine(player.X + player.spriteSizePx, laserY, laserWidth, WHITE);
      if (currentShotCooldown > 21) {
        arduboy.drawBitmap(player.X + player.spriteSizePx, player.Y, pewFrames[(30 - currentShotCooldown) / 3], player.spriteSizePx, player.spriteSizePx, WHITE);
      }
    }

    currentShotCooldown--;
  }
}

void advanceEnemies() {
  if (player.deathAnimationFrame) { return; }

  for (int i = 0; i < maxEnemies; i++) {
    t_spaceBat *spaceBat = &spaceBats[i];

    if (arduboy.everyXFrames(10)) {
      if (spaceBat->isActive) {
        spaceBat->X += paths[spaceBat->currentPathIndex][spaceBat->currentPathStepIndex][0];
        spaceBat->Y += paths[spaceBat->currentPathIndex][spaceBat->currentPathStepIndex][1];
      }
    }

    if (arduboy.everyXFrames(FRAMERATE)) {
      if (spaceBat->isActive) {
        spaceBat->currentPathStepIndex++;
        if (spaceBat->currentPathStepIndex > pathLength) { spaceBat->currentPathStepIndex = 0; }
      }
    }

    if (spaceBat->X == 0) { spaceBat->X = screenWidth - spaceBat->spriteSizePx; }
    if (spaceBat->X >= screenWidth) { spaceBat->X = 0 + 1; }
    if (spaceBat->Y == 0) { spaceBat->Y = screenHeight - spaceBat->spriteSizePx; }
    if (spaceBat->Y >= screenHeight) { spaceBat->Y = 0 + 1; }
  }
}

void sweepAndSpawn() {
  if (!player.deathAnimationFrame && arduboy.everyXFrames(spawnRate)) {
      if (megadraculaLives) { return; }

      uint8_t spawnedThisWave = 0;

      uint8_t maxToSpawn = score / pointsPerWave;
      uint8_t probabilityOfDracular = score / pointsPerWave;

      for (int i = 0; i < maxEnemies; i++) {
        t_spaceBat *spaceBat = &spaceBats[i];
        if (!spaceBat->isActive && spawnedThisWave <= maxToSpawn) {
          if (score >= nextMegadraculaAtScore && !megadraculaLives) {
            for (int j = 0; j < maxEnemies; j++) {
              spaceBats[j].isActive = false;
            }
            spawnMegaDracular(spaceBat);
            megadraculaLives = true;
            break;
          } else if (rand() % 50 <= probabilityOfDracular) {
            nextDraculaAtScore = score;
            spawnDracula(spaceBat);
          } else {
            spawnBat(spaceBat);
          }

          spawnedThisWave++;
        }
      }
  }
}

byte movingStar[11][2] = {{84, 9}, {67, 14}, {109, 19}, {27, 24}, {9, 29}, {96, 34}, {31, 39}, {45, 44}, {82, 49}, {19, 54}, {57, 59}};
byte fastMovingStar[11][2] = {{16, 10}, {125, 15}, {62, 20}, {79, 25}, {115, 30}, {104, 35}, {16, 40}, {117, 45}, {65, 50}, {98, 55}, {23, 60}};
byte stillStar[11][2] = {{84, 7}, {67, 12}, {109, 17}, {27, 22}, {9, 27}, {96, 32}, {31, 37}, {45, 42}, {82, 47}, {19, 52}, {57, 57}};

void drawStarField()
{
  for (byte i = 0; i < 11; i++)
  {
    if (arduboy.everyXFrames(1))
    {
      fastMovingStar[i][0]--;
      if (fastMovingStar[i][0] < 1) fastMovingStar[i][0] = 127;
      arduboy.drawPixel(fastMovingStar[i][0], fastMovingStar[i][1], WHITE);
    }

    if (arduboy.everyXFrames(2))
    {
      movingStar[i][0]--;
      if (movingStar[i][0] < 1) movingStar[i][0] = 127;
      arduboy.drawPixel(movingStar[i][0], movingStar[i][1], WHITE);
    }

    if (arduboy.everyXFrames(3))
    {
      stillStar[i][0]--;
      if (stillStar[i][0] < 1) stillStar[i][0] = 127;
      arduboy.drawPixel(stillStar[i][0], stillStar[i][1], WHITE);
    }

  }
}

#endif
