#ifndef OPERATIONS_H
#define OPERATIONS_H

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

struct Result {
  time_t timestamp;
  MACDResult macd;
  double price;
  double kama;
  double rsi;
  std::string signal;
};


class Operations {
  public:
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
  std::string resultToString(Result res){
       std::string result = convertToTimestamp(res.timestamp) +
            "\t MACD Line"  +  std::to_string(res.macd.macdLine) +
            "\t Signal Line" + std::to_string(res.macd.signalLine)+
            "\t Price: " + std::to_string(res.price) +
            "\t KAMA: " + std::to_string(res.kama) +
            "\t RSI: " + std::to_string(res.rsi) +
            "\t " + res.signal;

      return result;
  }


};

#endif // ! OPERATIONS_H
