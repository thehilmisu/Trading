#include "operations.h"
#include "raylib.h"
#include <algorithm>

Vector2 calculatePosition(Vector2 relativePosition) {
  return Vector2{relativePosition.x * GetScreenWidth(),
                 relativePosition.y * GetScreenHeight()};
}

std::vector<double> normalizeData(const std::vector<int> &prices) {

  auto element = std::minmax_element(prices.begin(), prices.end());

  std::cout << *element.first << ", " << *element.second << std::endl;

  return {};
}

struct DataPoint {
  Vector2 normalizedPosition;
  float radius = 5;
};

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
  float timer = 0.0f;

  std::vector<Coinbase::Candle> candles;
  InitWindow(screenWidth, screenHeight, "Trading View");

  SetTargetFPS(60);
  SetWindowState(FLAG_WINDOW_RESIZABLE);
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
          } else if (macd.macdLine < macd.signalLine && rsi > 50 &&
                     latestCandle.closingPrice < kama) {
            signal = "SELL";
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

          operations->writeAnalysisToFile("analysis.txt",
                                          operations->resultToString(res));

          lastFetchTime = latestCandle.timestamp;
        }
      }

      start = start + granularity;
      end = end + granularity;
      // std::this_thread::sleep_for(std::chrono::seconds(granularity));
    }

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(BLACK);

    DataPoint p;
    p.normalizedPosition = {0.5, 0.5};
    p.radius = 5;
    // DrawText("Congrats! You created your first window!", 190, 200, 20,
    // BLACK);
    DrawCircleV(calculatePosition(p.normalizedPosition), p.radius, RED);
    ;

    EndDrawing();
    //----------------------------------------------------------------------------------
  }

  std::vector test = {1, 2, 4, 5, 6, 7, 8, 9, 22};

  normalizeData(test);

  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow(); // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
