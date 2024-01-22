#include <Arduino.h>

class ArtronShop_SCB_API {
    private:
        String apiKey;
        String apiSecret;
        String authCode;
        int statusCode = 0;

        String accessToken;
        uint32_t expiresIn;
        uint32_t accessTokenUpdateAt;

        bool verifyToken() ;
        bool genToken() ;
        bool tokenRefresh() ;

        String lastBillerId;
        double lastAmount;
        String lastRef1;
        String lastTransactionDate;

    public:
        ArtronShop_SCB_API(String apiKey, String apiSecret, String authCode = "") ;
        
        bool setClock() ;
        bool QRCodeTag30Create(String billerId, double amount, String ref1, String ref2, String ref3, String *qrRawData) ;
        // bool PaymentTransactionInquiryForQRCodeTag30(String billerId, double amount, String ref1, String transactionDate) ;
        bool checkPaymentConfirm(bool *paymentAreConfirm) ;

};

