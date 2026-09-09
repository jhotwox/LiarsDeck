#include "GameManager.h"
#include "../utilities/UString.h"
#include "../network/WebSocketHandler.h"

GameManager game;

// TODO: Handle better error codes with or use enum for error codes
int GameManager::addPlayer(String name, uint32_t wsId) {
  if (isFull()) return -1;

  // Check for duplicate names (case-insensitive)
  if (playerCount > 0) {
    for (int i = 0; i < playerCount; i++)
      if (equalsIgnoreCase(players[i].name, name))
        return -1;
  }

  Player &p = players[playerCount];
  p.id = playerCount;
  p.name = name;
  p.connected = true;
  p.alive = true;
  p.wsId = wsId;
  p.isAdmin = (playerCount == 0);  // First player is admin
  p.handCount = 0;
  p.currentBullet = 0;
  p.avatar = -1; // Default to no avatar

  playerCount++;
  return p.id;
}

// Mark a player as disconnected based on their WebSocket ID (lost of connection)
void GameManager::disconnectPlayer(uint32_t wsId) {
  for (int i = 0; i < playerCount; i++)
    if (players[i].wsId == wsId)
      players[i].connected = false;
}

// Remove a player from the game based on their WebSocket ID (used when a player leaves the game voluntarily)
void GameManager::removePlayer(uint32_t wsId) {
  for (int i = 0; i < playerCount; i++) {
    if (players[i].wsId == wsId) {
      // Shift players down to fill the gap
      for (int j = i; j < playerCount - 1; j++) {
        players[j] = players[j + 1];
      }
      playerCount--;
      break;
    }
  }
}

int GameManager::findPlayerByName(String name) {
  for (int i = 0; i < playerCount; i++) {
    if (equalsIgnoreCase(players[i].name, name)) {
      return i;
    }
  }
  return -1;
}

bool GameManager::reconnectPlayer(String name, uint32_t wsId) {
  int playerIndex = findPlayerByName(name);
  if (playerIndex == -1) return false;
  
  players[playerIndex].wsId = wsId;
  players[playerIndex].connected = true;
  return true;
}

ResultCode GameManager::selectAvatarForPlayer(int playerIndex, int8_t avatarIndex) {
  if (playerIndex < 0 || playerIndex >= playerCount) return ResultCode::INVALID_PLAYER_INDEX;
  if (avatarIndex < 0 || avatarIndex > MAX_AVATARS) return ResultCode::INVALID_AVATAR_INDEX;

  // Check if the avatar is already taken by another player
  for (int i = 0; i < playerCount; i++) {
    if (i != playerIndex && players[i].avatar == avatarIndex) {
      return ResultCode::DUPLICATE_AVATAR; // Avatar already taken
    }
  }

  players[playerIndex].avatar = avatarIndex;
  return ResultCode::SUCCESS;
}

// Return array of available avatars (true if available, false if taken)
const bool* GameManager::getAvailableAvatars() {
  static bool availableAvatars[MAX_AVATARS];
  
  for (int i = 0; i < MAX_AVATARS; i++)
    availableAvatars[i] = true; // Assume all avatars are available

  for (int i = 0; i < game.playerCount; i++) {
    int avatarIndex = game.players[i].avatar;
    if (avatarIndex >= 0 && avatarIndex <= MAX_AVATARS)
      availableAvatars[avatarIndex - 1] = false; // Mark as taken
  }

  return availableAvatars;
}


bool GameManager::isFull() {
  return playerCount >= MAX_PLAYERS;
}

void GameManager::startGame() {
  state = START_ROUND;
  startRound();
}

