#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include "coinbase.h"
#include <memory>
#include <thread>
#include <fstream>
#include <algorithm>
#include "raylib.h"

struct MACDResult {
    double macdLine;
    double signalLine;
    double histogram;
};

double calculateSomething(){
  return 0.5f;
}
double calculateKAMA(const std::vector<Coinbase::Candle>& candles, size_t period = 10) {
    if (candles.size() < period) {
        throw std::invalid_argument("Not enough data to calculate KAMA.");
    }

    const double fastestSC = 2.0 / (2 + 1);  // Fastest smoothing constant
    const double slowestSC = 2.0 / (30 + 1); // Slowest smoothing constant

    double kama = candles[candles.size() - period].closingPrice;

    for (size_t i = candles.size() - period + 1; i < candles.size(); ++i) {
        double priceChange = std::abs(candles[i].closingPrice - candles[i - period].closingPrice);
        double volatility = 0.0;
        for (size_t j = 0; j < period; ++j) {
            volatility += std::abs(candles[i - j].closingPrice - candles[i - j - 1].closingPrice);
        }
        double er = (volatility == 0.0) ? 0.0 : priceChange / volatility;
        double smoothingConstant = std::pow(er * (fastestSC - slowestSC) + slowestSC, 2);

        kama += smoothingConstant * (candles[i].closingPrice - kama);
    }

    return kama;
}

double calculateRSI(const std::vector<Coinbase::Candle>& candles, size_t period = 14) {
    if (candles.size() < period + 1) {
        throw std::invalid_argument("Not enough data to calculate RSI.");
    }

    double gain = 0.0, loss = 0.0;

    for (size_t i = 1; i <= period; ++i) {
        double change = candles[i].closingPrice - candles[i - 1].closingPrice;
        if (change > 0) {
            gain += change;
        } else {
            loss -= change;
        }
    }
    gain /= period;
    loss /= period;

    for (size_t i = period + 1; i < candles.size(); ++i) {
        double change = candles[i].closingPrice - candles[i - 1].closingPrice;
        if (change > 0) {
            gain = (gain * (period - 1) + change) / period;
            loss = (loss * (period - 1)) / period;
        } else {
            gain = (gain * (period - 1)) / period;
            loss = (loss * (period - 1) - change) / period;
        }
    }

    double rs = (loss == 0.0) ? 100.0 : gain / loss;
    double rsi = 100.0 - (100.0 / (1.0 + rs));

    return rsi;
}


std::vector<double> calculateEMA(const std::vector<double>& prices, int period) {
    std::vector<double> ema(prices.size());
    double multiplier = 2.0 / (period + 1);

    ema[0] = prices[0]; // Initial EMA value
    for (size_t i = 1; i < prices.size(); ++i) {
        ema[i] = (prices[i] - ema[i - 1]) * multiplier + ema[i - 1];
    }

    return ema;
}

MACDResult calculateMACD(const std::vector<Coinbase::Candle>& candles, int shortPeriod = 12, int longPeriod = 26, int signalPeriod = 9) {

    std::vector<double> prices;
    for(const auto& candle : candles)
        prices.push_back(candle.closingPrice);

    std::vector<double> shortEMA = calculateEMA(prices, shortPeriod);
    std::vector<double> longEMA = calculateEMA(prices, longPeriod);

    std::vector<double> macdLine(prices.size());
    for (size_t i = 0; i < prices.size(); ++i) {
        macdLine[i] = shortEMA[i] - longEMA[i];
    }

    std::vector<double> signalLine = calculateEMA(macdLine, signalPeriod);
    double histogram = macdLine.back() - signalLine.back();

    return {macdLine.back(), signalLine.back(), histogram};
}

void writeAnalysisToFile(const std::string& fileName, const std::string& data) {
    std::ofstream file(fileName, std::ios_base::app); // Open file in append mode
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open file for writing.");
    }
    file << data << std::endl; // Write the analysis data followed by a newline
    file.close();
}

std::string convertToTimestamp(time_t unixtime){
    std::tm *timeStruct = std::localtime(&unixtime);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeStruct);
    return std::string(buffer);
}


