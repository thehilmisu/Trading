#include "operations.h"
#include "raylib.h"
#include <algorithm>

void moveCamera(Camera2D &camera) {

  static Vector2 lastMousePos = {0.0f, 0.0f};
  static bool isDragging = false;

  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    if (!isDragging) {
      isDragging = true;
      lastMousePos = GetMousePosition();
    }

    Vector2 mouseDelta = {GetMousePosition().x - lastMousePos.x,
                          GetMousePosition().y - lastMousePos.y};
    camera.target = {camera.target.x - mouseDelta.x,
                     camera.target.y - mouseDelta.y};
    lastMousePos = GetMousePosition();
  } else {
    isDragging = false;
  }

  if (GetMouseWheelMove() != 0) {
    camera.zoom += GetMouseWheelMove() * 0.05f;
    if (camera.zoom < 0.1f)
      camera.zoom = 0.1f;
    if (camera.zoom > 3.0f)
      camera.zoom = 3.0f;
  }
}

int main() {

  std::unique_ptr<Coinbase> coinbase = std::make_unique<Coinbase>();
  std::unique_ptr<Operations> operations = std::make_unique<Operations>();

  int granularity = 60;
  time_t start = std::time(nullptr) - (60 * 60); // 1 hour before
  time_t end = start + (granularity * 60);       // now

  // Initialization
  //--------------------------------------------------------------------------------------
  int screenWidth = 1280;
  int screenHeight = 720;
  float timer = 59.0f;

  std::vector<Coinbase::Candle> candles;
  std::vector<Result> result;
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(screenWidth, screenHeight, "Trading View");

  SetTargetFPS(60);
  SetWindowState(FLAG_WINDOW_RESIZABLE);

  Camera2D camera = {0};
  camera.target = (Vector2){0.0f, 0.0f};
  camera.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  static int buy_count = 0;
  static int sell_count = 0;
  static int success_count = 0;

  //--------------------------------------------------------------------------------------

  // Main loop
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {
    // Update
    //----------------------------------------------------------------------------------
    // TODO: Update your variables here
    //----------------------------------------------------------------------------------
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();

    moveCamera(camera);

    float deltaTime = GetFrameTime();
    timer += deltaTime;

    if (timer >= granularity) {
      timer = 0.0f;
      std::cout << "Fetching data.... " << std::endl;
      candles = coinbase->fetchCoinbaseData("BTC-USD", granularity, start, end);

      std::cout << "Candles size : " << candles.size() << std::endl;
      std::cout << "start time  : " << std::to_string(start) << "   "
                << operations->convertToTimestamp(start) << std::endl;
      std::cout << "end time  : " << std::to_string(end) << "   "
                << operations->convertToTimestamp(end) << std::endl;

      if (!candles.empty()) {

        std::reverse(candles.begin(), candles.end());

        double kama = operations->calculateKAMA(candles, 10);
        double rsi = operations->calculateRSI(candles, 14);
        MACDResult macd = operations->calculateMACD(candles);

        const Coinbase::Candle latestCandle = candles.back();
        static std::time_t lastFetchTime;
        if (lastFetchTime != latestCandle.timestamp) {

          std::string signal;

          if (macd.macdLine > macd.signalLine && rsi < 50 &&
              latestCandle.closingPrice > kama) {
            signal = "BUY";
            buy_count++;
          } else if (macd.macdLine < macd.signalLine && rsi > 50 &&
                     latestCandle.closingPrice < kama) {
            signal = "SELL";
            sell_count++;
          } else {
            signal = "HOLD";
          }

          Result res;
          res.timestamp = latestCandle.timestamp;
          res.macd = macd;
          res.price = latestCandle.closingPrice;
          res.kama = kama;
          res.rsi = rsi;
          res.signal = signal;
          res.normalized_timestamp = 60;

          result.push_back(res);
#if 0
          operations->writeAnalysisToFile("analysis.txt",
                                          operations->resultToString(res));
#endif
          lastFetchTime = latestCandle.timestamp;
        }
      }

      start = start + granularity;
      end = end + granularity;
      // std::this_thread::sleep_for(std::chrono::seconds(granularity));
      operations->normalizeData(result);
    }

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(BLACK);

    if (result.size() > 0) {
      float x_scale = (screenWidth / result.size());
      float y_scale = (screenHeight / 2.0f);
      Color color = RED;
      int fontsize = 12;
      static Vector2 prevPosition = {0.0f, 0.0f};

      BeginMode2D(camera);

      for (size_t i = 0; i < result.size(); i++) {

        float x =
            static_cast<float>(result.at(i).normalized_timestamp * (i + 2));
        float y = static_cast<float>(screenHeight -
                                     (result.at(i).normalized_price * y_scale));

        if (prevPosition.x != x && prevPosition.y != y) {
          if (i != 0) {
            DrawLineBezier(prevPosition, (Vector2){x, y}, 0.7f, WHITE);
          }
          prevPosition = (Vector2){x, y};
        }

        if (result.at(i).signal == "HOLD")
          color = RED;
        else if (result.at(i).signal == "BUY")
          color = GREEN;
        else
          color = BLUE;

        DrawCircleV({x, y}, 3, RED);
        DrawText(result.at(i).signal.c_str(), x + 3, y, fontsize, color);
        DrawText(std::to_string(result.at(i).price).c_str(), x + 3, y + 16,
                 fontsize, color);
        DrawText(std::to_string(result.at(i).normalized_price).c_str(), x + 3,
                 y + 32, fontsize, color);
        DrawText(std::to_string(i).c_str(), x + 3, y + 47, fontsize, color);
      }

      EndMode2D();

      DrawRectangle(0, screenHeight - 250, screenWidth, screenHeight - 250,
                    GRAY);

      char buffer[100];

      sprintf(buffer, "Total buy decision : %d", buy_count);
      DrawText(buffer, 50, screenHeight - 200, fontsize + 5, WHITE);
      sprintf(buffer, "Total sell decision : %d", sell_count);
      DrawText(buffer, 50, screenHeight - 180, fontsize + 5, WHITE);
      sprintf(buffer, "Total successfull decision : %d", success_count);
      DrawText(buffer, 50, screenHeight - 160, fontsize + 5, WHITE);
    }

    EndDrawing();
    //----------------------------------------------------------------------------------
  }
  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow(); // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
