#include "WebSocketHandler.h"
#include "../game/GameManager.h"
#include <ArduinoJson.h>

AsyncWebSocket ws("/ws");

// 🎨 Helper: Convert CardColor to String
String colorToString(CardColor color) {
  switch (color) {
    case RED: return "RED";
    case BLUE: return "BLUE";
    case GREEN: return "GREEN";
    case JOKER: return "JOKER";
    default: return "UNKNOWN";
  }
}

void notifyPlayerList() {
  JsonDocument doc;

  doc["type"] = "PLAYER_LIST_UPDATE";
  JsonArray arr = doc["data"]["players"].to<JsonArray>();

  for (int i = 0; i < game.playerCount; i++) {
    JsonObject p = arr.add<JsonObject>();
    p["id"] = game.players[i].id;
    p["name"] = game.players[i].name;
    p["alive"] = game.players[i].alive;
  }

  String msg;
  serializeJson(doc, msg);
  Serial.println(msg);
  ws.textAll(msg);
}

void cleanPlayerListExceptAdmin() {
  if (game.playerCount == 0) return;

  Player admin = game.players[0];
  for (int i = 1; i < game.playerCount; i++) {
    game.players[i].connected = false;
    game.players[i].alive = false;
    game.players[i].handCount = 0;
    game.players[i].currentBullet = 0;
  }
  game.playerCount = 1;
  game.players[0] = admin;
  Serial.println("Player list cleaned, only admin remains");
}

void handleJoin(AsyncWebSocketClient *client, JsonVariant data) {
  if (!game.canJoin()) {
    JsonDocument errorDoc;
    errorDoc["type"] = "ERROR";
    errorDoc["data"] = "Lobby full";
    String errorMsg;
    serializeJson(errorDoc, errorMsg);
    client->text(errorMsg);
    return;
  }

  // Add player to the game
  String name = data["name"].as<String>();
  int id = game.addPlayer(name, client->id());

  // Send confirmation to the joining player
  JsonDocument doc;
  doc["type"] = "GAME_JOINED";
  doc["data"]["playerId"] = id;
  doc["data"]["isAdmin"] = (id == 0);

  String msg;
  serializeJson(doc, msg);
  client->text(msg);

  // Notify all players about the updated player list
  notifyPlayerList();
}

void handleStartGame(AsyncWebSocketClient *client) {
  // Only admin can start the game
  if (client->id() != game.players[0].wsId) return;

  game.startGame();

  JsonDocument doc;
  doc["type"] = "GAME_STARTED";

  String msg;
  serializeJson(doc, msg);

  ws.textAll(msg);
}

// Admin press cancel game button, notify all players and reset the game state to WAITING_PLAYERS
void handleCancelGame(AsyncWebSocketClient *client) {
  // Only admin can cancel the game
  if (client->id() != game.players[0].wsId) return;

  game.cancelGame();

  JsonDocument doc;
  doc["type"] = "GAME_CANCELLED";

  String msg;
  serializeJson(doc, msg);
  
  Serial.println("Game cancelled by admin");
  ws.textAll(msg);

  // Delete All users except admin
  cleanPlayerListExceptAdmin();
  
  // Notify updated player list
  notifyPlayerList();
}

// 🎮 Notify round started
void notifyRoundStarted() {
  JsonDocument doc;
  doc["type"] = "ROUND_STARTED";
  doc["data"]["tableColor"] = colorToString(game.tableColor);
  doc["data"]["firstPlayer"] = game.currentPlayerIndex;
  
  String msg;
  serializeJson(doc, msg);
  Serial.println(msg);
  ws.textAll(msg);
}

// 🎴 Notify cards dealt to a player (private message)
void notifyHandDealt(uint8_t playerId) {
  if (playerId >= game.playerCount) return;
  
  Player &player = game.players[playerId];
  if (!player.connected) return;
  
  JsonDocument doc;
  doc["type"] = "HAND_DEALT";
  JsonArray cardsArr = doc["data"]["cards"].to<JsonArray>();
  
  for (uint8_t i = 0; i < player.handCount; i++) {
    cardsArr.add(colorToString(player.handCards[i].color));
  }
  
  String msg;
  serializeJson(doc, msg);
  Serial.printf("Enviando mano a jugador %d: %s\n", playerId, msg.c_str());
  
  // Send only to this specific player
  ws.text(player.wsId, msg);
}

