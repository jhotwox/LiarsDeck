>[!IMPORTANT]
>
>On UPDATE of `style.css` or `index.html` (data), you have to build and upload Filesystem Image

Server is located on:
> 192.168.4.1

Listen to ESP32:
``` BASH

```

TODO:
- [] Link name with navigator ID or MAC adress
- [] Don't allow joker to be table color
- [] When game is over, reindex connected players in players array to be able to add new players and delete disconnected players. Reference:
``` C++
//GameManager.cpp
void GameManager::disconnectPlayer(uint32_t wsId) {
  for (int i = 0; i < playerCount; i++)
    if (players[i].wsId == wsId)
      players[i].connected = false;
}

bool GameManager::reconnectPlayer(String name, uint32_t wsId) {
  int playerIndex = findPlayerByName(name);
  if (playerIndex == -1) return false;
  
  players[playerIndex].wsId = wsId;
  players[playerIndex].connected = true;
  return true;
}
```
- [] Investigate and decide if we are gonna use a random color for tableColor or select one on the deck
- [] Handle admin disconnection (set admin to the next player)
Reference:
``` C++
// WebSocketHandler.cpp
void handleReconnect(AsyncWebSocketClient *client, JsonVariant data) {
  if (game.reconnectPlayer(name, client->id())) {
    int playerId = game.findPlayerByName(name);
    
    JsonDocument doc;
    doc["type"] = "RECONNECTED";
    doc["data"]["playerId"] = playerId;
    doc["data"]["isAdmin"] = game.players[playerId].isAdmin;
    doc["data"]["gameState"] = getGameStateString();
    
    client->text(msg);
    
    notifyPlayerList();
  } else {
    // Player not found, treat as new join
    handleJoin(client, data);
  }
}
```
- [x] Add agentContext
- [x] Add socket connection to postman
- [] Add player leave handler (if is his turn, change turn and set new player number, also be sure to always deal cards counting player number)

<!-- Design -->
- [x] Delete white border blur from bg
- [x] Align center player list
- [x] Design red(close/exit) button
- [x] Implement admin controls
- [x] Show connected players
- [x] Add avatar selector
- [x] In-game connected players
- [x] In-game avatar selector
- [] Fix translate-Y on button focus (pressed)
- [] Fix padding and height in player-list-container
