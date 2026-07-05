cmake_minimum_required(VERSION 3.20)
project(FreeBuffPaymentSystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build unit tests" ON)
option(USE_LIBQRENCODE "Link libqrencode" OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type)
endif()

set(CORE_SOURCES
    src/core/PaymentResult.cpp
    src/core/Transaction.cpp
    src/core/Money.cpp
)

set(PAYMENT_SOURCES
    src/payment/CreditCard.cpp
    src/payment/DebitCard.cpp
    src/payment/QRPayment.cpp
    src/payment/NFCPayment.cpp
    src/payment/PaymentFactory.cpp
)

set(HARDWARE_SOURCES
    src/hardware/NFCReader.cpp
    src/hardware/USBDeviceManager.cpp
    src/hardware/HardwareAbstraction.cpp
)

set(PERSISTENCE_SOURCES
    src/persistence/TransactionRepository.cpp
    src/persistence/Logger.cpp
)

set(UI_SOURCES
    src/ui/TerminalUI.cpp
)

set(ALL_SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${PAYMENT_SOURCES}
    ${HARDWARE_SOURCES}
    ${PERSISTENCE_SOURCES}
    ${UI_SOURCES}
)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME} ${ALL_SOURCES})

if(USE_LIBQRENCODE)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(QRENCODE libqrencode)
        if(QRENCODE_FOUND)
            target_compile_definitions(${PROJECT_NAME} PRIVATE USE_LIBQRENCODE)
            target_include_directories(${PROJECT_NAME} PRIVATE ${QRENCODE_INCLUDE_DIRS})
            target_link_libraries(${PROJECT_NAME} PRIVATE ${QRENCODE_LIBRARIES})
        endif()
    endif()
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)
