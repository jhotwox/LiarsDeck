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

String ResultCodeToString(ResultCode code) {
  switch (code) {
    case ResultCode::SUCCESS: return "SUCCESS";
    case ResultCode::INVALID_PLAYER_INDEX: return "INVALID_PLAYER_INDEX";
    case ResultCode::INVALID_AVATAR_INDEX: return "INVALID_AVATAR_INDEX";
    case ResultCode::DUPLICATE_AVATAR: return "DUPLICATE_AVATAR";
    default: return "UNKNOWN_ERROR";
  }
}

void notifyPlayerList() {
  JsonDocument doc;
  doc["type"] = "PLAYER_LIST_UPDATE";
  JsonObject data = doc["data"].to<JsonObject>();
  JsonArray arr = data["players"].to<JsonArray>();
  doc["data"]["gameState"] = getGameStateString();

  for (int i = 0; i < game.playerCount || i < 4; i++) {
    JsonObject p = arr.add<JsonObject>();
    if (game.playerCount > i) {
      p["id"] = game.players[i].id;
      p["name"] = game.players[i].name;
      p["alive"] = game.players[i].alive;
      p["connected"] = game.players[i].connected;
      p["handCount"] = game.players[i].handCount;
      p["avatar"] = game.players[i].avatar;
    } else {
      p["id"] = i;
      p["name"] = "vacio";
      p["alive"] = true;
      p["connected"] = true;
      p["handCount"] = -1;
      p["avatar"] = -1;
    }
  }

  if (game.state == WAITING_PLAYERS) {
    const bool *availableAvatars = game.getAvailableAvatars();
    JsonArray avatarsArr = data["availableAvatars"].to<JsonArray>();
    for (int i = 0; i < MAX_AVATARS; i++) {
      avatarsArr.add(availableAvatars[i]);
    }
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
    game.players[i].avatar = -1;
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
  JsonArray arr = doc["data"]["availableAvatars"].to<JsonArray>();
  
  const bool *availableAvatars = game.getAvailableAvatars();
  for (int i = 0; i < MAX_AVATARS; i++)
    arr.add(availableAvatars[i]);
  
  String msg;
  serializeJson(doc, msg);
  client->text(msg);

  // Notify all players about the updated player list
  notifyPlayerList();
}

void handleStartGame(AsyncWebSocketClient *client) {
  // Only admin can start the game
  if (client->id() != game.players[0].wsId) return;

  JsonDocument doc;
  doc["type"] = "GAME_STARTED";

  String msg;
  serializeJson(doc, msg);

  ws.textAll(msg);

  game.startGame();
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
  JsonObject data = doc["data"].to<JsonObject>();
  data["tableColor"] = colorToString(game.tableColor);
  data["firstPlayer"] = (int)game.currentPlayerIndex;
  
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
  JsonObject data = doc["data"].to<JsonObject>();
  data["playerId"] = (int)game.currentPlayerIndex;
  data["playerName"] = game.players[game.currentPlayerIndex].name;
  
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
    doc["data"]["avatar"] = game.players[playerId].avatar;
    doc["data"]["gameState"] = getGameStateString();

    if (game.state == WAITING_PLAYERS) {
      JsonArray arr = doc["data"]["availableAvatars"].to<JsonArray>();
      const bool *availableAvatars = game.getAvailableAvatars();
      for (int i = 0; i < MAX_AVATARS; i++)
        arr.add(availableAvatars[i]);
    }
    
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
  JsonObject data = doc["data"].to<JsonObject>();
  data["tableColor"] = colorToString(game.tableColor);
  data["currentPlayerId"] = (int)game.currentPlayerIndex;
  data["currentPlayerName"] = game.players[game.currentPlayerIndex].name;
  
  JsonArray cardsArr = data["myCards"].to<JsonArray>();
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
      game.removePlayer(client->id());
      notifyPlayerList();
    }

    if (msgType == "SELECT_AVATAR") {
      int avatarId = doc["data"]["avatarId"];
      int userId = doc["data"]["myId"];
      
      if (userId != -1) {
        ResultCode result = game.selectAvatarForPlayer(userId, avatarId);
        if (result == ResultCode::SUCCESS) {
          notifyPlayerList();
        } else {
          // Handle error, e.g., send an error message back to the client
          JsonDocument errorDoc;
          errorDoc["type"] = "ERROR";
          errorDoc["data"] = "Failed to select avatar";
          errorDoc["Reason"] = ResultCodeToString(result);
          String errorMsg;
          serializeJson(errorDoc, errorMsg);
          client->text(errorMsg);
        }
      }
    }

    // array where index is the avatar index - 1 and value is false if taken, true if available
    if (msgType == "AVAILABLE_AVATARS") {
      JsonDocument doc;
      doc["type"] = "availableAvatarsResponse";
      JsonArray arr = doc["data"]["availableAvatars"].to<JsonArray>();
      
      // Collect all avatars that are currently taken
      // bool availableAvatars[MAX_AVATARS-1] = {true};
      // This is a dynamic array, so we need to free it later
      const bool *availableAvatars = game.getAvailableAvatars();
      
      // Fill the JSON array with available avatars
      for (int i = 0; i < MAX_AVATARS; i++)
        arr.add(availableAvatars[i]);
      
      String msg;
      serializeJson(doc, msg);
      client->text(msg);
    }
  }
}

void setupWebSocket(AsyncWebServer &server) {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
}