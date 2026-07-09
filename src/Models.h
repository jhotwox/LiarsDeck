#pragma once

#define MAX_PLAYERS 4
#define HAND_SIZE 5
#define REVOLVER_SIZE 6
#define MAX_CARDS_PER_TURN 3
#define DECK_SIZE 20 // MAX_PLAYERS * HAND_SIZE + 1 (table card maybe)

enum CardColor {
  RED, // 6 available
  BLUE, // 6 available
  GREEN, // 6 available
  JOKER // 2 available
};

enum BulletType {
  BLANK,   // ⚪ 5 for player
  LETHAL   // 💀 1 for player
};

// 🔄 Global states
enum GameState {
  WAITING_PLAYERS,      // Waiting player in lobby
  START_ROUND,          // Initializing round
  PLAYER_TURN,          // Turn of a player
  RESOLVING_CHALLENGE,  // Someone told LIAR
  RUSSIAN_ROULETTE,     // Player shutting revolver
  ROUND_END,            // End of the round
  GAME_END              // Game finished (there is a winnner)
};

// 👤 Individual states for the player
enum PlayerState {
  CONNECTED, // Player is connected to the game
  ALIVE, // Player is alive in the game
  ELIMINATED, // Player is eliminated from the game
  OUT_OF_CARDS, // Player has no cards left
  WAITING_TURN, // Player is waiting for their turn
  PLAYING // Player is currently playing their turn
};

struct Card {
  CardColor color;
  
  // Helper to  know if is valid, accordesly the table color
  bool isValid(CardColor tableColor) {
    return (color == tableColor || color == JOKER);
  }
};

struct Player {
  uint8_t id;
  String name;
  bool connected;
  bool alive;
  uint32_t wsId;
  bool isAdmin;
  
  Card handCards[HAND_SIZE]; //5
  uint8_t handCount = 0;
  
  BulletType revolver[REVOLVER_SIZE]; //6
  uint8_t currentBullet = 0;  // Index for the next bullet
  
  // Helper methods
  
  // Add a card to the player's hand
  void addCard(Card card) {
    if (handCount < HAND_SIZE) {
      handCards[handCount++] = card;
    }
  }
  
  void removeCard(uint8_t index) {
    if (index < handCount) {
      for (uint8_t i = index; i < handCount - 1; i++) {
        handCards[i] = handCards[i + 1];
      }
      handCount--;
    }
  }
  
  void clearHand() {
    handCount = 0;
  }
  
  bool hasCards() {
    return handCount > 0;
  }
};
