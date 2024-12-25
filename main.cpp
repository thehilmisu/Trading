#include "operations.h"
#include "raylib.h"
#include <algorithm>

Vector2 calculatePosition(Vector2 relativePosition) {
  return Vector2{relativePosition.x * GetScreenWidth(),
                 relativePosition.y * GetScreenHeight()};
}

void normalizeData(std::vector<Result> &prices) {

  std::vector<double> tmp;
  for (const auto &i : prices)
    tmp.push_back(i.price);

  auto element = std::minmax_element(tmp.begin(), tmp.end());
  auto min_price = *element.first;
  auto max_price = *element.second;
 

  for (size_t i = 0; i < prices.size(); i++) {
    if(min_price != max_price)
      prices[i].normalized_price = static_cast<double>((prices.at(i).price - min_price) / (max_price - min_price));
  }
}

void moveCamera(Camera2D &camera){

  if(IsKeyDown(KEY_RIGHT)){
    camera.target.x += 200.0f * GetFrameTime();
  }
  if(IsKeyDown(KEY_LEFT)){
    camera.target.x -= 200.0f * GetFrameTime();
  }
  if(IsKeyDown(KEY_UP)){
    camera.target.y += 200.0f * GetFrameTime();
  }
  if(IsKeyDown(KEY_DOWN)){
    camera.target.y -= 200.0f * GetFrameTime();
  }

  if(GetMouseWheelMove() != 0){
    camera.zoom += GetMouseWheelMove() * 0.05f;
    if(camera.zoom < 0.1f) camera.zoom = 0.1f;
    if(camera.zoom > 3.0f) camera.zoom = 3.0f;
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
  InitWindow(screenWidth, screenHeight, "Trading View");

  SetTargetFPS(60);
  SetWindowState(FLAG_WINDOW_RESIZABLE);


  Camera2D camera = {0};
  camera.target = (Vector2){0.0f, 0.0f};
  camera.offset = (Vector2){screenWidth/2.0f, screenHeight/2.0f};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

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
          res.normalized_timestamp = 60;

          result.push_back(res);
          operations->writeAnalysisToFile("analysis.txt",
                                          operations->resultToString(res));

          lastFetchTime = latestCandle.timestamp;
        }
      }

      start = start + granularity;
      end = end + granularity;
      // std::this_thread::sleep_for(std::chrono::seconds(granularity));
      normalizeData(result);
    }

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(BLACK);


    if(result.size() > 0){
        float x_scale = (screenWidth / result.size());
        float y_scale = (screenHeight / 2.0f);
        Color color = RED;
        int fontsize = 12;
        BeginMode2D(camera);
        for (size_t i = 0; i < result.size(); i++) {
          float x = static_cast<float>(result.at(i).normalized_timestamp * (i + 2));
          float y = static_cast<float>(screenHeight - (result.at(i).normalized_price * y_scale));
          DrawCircleV({x, y}, 3, RED);
          DrawLine(-1000, 100, 1000, 100, WHITE);
          if(result.at(i).signal == "HOLD"){
            color = RED;
          }else if(result.at(i).signal == "BUY"){
            color = GREEN;
          }else{
            color = BLUE;
          }
          DrawText(result.at(i).signal.c_str(), x+3, y, fontsize, color);
          DrawText(std::to_string(result.at(i).price).c_str(), x+3, y+16, fontsize, color);
          DrawText(std::to_string(result.at(i).normalized_price).c_str(), x+3, y+32, fontsize, color);
          DrawText(std::to_string(i).c_str(), x+3, y+47, fontsize, color);
        }
        EndMode2D();
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
