# LyingCardsESP32 - Project Instructions & Agent Guidelines

## 🎯 Project Objective
The objective is to build a multiplayer card game called **"Liar's Deck"** (inspired by Liar's Bar) hosted entirely on an **ESP32**. 
- The ESP32 acts as the authoritative game server, hosting a WebSocket server and delivering the frontend web files.
- The game involves 2 to 4 players.
- Players try to be the last one alive. You play cards face down declaring they match the "Table Color". If someone calls "LIAR" and you were lying, you play Russian Roulette. If they were wrong, they play Russian Roulette.

## 🛠️ Technology Stack
- **Hardware/Environment**: ESP32, PlatformIO (C++).
- **Backend Libraries**: `ESPAsyncWebServer`, `AsyncTCP`, `ArduinoJson` (v7+).
- **Frontend**: Vanilla HTML, CSS, JavaScript (served from the `data/` directory).
- **Communication**: WebSockets (`ws://<ip>/ws`). The ESP32 pushes JSON events. Clients send JSON actions.

## 📖 Game Rules Summary
- **Deck**: 20 cards (6 Red, 6 Blue, 6 Green, 2 Jokers).
- **Revolver**: 6 chambers per player (1 Lethal 💀, 5 Blanks ⚪).
- **Turn Options**: 
  1. Play 1 to 3 cards face-down.
  2. Call "LIAR" on the previous player (only if not the first turn of the round).
- **Liar Resolution**:
  - If a lie is caught, the liar plays Russian Roulette.
  - If it was a false accusation (no lie), the accuser plays Russian Roulette.
- **Death**: Pulling the lethal bullet eliminates the player. The game ends when 1 player remains.

## 🏗️ Architecture & WebSocket Protocol
The ESP32 is the **Absolute Authority**. Clients only send actions and never compute game logic.
- **JSON Format**: `{"type": "EVENT_NAME", "data": { ... }}`
- **Client -> Server Actions**: `JOIN_GAME`, `START_GAME`, `PLAY_CARDS`, `CALL_LIAR`, `PING`.
- **Server -> Client Events**: `GAME_JOINED`, `PLAYER_LIST_UPDATE`, `GAME_STARTED`, `ROUND_STARTED`, `HAND_DEALT` (private), `TURN_CHANGED`, `CARDS_PLAYED`, `LIAR_CALLED`, `CARDS_REVEALED`, `ROULETTE_RESULT`, `PLAYER_ELIMINATED`, `ROUND_ENDED`, `GAME_OVER`, `GAME_STATE` (for reconnections).

## 🚀 Current Progress
- **Backend Setup**: PlatformIO project initialized with `GameManager` and `WebSocketHandler`.
- **Lobby & Connection**: Players can join the lobby, get an ID, and view connected players. The Admin (Player 0) can start or cancel the game.
- **Round Initialization**: The deck is shuffled, a table color is chosen, and 5 cards are dealt to each alive player.
- **Reconnection Logic**: Implemented and recently fixed. Players reloading the page correctly receive `RECONNECTED` and request a full `GAME_STATE` update to restore the UI (table card, current turn, player hand, opponents' card counts) without losing state.
- **JSON Robustness**: Adjusted `ArduinoJson` serialization to explicitly use `JsonObject` and cast `uint8_t` to `int` to prevent missing fields in WebSocket messages.
- **Frontend UI**:
  - Responsive design with mobile landscape orientation enforcement.
  - The game table renders dynamically. Other players are positioned around the table (top, left, right).
  - CSS transforms modified so player names/status text stay upright at 0 degrees, while their face-down cards rotate correctly towards the center of the table.
  - The player's own hand is rendered at the bottom, highlighting green when it is their turn.
  - Card selection logic allows picking up to 3 cards (with a visual shake if the limit is exceeded).

## 📝 Pending Implementation (Next Steps)
1. **Playing Cards (`PLAY_CARDS`)**: 
   - Add a "Play" button in the frontend when cards are selected.
   - Send `PLAY_CARDS` with card indices to the ESP32.
   - ESP32 must validate the play, move cards from hand to `centerPile`, update turn (`currentPlayerIndex`), and broadcast `CARDS_PLAYED` and `TURN_CHANGED`.
2. **Calling Liar (`CALL_LIAR`)**:
   - Add a "Call Liar" button for players (only visible if `lastPlayerIndex` exists).
   - ESP32 logic to reveal the last played cards, determine if it was a lie, and broadcast `LIAR_CALLED` and `CARDS_REVEALED`.
3. **Russian Roulette Logic (`ROULETTE_RESULT`)**:
   - Process the penalty: pop the next bullet for the penalized player.
   - If Lethal: `PLAYER_ELIMINATED`.
   - Clear table and start next round (`ROUND_ENDED`).
4. **Game Loop & Timers**:
   - Implement the 30-second turn timer (either frontend enforcement or backend tick).
   - Skip players with no cards. Require players with no cards (if they are the only ones left) to call Liar.
   - End game when 1 player is left (`GAME_OVER`).
