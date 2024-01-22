# ArtronShop_SCB_API

SCB API SDK for ESP32, Support Thai QR Code Tag 30 and Payment Confirm

**DO NOT USE IT IN PRODUCTION**

it library support sandbox mode only. if use in production plz check

 * TLS / SSL CACert
 * Prodution API host and endpoint
 * Add more security, don't hardcode API Key and API Secret into code

## QR Code Generation

support QR 30 Only

```c++
bool QRCodeTag30Create(String billerId, double amount, String ref1, String ref2, String ref3, String *qrRawData) ;
```

this function use `POST /​v1/​payment/​qrcode/​create` API, plz check in docs

 * Biller ID : Partners can get on merchant profile of their application.
 * ref1, ref2, ref3 : Reference number required for the relevant payment methods. English capital letter and number only. Length: up to 20
 * qrRawData : output of QR Code data, next create QR Code image and display to customer

this function return `True` if create successful, return `False` if no internet connect or some parameter invalid


```c++
bool checkPaymentConfirm(bool *paymentAreConfirm) ;
```

this function use `GET /​partners/​v1/​payment/​billpayment/​inquiry?{param}={value}` API, plz check in docs

 * paymentAreConfirm : output of last QR Code Tag 30 payment status, it `True` when customer make payment

this function return `True` if check payment successful, return `False` if no internet connect or some parameter invalid

## More

Check in docs : https://developer.scb/#/documents/documentation/qr-payment/thai-qr.html
