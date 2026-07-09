>[!IMPORTANT]
>
>On UPDATE of `style.css` or `index.html` (data), you have to build and upload Filesystem Image

Server is located on:
> 192.168.4.1

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