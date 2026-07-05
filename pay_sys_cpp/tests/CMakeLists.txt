cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

# Link against the main library sources
set(TEST_SOURCES
    test_luhn.cpp
    test_payment.cpp
)

add_executable(test_luhn test_luhn.cpp ${PROJECT_SOURCE_DIR}/src/core/PaymentResult.cpp ${PROJECT_SOURCE_DIR}/src/core/Money.cpp)
add_test(NAME LuhnTest COMMAND test_luhn)

add_executable(test_payment test_payment.cpp
    ${PROJECT_SOURCE_DIR}/src/core/PaymentResult.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Money.cpp
    ${PROJECT_SOURCE_DIR}/src/payment/CreditCard.cpp
    ${PROJECT_SOURCE_DIR}/src/payment/DebitCard.cpp
    ${PROJECT_SOURCE_DIR}/src/payment/PaymentFactory.cpp
    ${PROJECT_SOURCE_DIR}/src/payment/QRPayment.cpp
    ${PROJECT_SOURCE_DIR}/src/payment/NFCPayment.cpp
)
add_test(NAME PaymentTest COMMAND test_payment)
