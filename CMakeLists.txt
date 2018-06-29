# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

SET(LIVE555 jni/live/jni)

file(GLOB BasicUsageEnvironment_SRC  "${LIVE555}/BasicUsageEnvironment/*.c*")
file(GLOB groupsock_SRC  "${LIVE555}/groupsock/*.c*")
file(GLOB liveMedia_SRC  "${LIVE555}/liveMedia/*.c*")
file(GLOB UsageEnvironment_SRC  "${LIVE555}/UsageEnvironment/*.c*")

add_library( # Sets the name of the library.
             rtsplive555

             # Sets the library as a shared library.
             SHARED
             ${BasicUsageEnvironment_SRC}
             ${groupsock_SRC}
             ${liveMedia_SRC}
             ${UsageEnvironment_SRC}
             # Provides a relative path to your source file(s).
             jni/testRTSPClient.cpp
             )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )


INCLUDE_DIRECTORIES(${LIVE555}/BasicUsageEnvironment/include
                    ${LIVE555}/groupsock/include
                    ${LIVE555}/liveMedia/include
                    ${LIVE555}/UsageEnvironment/include
                    /usr/include
                    jni/)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       rtsplive555
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )