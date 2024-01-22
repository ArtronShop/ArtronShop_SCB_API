#include "ArtronShop_SCB_API.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include "ArduinoJson-v7.0.2.h"

static const char * TAG = "ArtronShop_SCB_API";

#define API_HOST "api-sandbox.partners.scb"

ArtronShop_SCB_API::ArtronShop_SCB_API(String apiKey, String apiSecret, String authCode) {
    this->apiKey = apiKey;
    this->apiSecret = apiSecret;
    this->authCode = authCode;
}

bool ArtronShop_SCB_API::setClock() {
  configTime(0, 0, "pool.ntp.org");

  ESP_LOGV(TAG, "Waiting for NTP time sync");
  time_t nowSecs = 0;
  int err = 0;
  while(err < 10) {
    nowSecs = time(nullptr);
    if (nowSecs < (8 * 3600 * 2)) {
        delay(500);
        err++;
    } else {
        break;
    }
  }
  ESP_LOGV(TAG, "Current times: %d", nowSecs);

  return err < 10;
}

bool ArtronShop_SCB_API::verifyToken() {
    if (accessToken.length() == 0) {
        return false;
    }

    if (millis() < accessTokenUpdateAt) {
        return false;
    }

    if ((millis() - accessTokenUpdateAt) >= (expiresIn * 1000)) {
        return false;
    }

    return true;
}

bool ArtronShop_SCB_API::genToken() {
    WiFiClientSecure *client = new WiFiClientSecure;
    if(!client) {
        ESP_LOGE(TAG, "create WiFiClientSecure fail");
        return false;
    }

    bool ok = false;

    client->setInsecure(); // TODO: add CACert
    {
        HTTPClient http;
        if (http.begin(*client, "https://" API_HOST "/partners/sandbox/v1/oauth/token")) {
            http.addHeader("Content-Type", "application/json");
            http.addHeader("accept-language", "EN");
            http.addHeader("requestUId", "85230887-e643-4fa4-84b2-4e56709c4ac4");
            http.addHeader("resourceOwnerId", this->apiKey);

            String payload = "";
            payload += "{";
            payload += "\"applicationKey\": \"" + this->apiKey + "\", ";
            payload += "\"applicationSecret\": \"" + this->apiSecret + "\"";
            if (this->authCode.length() > 0) {
                payload += ", \"authCode\": \"" + this->authCode + "\"";
            }
            payload += "}";
            this->statusCode = http.POST(payload);
            if (this->statusCode == HTTP_CODE_OK) {
                String payload = http.getString();
                ESP_LOGV(TAG, "%s", payload.c_str());

                JsonDocument doc;
                if (deserializeJson(doc, payload) == DeserializationError::Ok) {
                    int code = 0;
                    if (!doc["status"]["code"].isNull()) {
                        code = doc["status"]["code"].as<int>();
                    }
                    if (code == 1000) {
                        if (!doc["data"]["accessToken"].isNull()) {
                            this->accessToken = doc["data"]["accessToken"].as<String>();
                            this->expiresIn = doc["data"]["expiresIn"].as<uint32_t>();
                            this->accessTokenUpdateAt = millis();

                            ESP_LOGI(TAG, "Token: %s", this->accessToken.c_str());
                            ESP_LOGI(TAG, "Expires In: %d", this->expiresIn);
                            ok = true;
                        } else {
                            ESP_LOGE(TAG, "accessToken not found in json payload");
                        }
                    } else {
                        ESP_LOGE(TAG, "Business Code error: %d", code);
                    }
                } else {
                    ESP_LOGE(TAG, "json decode fail");
                }
            } else {
                ESP_LOGE(TAG, "GET... failed, error: %s", http.errorToString(this->statusCode).c_str());
            }
        } else {
            ESP_LOGE(TAG, "Unable to connect");
        }
        http.end();
    }
  
    delete client;

    return ok;
}

bool ArtronShop_SCB_API::tokenRefresh() {
    // TODO: implement it

    return false;
}