void eventloop(){

    std::unique_ptr<Coinbase> coinbase = std::make_unique<Coinbase>();

    int granularity = 60;
    time_t start = std::time(nullptr) - ( 60 * 60); // 1 hour before
    time_t end = start + ( granularity * 60  ); // now

    while(true){

        try{
            std::cout << "Fetching data.... " << std::endl;
            std::vector<Coinbase::Candle> candles = coinbase->fetchCoinbaseData("BTC-USD", granularity, start, end);

            std::cout << "Candles size : " << candles.size() << std::endl;
            std::cout << "start time  : " << std::to_string(start) << "   " << convertToTimestamp(start) << std::endl;
            std::cout << "end time  : " << std::to_string(end) << "   " << convertToTimestamp(end) << std::endl;

            if(!candles.empty()){

                std::cout << "Data fetched : " << candles.size() << std::endl;

                std::reverse(candles.begin(), candles.end());

                double kama = calculateKAMA(candles, 10);
                double rsi = calculateRSI(candles, 14);
                MACDResult macd = calculateMACD(candles);

                const Coinbase::Candle latestCandle = candles.back();
                static std::time_t lastFetchTime;
                if(lastFetchTime != latestCandle.timestamp){

                    std::string signal;

                    if (macd.macdLine > macd.signalLine && rsi < 50 && latestCandle.closingPrice> kama) {
                        signal =  "BUY";
                    } else if (macd.macdLine < macd.signalLine && rsi > 50 && latestCandle.closingPrice < kama) {
                        signal = "SELL";
                    }else{
                        signal = "HOLD";
                    }

                    //if(kama > latestCandle.closingPrice && rsi < 30){
                    //    signal = "BUY";
                    //}else if (kama < latestCandle.closingPrice && rsi > 70) {
                    //    signal = "SELL";
                    //} else {
                    //    signal = "HOLD";
                    //}

                    // Print results with timestamp
                    std::cout << "Timestamp: " << convertToTimestamp(latestCandle.timestamp)
                        << "\t MACD Line" << macd.macdLine
                        << "\t Signal Line" << macd.signalLine
                        << "\t Price: " << latestCandle.closingPrice
                        << "\t KAMA: " << kama
                        << "\t RSI: " << rsi
                        << "\t " << signal << std::endl;

                    std::string result = convertToTimestamp(latestCandle.timestamp) +
                        "\t MACD Line"  +  std::to_string(macd.macdLine) +
                        "\t Signal Line" + std::to_string(macd.signalLine)+
                        "\t Price: " + std::to_string(latestCandle.closingPrice) +
                        "\t KAMA: " + std::to_string(kama) +
                        "\t RSI: " + std::to_string(rsi) +
                        "\t " + signal;

                    writeAnalysisToFile("analysis.txt", result);

                    lastFetchTime = latestCandle.timestamp;
                }

            }

            start = start + granularity;
            end = end + granularity;
            std::this_thread::sleep_for(std::chrono::seconds(granularity));

        }catch(const std::exception& e){
            std::cerr << "Error: " << e.what() << std::endl;
        }

    }

}

void draw(){
    
    
}

Vector2 calculatePosition(Vector2 relativePosition){
    return Vector2{relativePosition.x * GetScreenWidth(), relativePosition.y * GetScreenHeight()};
}

struct DataPoint{
    Vector2 normalizedPosition;
    float radius = 5;
};

int main() {

    // Initialization
    //--------------------------------------------------------------------------------------
    int screenWidth = 1280;
    int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Trading View");

    SetTargetFPS(60);               
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    //--------------------------------------------------------------------------------------

    // Main loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update your variables here
        //----------------------------------------------------------------------------------
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(BLACK);
           
            for(int x=0; x<=screenWidth; x+=screenWidth/80){
                DrawLine(x, 0, x, screenHeight,RED);
                for(int y=0;y<=screenHeight;y+=screenHeight/80){
                    DrawLine(0, y, screenWidth, y,RED);
                }
            }

            DataPoint p;
            p.normalizedPosition = {0.5, 0.5};
            p.radius = 5;
            //DrawText("Congrats! You created your first window!", 190, 200, 20, BLACK);
            DrawCircleV(calculatePosition(p.normalizedPosition),p.radius, RED);;

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