void GameManager::cancelGame() {
  // Reset game state to lobby
  // TODO: Add CANCEL_GAME notification to clients, maybe is necessary CANCEL_GAME state to handle it in the front-end
  state = WAITING_PLAYERS;
  
  // TODO: Reindex connected players in players array to be able to add new players and delete disconnected players.

  // Reset all players to alive
  for (uint8_t i = 0; i < playerCount; i++) {
    players[i].alive = true;
    players[i].clearHand();
    players[i].currentBullet = 0;
  }
  
  // Reset deck and piles
  deckCount = 0;
  centerPileCount = 0;
  currentPlayerIndex = 0;
  lastPlayerIndex = 0;
}

void GameManager::startRound() {
  // 1. Initialize and shuffle deck
  initializeDeck();
  shuffleDeck();
  
  // 2. Choose table color (first card from deck)
  // FIXME: In this case, the first user will always have the advantage of knowing the table color before playing, maybe is better to choose a random color 
  tableColor = deck[0].color;
  // First card is used for the table, rest are dealt from index 1
  
  // 3. Deal cards to alive players
  dealCards();
  
  // 4. Initialize revolvers for alive players
  for (uint8_t i = 0; i < playerCount; i++) {
    if (players[i].alive && players[i].connected) {
      initializeRevolver(players[i]);
      shuffleRevolver(players[i]);
    }
  }
  
  // 5. Clear center pile
  centerPileCount = 0;
  
  // 6. Change state
  state = PLAYER_TURN;
  
  // 7. Notify WebSocket clients
  notifyRoundStarted();
  
  // 8. Send cards to each player (private message)
  for (uint8_t i = 0; i < playerCount; i++) {
    if (players[i].alive && players[i].connected) {
      notifyHandDealt(i);
    }
  }
  
  // 9. Notify initial turn
  notifyTurnChanged();

  // 10. Notify updated player list (to update card counts in the game area)
  notifyPlayerList();
}

// TODO: Make dynamic card deck length for number of players
// 🎲 Initialize 20-card deck
void GameManager::initializeDeck() {
  deckCount = 0;
  
  // 6 red cards
  for (uint8_t i = 0; i < 6; i++) {
    deck[deckCount++].color = RED;
  }
  
  // 6 blue cards
  for (uint8_t i = 0; i < 6; i++) {
    deck[deckCount++].color = BLUE;
  }
  
  // 6 green cards
  for (uint8_t i = 0; i < 6; i++) {
    deck[deckCount++].color = GREEN;
  }
  
  // 2 jokers
  deck[deckCount++].color = JOKER;
  deck[deckCount++].color = JOKER;
}

// 🔀 Shuffle deck using Fisher-Yates algorithm
void GameManager::shuffleDeck() {
  for (int i = deckCount - 1; i > 0; i--) {
    int j = random(i + 1);
    Card temp = deck[i];
    deck[i] = deck[j];
    deck[j] = temp;
  }
}

// 🎴 Deal 5 cards to each alive player
void GameManager::dealCards() {
  uint8_t cardIndex = 0;  // Start dealing from the top of the deck
  
  for (uint8_t i = 0; i < playerCount; i++) {
    if (players[i].alive && players[i].connected) {
      players[i].clearHand();
      
      for (uint8_t j = 0; j < HAND_SIZE && cardIndex < deckCount; j++) {
        players[i].addCard(deck[cardIndex++]);
      }
    }
  }
}

// 🔫 Initialize revolver (1 lethal + 5 blanks)
void GameManager::initializeRevolver(Player &player) {
  player.currentBullet = 0;
  
  // First bullet is lethal
  player.revolver[0] = LETHAL;
  
  // The other 5 are blanks
  for (uint8_t i = 1; i < REVOLVER_SIZE; i++) {
    player.revolver[i] = BLANK;
  }
}

// 🔀 Shuffle player's revolver
void GameManager::shuffleRevolver(Player &player) {
  for (int i = REVOLVER_SIZE - 1; i > 0; i--) {
    int j = random(i + 1);
    BulletType temp = player.revolver[i];
    player.revolver[i] = player.revolver[j];
    player.revolver[j] = temp;
  }
}

bool GameManager::canJoin() {
  return state == WAITING_PLAYERS && !isFull();
}