/*
 * Ref1 & Ref2 & Ref3 => English capital letter and number only.
*/
bool ArtronShop_SCB_API::QRCodeTag30Create(String billerId, double amount, String ref1, String ref2, String ref3, String *qrRawData) {
    if (!this->verifyToken()) {
        if (!this->genToken()) {
            return false;
        }
    }

    // QR Tag 30
    WiFiClientSecure *client = new WiFiClientSecure;
    if(!client) {
        ESP_LOGE(TAG, "create WiFiClientSecure fail");
        return false;
    }

    bool ok = false;

    client->setInsecure(); // TODO: add CACert
    {
        HTTPClient http;
        if (http.begin(*client, "https://" API_HOST "/partners/sandbox/v1/payment/qrcode/create")) {
            http.addHeader("Content-Type", "application/json");
            http.addHeader("accept-language", "EN");
            http.addHeader("authorization", "Bearer " + this->accessToken);
            http.addHeader("requestUId", "1b01dff2-b3a3-4567-adde-cd9dd73c8b6d");
            http.addHeader("resourceOwnerId", this->apiKey);

            String payload = "";
            payload += "{";
            payload += "\"qrType\": \"PP\", ";
            payload += "\"ppType\": \"BILLERID\", ";
            payload += "\"ppId\": \"" + billerId + "\", ";
            payload += "\"amount\": \"" + String(amount, 2) + "\", ";
            payload += "\"ref1\": \"" + ref1 + "\", ";
            payload += "\"ref2\": \"" + ref2 + "\", ";
            payload += "\"ref3\": \"" + ref3 + "\" ";
            payload += "}";
            this->statusCode = http.POST(payload);
            if (this->statusCode == HTTP_CODE_OK) {
                String payload = http.getString();
                ESP_LOGV(TAG, "%s", payload.c_str());

                JsonDocument doc;
                if (deserializeJson(doc, payload) == DeserializationError::Ok) {
                    int code = 0;
                    if (!doc["status"]["code"].isNull()) {
                        code = doc["status"]["code"].as<int>();
                    }
                    if (code == 1000) {
                        if (!doc["data"]["qrRawData"].isNull()) {
                            *qrRawData = doc["data"]["qrRawData"].as<String>();
                            ESP_LOGI(TAG, "qrRawData: %s", qrRawData->c_str());
                            
                            this->lastBillerId = billerId;
                            this->lastAmount = amount;
                            this->lastRef1 = ref1;

                            time_t t = time(nullptr);
                            t += 7 * 60 * 60; // +7
                            struct tm * now = localtime(&t);
                            this->lastTransactionDate = "";
                            this->lastTransactionDate += String(now->tm_year + 1900);
                            this->lastTransactionDate += "-";
                            if ((now->tm_mon + 1) < 10) {
                                this->lastTransactionDate += "0";
                            }
                            this->lastTransactionDate += String(now->tm_mon + 1);
                            this->lastTransactionDate += "-";
                            if (now->tm_mday < 10) {
                                this->lastTransactionDate += "0";
                            }
                            this->lastTransactionDate += String(now->tm_mday);
                            
                            ok = true;
                        }
                    } else {
                        ESP_LOGE(TAG, "Business Code error: %d", code);
                    }
                } else {
                    ESP_LOGE(TAG, "json decode fail");
                }
            } else {
                ESP_LOGE(TAG, "GET... failed, error: %s", http.errorToString(this->statusCode).c_str());
            }
        } else {
            ESP_LOGE(TAG, "Unable to connect");
        }
        http.end();
    }
  
    delete client;

    return ok;
}

bool ArtronShop_SCB_API::checkPaymentConfirm(bool *paymentAreConfirm) {
    *paymentAreConfirm = false;

    // Payment Transaction Inquiry
    WiFiClientSecure *client = new WiFiClientSecure;
    if(!client) {
        ESP_LOGE(TAG, "create WiFiClientSecure fail");
        return false;
    }

    bool ok = false;

    client->setInsecure(); // TODO: add CACert
    {
        HTTPClient http;
        String endpoint = "https://" API_HOST "/partners/sandbox/v1/payment/billpayment/inquiry";
        endpoint += "?billerId=" + this->lastBillerId;
        endpoint += "&reference1=" + this->lastRef1;
        endpoint += "&transactionDate=" + this->lastTransactionDate;
        endpoint += "&eventCode=00300100";
        ESP_LOGV(TAG, "Endpoint: %s", endpoint.c_str());
        if (http.begin(*client, endpoint)) {
            http.addHeader("Content-Type", "application/json");
            http.addHeader("accept-language", "EN");
            http.addHeader("authorization", "Bearer " + this->accessToken);
            http.addHeader("requestUId", "871872a7-ed08-4229-a637-bb7c733305db");
            http.addHeader("resourceOwnerId", this->apiKey);

            this->statusCode = http.GET();
            if (this->statusCode == HTTP_CODE_OK) {
                *paymentAreConfirm = true;
                ok = true;
            } else if (this->statusCode == 404) {
                *paymentAreConfirm = false;
                ok = true;
            } else {
                ESP_LOGE(TAG, "GET... failed, error: %s", http.errorToString(this->statusCode).c_str());
            }
            
        } else {
            ESP_LOGE(TAG, "Unable to connect");
        }
        http.end();
    }
  
    delete client;

    return ok;
}
