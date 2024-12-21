#ifndef COINBASE_H
#define COINBASE_H

#include <curl/curl.h>
#include <iostream>
#include <json/json.h>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>

class Coinbase {

private: // Callback to store API response
  static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                              void *userp) {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
  }

public:
  struct Candle {
      std::time_t timestamp;
      double closingPrice;
  };
  std::vector<Candle> fetchCoinbaseData(const std::string &product_id, int granularity, time_t start, time_t end) {
  std::string url = "https://api.exchange.coinbase.com/products/" +
                      product_id +"/candles?granularity=" + std::to_string(granularity)+"&start="+std::to_string(start)+"&end="+std::to_string(end);

  std::cout << "API URL : " << url << std::endl;

  CURL *curl;
  CURLcode res;
  std::string readBuffer;

  curl = curl_easy_init();
  if(curl) {
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
      struct curl_slist *headers = NULL;
      headers = curl_slist_append(headers, "Content-Type: application/json");
      headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
      res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
          std::cerr << "Error: " << curl_easy_strerror(res) << std::endl;
          return {};
      }
      curl_slist_free_all(headers);
  }
  curl_easy_cleanup(curl);

  // Print the raw response for debugging
  //std::cout << "API Response: " << readBuffer << std::endl;

  // Parse the JSON response
  Json::Value jsonData;
  Json::CharReaderBuilder builder;
  std::istringstream stream(readBuffer);
  std::string errs;
  std::vector<Candle> candles;

  if (Json::parseFromStream(builder, stream, &jsonData, &errs)) {
    if (jsonData.isArray()) {
      for (const auto &candle : jsonData) {
        Candle c;
        c.timestamp = static_cast<time_t>(candle[0].asInt64()); // Convert timestamp to time_t
        c.closingPrice = candle[4].asDouble(); // Extract "close" price
        candles.push_back(c);
      }
    } else {
      std::cerr << "Error: Expected JSON array, but received something else." << std::endl;
    }
  } else {
    std::cerr << "JSON Parse Error: " << errs << std::endl;
  }

  return candles;
}


std::vector<Candle> fetchCoinbaseData(const std::string &product_id, int granularity) {
  std::string url = "https://api.exchange.coinbase.com/products/" +
                    product_id +"/candles?granularity=" + std::to_string(granularity);
  CURL *curl;
  CURLcode res;
  std::string readBuffer;

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    res = curl_easy_perform(curl);
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
      return {};
    }

    if (http_code != 200) {
      return {};
    }
  }

  // Print the raw response for debugging
  //std::cout << "API Response: " << readBuffer << std::endl;

  // Parse the JSON response
  Json::Value jsonData;
  Json::CharReaderBuilder builder;
  std::istringstream stream(readBuffer);
  std::string errs;
  std::vector<Candle> candles;

  if (Json::parseFromStream(builder, stream, &jsonData, &errs)) {
    if (jsonData.isArray()) {
      for (const auto &candle : jsonData) {
        Candle c;
        c.timestamp = static_cast<time_t>(candle[0].asInt64()); // Convert timestamp to time_t
        c.closingPrice = candle[4].asDouble(); // Extract "close" price
        candles.push_back(c);
      }
    } else {
      std::cerr << "Error: Expected JSON array, but received something else." << std::endl;
    }
  } else {
    std::cerr << "JSON Parse Error: " << errs << std::endl;
  }

  return candles;
}

  void printCandleData(const std::vector<Candle>& candles) {
    for (const auto &candle : candles) {
      std::cout << "Timestamp: " << std::ctime(&candle.timestamp) // Convert to readable time
                << "Closing Price: " << candle.closingPrice << std::endl;
    }
  }
};
#endif