// 🎯 Notify turn changed
void notifyTurnChanged() {
  JsonDocument doc;
  doc["type"] = "TURN_CHANGED";
  doc["data"]["playerId"] = game.currentPlayerIndex;
  doc["data"]["playerName"] = game.players[game.currentPlayerIndex].name;
  
  String msg;
  serializeJson(doc, msg);
  Serial.println(msg);
  ws.textAll(msg);
}

// 🔄 Convert GameState enum to string
String getGameStateString() {
  switch (game.state) {
    case WAITING_PLAYERS: return "WAITING_PLAYERS";
    case START_ROUND: return "START_ROUND";
    case PLAYER_TURN: return "PLAYER_TURN";
    case RESOLVING_CHALLENGE: return "RESOLVING_CHALLENGE";
    case RUSSIAN_ROULETTE: return "RUSSIAN_ROULETTE";
    case ROUND_END: return "ROUND_END";
    case GAME_END: return "GAME_END";
    default: return "UNKNOWN";
  }
}

// 🔌 Handle player reconnection
void handleReconnect(AsyncWebSocketClient *client, JsonVariant data) {
  String name = data["name"].as<String>();
  
  if (game.reconnectPlayer(name, client->id())) {
    int playerId = game.findPlayerByName(name);
    
    JsonDocument doc;
    doc["type"] = "RECONNECTED";
    doc["data"]["playerId"] = playerId;
    doc["data"]["isAdmin"] = game.players[playerId].isAdmin;
    doc["data"]["gameState"] = getGameStateString();
    
    String msg;
    serializeJson(doc, msg);
    client->text(msg);
    
    Serial.printf("Player %s reconnected with ID %d\n", name.c_str(), playerId);
    
    notifyPlayerList();
  } else {
    // Player not found, treat as new join
    handleJoin(client, data);
  }
}

// 📊 Send current game state to a specific player
void handleGameStateRequest(AsyncWebSocketClient *client) {
  // Find player by wsId
  int playerId = -1;
  for (int i = 0; i < game.playerCount; i++) {
    if (game.players[i].wsId == client->id()) {
      playerId = i;
      break;
    }
  }
  
  if (playerId == -1) return;
  
  Player &player = game.players[playerId];
  
  JsonDocument doc;
  doc["type"] = "GAME_STATE";
  doc["data"]["tableColor"] = colorToString(game.tableColor);
  
  JsonArray cardsArr = doc["data"]["myCards"].to<JsonArray>();
  for (uint8_t i = 0; i < player.handCount; i++) {
    cardsArr.add(colorToString(player.handCards[i].color));
  }
  
  String msg;
  serializeJson(doc, msg);
  client->text(msg);
}

void onWsEvent(
  AsyncWebSocket *server,
  AsyncWebSocketClient *client,
  AwsEventType type,
  void *args,
  uint8_t *data,
  size_t len
) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("Client connected");
  }
  
  if (type == WS_EVT_DISCONNECT) {
    game.disconnectPlayer(client->id());
    notifyPlayerList();
  }
  
  if (type == WS_EVT_DATA) {
    JsonDocument doc;
    deserializeJson(doc, data);

    String msgType = doc["type"];

    if (msgType == "JOIN_GAME") {
      handleJoin(client, doc["data"]);
    }
    
    if (msgType == "RECONNECT") {
      handleReconnect(client, doc["data"]);
    }
    
    if (msgType == "REQUEST_GAME_STATE") {
      handleGameStateRequest(client);
    }

    if (msgType == "START_GAME") {
      handleStartGame(client);
    }
    
    if (msgType == "CANCEL_GAME") {
      handleCancelGame(client);
    }

    if (msgType == "LEAVE_GAME") {
      game.disconnectPlayer(client->id());
      notifyPlayerList();
    }
  }
}

void setupWebSocket(AsyncWebServer &server) {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
}