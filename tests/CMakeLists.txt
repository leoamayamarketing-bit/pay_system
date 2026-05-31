# ═══════════════════════════════════════════════════════════════════════════════
#  FreeBuff Payment System  –  Unit Tests
#  Links against the project's static libraries.
# ═══════════════════════════════════════════════════════════════════════════════

cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

# ── Luhn Algorithm Tests ─────────────────────────────────────────────────────
add_executable(test_luhn test_luhn.cpp)
target_link_libraries(test_luhn PRIVATE core payment)
if(TARGET qr_sim)
    target_link_libraries(test_luhn PRIVATE qr_sim)
endif()
add_test(NAME LuhnTest COMMAND test_luhn)

# ── Payment Integration Tests ────────────────────────────────────────────────
add_executable(test_payment test_payment.cpp)
target_link_libraries(test_payment PRIVATE core payment)
if(TARGET qr_sim)
    target_link_libraries(test_payment PRIVATE qr_sim)
endif()
add_test(NAME PaymentTest COMMAND test_payment)
