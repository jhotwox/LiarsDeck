#pragma once
#include <Arduino.h>
#include "../Models.h"

class GameManager {
  public:
    Player players[MAX_PLAYERS];
    uint8_t playerCount = 0;
    GameState state = WAITING_PLAYERS;
    
    // 🎴 Main deck (20 cards)
    Card deck[DECK_SIZE];
    uint8_t deckCount = 0; // Current number of cards in the deck
    
    // 🟩 Current table card
    CardColor tableColor;
    
    // 📚 Center pile (played cards)
    Card centerPile[DECK_SIZE];
    uint8_t centerPileCount = 0;
    
    // 🎯 Turn control
    uint8_t currentPlayerIndex = 0;
    uint8_t lastPlayerIndex = 0;  // Last player who played cards
    
    // Player management methods
    bool isFull();
    bool canJoin();
    int addPlayer(String name, uint32_t wsId);
    void disconnectPlayer(uint32_t wsId);
    void removePlayer(uint32_t wsId);
    int findPlayerByName(String name);
    bool reconnectPlayer(String name, uint32_t wsId);
    ResultCode selectAvatarForPlayer(int playerIndex, int8_t avatarIndex);
    const bool* getAvailableAvatars();
    
    // Game control methods
    void startGame();
    void startRound();
    void cancelGame();
    
    // 🎲 Initialization methods
    void initializeDeck();
    void shuffleDeck();
    void dealCards();
    void initializeRevolver(Player &player);
    void shuffleRevolver(Player &player);
    
    // Helper to get current player
    Player& getCurrentPlayer() {
      return players[currentPlayerIndex];
    }
};

extern GameManager game;