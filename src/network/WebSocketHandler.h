#pragma once
#include <ESPAsyncWebServer.h>

void setupWebSocket(AsyncWebServer &server);
void notifyPlayerList();
void notifyRoundStarted();
void notifyHandDealt(uint8_t playerId);
void notifyTurnChanged();
void handleReconnect(AsyncWebSocketClient *client, JsonVariant data);
void handleGameStateRequest(AsyncWebSocketClient *client);
void handleCancelGame(AsyncWebSocketClient *client);
String getGameStateString();