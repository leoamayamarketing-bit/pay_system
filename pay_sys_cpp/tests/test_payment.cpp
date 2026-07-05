#include <iostream>
#include <cassert>
#include "core/Money.h"
#include "core/PaymentResult.h"
#include "payment/CreditCard.h"
#include "payment/DebitCard.h"
#include "payment/QRPayment.h"
#include "payment/NFCPayment.h"
#include "payment/PaymentFactory.h"

using namespace freebuff;

int main() {
    std::cout << "=== Payment Integration Tests ===\n\n";
    
    int passed = 0;
    int failed = 0;
    
    auto check = [&](bool condition, const std::string& desc) {
        if (condition) {
            std::cout << "  PASS: " << desc << "\n";
            passed++;
        } else {
            std::cout << "  FAIL: " << desc << "\n";
            failed++;
        }
    };
    
    // 1. Money tests
    std::cout << "\n  --- Money Tests ---\n";
    
    Money m1 = Money::fromDollars(10.50);
    check(m1.cents() == 1050, "fromDollars(10.50) -> 1050 cents");
    check(m1.toString() == "$10.50", "toString format");
    
    Money m2 = Money::fromCents(500);
    check(m2.toDollars() == 5.00, "fromCents(500) -> $5.00");
    
    Money sum = m1 + m2;
    check(sum.cents() == 1550, "Addition: 1050 + 500 = 1550");
    
    Money diff = m1 - m2;
    check(diff.cents() == 550, "Subtraction: 1050 - 500 = 550");
    
    check(m1 > m2, "Greater than comparison");
    check(m2 < m1, "Less than comparison");
    check(m1 == Money::fromDollars(10.50), "Equality comparison");
    
    // 2. Credit Card tests
    std::cout << "\n  --- Credit Card Tests ---\n";
    
    CardInfo validCard{"4111111111111111", 12, 2028, "123", "John Doe"};
    CreditCard cc(validCard);
    check(cc.name() == "Credit Card", "Name is Credit Card");
    
    auto ccValidation = cc.validate();
    check(ccValidation.isSuccess(), "Valid credit card passes validation");
    
    auto ccProcess = cc.process(Money::fromDollars(100));
    check(ccProcess.isSuccess(), "Credit card process $100");
    check(ccProcess.transactionId.has_value(), "Has transaction ID");
    check(ccProcess.transactionId->rfind("CC-", 0) == 0, "Transaction ID starts with CC-");
    
    auto ccRefund = cc.refund(*ccProcess.transactionId);
    check(ccRefund.status == PaymentStatus::Refunded, "Refund successful");
    
    // Test invalid amount
    auto invalidProcess = cc.process(Money::fromDollars(0));
    check(!invalidProcess.isSuccess(), "Zero amount rejected");
    check(invalidProcess.status == PaymentStatus::InvalidAmount, "InvalidAmount status");
    
    // 3. Debit Card tests
    std::cout << "\n  --- Debit Card Tests ---\n";
    
    BankAccount account{"ES12345", "BANK01", Money::fromDollars(500)};
    DebitCard dc("1234567890123456", account);
    check(dc.name() == "Debit Card", "Name is Debit Card");
    
    // Insufficient funds
    auto dcResult = dc.process(Money::fromDollars(1000));
    check(!dcResult.isSuccess(), "Insufficient funds rejected");
    check(dcResult.status == PaymentStatus::InsufficientFunds, "InsufficientFunds status");
    
    // Sufficient funds
    dcResult = dc.process(Money::fromDollars(100));
    check(dcResult.isSuccess(), "Debit card process $100");
    check(dc.balance() == Money::fromDollars(400), "Balance deducted correctly: $500 - $100 = $400");
    
    // 4. QR Payment tests
    std::cout << "\n  --- QR Payment Tests ---\n";
    
    QRPayload payload{"MERCH001", "TestStore", "REF001", Money::fromDollars(50)};
    QRPayment qr(payload);
    check(qr.name() == "QR Payment", "Name is QR Payment");
    
    auto qrResult = qr.process(Money::fromDollars(25.50));
    check(qrResult.isSuccess(), "QR payment process $25.50");
    
    auto qrCode = qr.generateQRCode();
    check(!qrCode.empty(), "QR code generated");
    check(qrCode.size() > 3, "QR code has multiple lines");
    
    // Test encode/decode
    QRPayload encodedPayload{"MERCH002", "AnotherStore", "REF002", Money::fromCents(9999)};
    std::string encoded = encodedPayload.encode();
    check(encoded.find("FREEBUFFQR:") == 0, "QR payload encoding has correct prefix");
    
    auto decoded = QRPayload::decode(encoded);
    check(decoded.has_value(), "QR payload decoding succeeds");
    check(decoded->merchantId == "MERCH002", "Decoded merchant ID matches");
    check(decoded->amount == Money::fromCents(9999), "Decoded amount matches");
    
    // Test decode failure
    auto badDecode = QRPayload::decode("INVALID:data");
    check(!badDecode.has_value(), "Invalid QR data rejected");
    
    // 5. NFC Payment tests
    std::cout << "\n  --- NFC Payment Tests ---\n";
    
    NFCData nfcData{"A1:B2:C3:D4", NFCCardType::Visa, "411111******1111"};
    NFCPayment nfc(nfcData);
    check(nfc.name() == "NFC Payment", "Name is NFC Payment");
    
    auto nfcResult = nfc.process(Money::fromDollars(75));
    check(nfcResult.isSuccess(), "NFC payment process $75");
    check(nfcResult.transactionId->rfind("NFC-", 0) == 0, "Transaction ID starts with NFC-");
    
    // 6. PaymentFactory tests
    std::cout << "\n  --- PaymentFactory Tests ---\n";
    
    auto& factory = PaymentFactory::instance();
    
    factory.registerMethod("TestCC", []() -> std::unique_ptr<IPaymentMethod> {
        return std::make_unique<CreditCard>(
            CardInfo{"4111111111111111", 12, 2028, "123", "Test"});
    });
    
    auto methods = factory.availableMethods();
    check(!methods.empty(), "Factory has registered methods");
    
    auto testMethod = factory.create("TestCC");
    check(testMethod != nullptr, "Factory creates CreditCard");
    check(testMethod->name() == "Credit Card", "Created method is Credit Card");
    
    auto nonExistent = factory.create("NonExistent");
    check(nonExistent == nullptr, "Factory returns null for unregistered method");
    
    check(factory.unregisterMethod("TestCC"), "Unregister method succeeds");
    check(!factory.unregisterMethod("NonExistent"), "Unregister non-existent returns false");
    
    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}
