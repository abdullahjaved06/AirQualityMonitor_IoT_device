
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(AWS_IoT_MQTT)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)


# Add the component AWS_IOT_MQTT
target_sources(app PRIVATE
    components/aws_iot_mqtt/aws_iot_mqtt.c)
target_include_directories(app
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/components/aws_iot_mqtt
)

# Add the component JSON_PAYLOAD
target_sources(app PRIVATE
    components/json_payload/json_payload.c)
target_include_directories(app
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/components/json_payload
)

# Add the component JSON_PAYLOAD
target_sources(app PRIVATE
    components/certificates/certificates.c)
target_include_directories(app
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/components/certificates
)

# Add the component SLEEP
target_sources(app PRIVATE
    components/sleep/sleep.c)
target_include_directories(app
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/components/sleep
)

# Add the component COMMON
target_sources(app PRIVATE
    components/common/common.c)
target_include_directories(app
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/components/common
)

# Add the component LTE_MANAGER
target_sources(app PRIVATE
    components/lte_manager/lte_manager.c)
target_include_directories(app
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/components/lte_manager
)

# Add the component sht40 sensor
target_sources(app PRIVATE
    components/sht40/sht40.c)
target_include_directories(app
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/components/sht40
)

# Add the component scd41 sensor
target_sources(app PRIVATE
    components/scd41/scd41.c)
target_include_directories(app
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/components/scd41
)

# Add the component led's
target_sources(app PRIVATE
    components/leds/led.c)
target_include_directories(app
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/components/leds
)

# Add the component npm1300
target_sources(app PRIVATE
    components/npm1300/npm1300.c)
target_include_directories(app
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/components/npm1300
)

target_sources(app PRIVATE
    components/epd/display.c)
target_include_directories(app
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/components/epd
)