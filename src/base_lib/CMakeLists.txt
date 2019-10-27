
if(WIN32)
ADD_LIBRARY(
    olm_base STATIC
    base64.c
    CryptoHelper.cpp
    win/CryptoHelperWindows.cpp
)
else(WIN32)
ADD_LIBRARY(
    olm_base STATIC
    base64.cpp
    CryptoHelper.cpp
    linux/CryptoHelperLinux.cpp
)
ENDIF(WIN32)

target_link_libraries(
     olm_base
     ${EXTERNAL_LIBS}
)

#install(TARGETS olm_base ARCHIVE DESTINATION lib/${PROJECT_NAME})
