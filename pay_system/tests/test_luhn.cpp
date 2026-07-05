#include <iostream>
#include <string>
#include <cassert>
#include "payment/CreditCard.h"

using namespace freebuff;

int main() {
    std::cout << "=== Luhn Algorithm Tests ===\n\n";
    
    struct TestCase {
        std::string cardNumber;
        bool expected;
        const char* description;
    };
    
    TestCase tests[] = {
        {"4111111111111111", true, "Visa test number"},
        {"5500000000000004", true, "Mastercard test number"},
        {"340000000000009", true, "Amex test number (15 digits)"},
        {"1234567890123456", false, "Invalid number"},
        {"", false, "Empty string"},            {"0001", false, "Too short (fails Luhn)"},
        {"4111111111111112", false, "Wrong checksum digit"},
        {"4012888888881881", true, "Another valid Visa"},
        {"abc", false, "Non-numeric"},
        {"4111 1111 1111 1111", true, "With spaces (clean version)"},
    };
    
    int passed = 0;
    int failed = 0;
    
    // Test without spaces
    for (const auto& test : tests) {
        // Skip the spaces test for Luhn direct
        if (test.cardNumber.find(' ') != std::string::npos) continue;
        
        bool result = CreditCard::validateLuhn(test.cardNumber);
        if (result == test.expected) {
            std::cout << "  PASS: " << test.description << "\n";
            passed++;
        } else {
            std::cout << "  FAIL: " << test.description
                      << " (expected " << (test.expected ? "valid" : "invalid")
                      << ", got " << (result ? "valid" : "invalid") << ")\n";
            failed++;
        }
    }
    
    // Test CVV validation
    std::cout << "\n  --- CVV Validation ---\n";
    
    struct CVVTest {
        std::string cvv;
        bool expected;
        const char* desc;
    };
    
    CVVTest cvvTests[] = {
        {"123", true, "3-digit CVV"},
        {"1234", true, "4-digit CVV (Amex)"},
        {"12", false, "Too short"},
        {"12345", false, "Too long"},
        {"abc", false, "Non-numeric"},
        {"", false, "Empty CVV"},
    };
    
    for (const auto& t : cvvTests) {
        bool result = CreditCard::validateCVV(t.cvv);
        if (result == t.expected) {
            std::cout << "  PASS: " << t.desc << "\n";
            passed++;
        } else {
            std::cout << "  FAIL: " << t.desc << "\n";
            failed++;
        }
    }
    
    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}
