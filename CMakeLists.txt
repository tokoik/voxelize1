cmake_minimum_required(VERSION 3.22)
project(voxelize1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC で UTF-8 ソースコードを正しく処理するための設定
if(MSVC)
    add_compile_options("/utf-8")
endif()

# ソースファイル
set(SOURCES
    voxelize1.cpp
)

# ヘッダファイル
set(HEADERS
)

# リソースファイル
set(RESOURCE_FILES
)

# ソリューションエクスプローラー用のグループ分け
source_group("Resource Files" FILES ${RESOURCE_FILES})

# 外部ライブラリをプロジェクト内の libs/ フォルダへ自動ダウンロードする設定
set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")

if(APPLE)

    # macOS の設定
    set(ICON_FILE "opengl.icns")

    # アプリケーションバンドルとして実行ファイルを作成する
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${RESOURCE_FILES} ${ICON_FILE})

    # GLUT と OpenGL の Framework を検索してリンクする
    find_library(GLUT_LIBRARY GLUT)
    find_library(OPENGL_LIBRARY OpenGL)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GLUT_LIBRARY} ${OPENGL_LIBRARY})

    # アプリケーションバンドルの作成設定
    set_target_properties(${PROJECT_NAME} PROPERTIES

        # アプリケーションバンドルとしてビルドする
        MACOSX_BUNDLE TRUE

        # Info.plist に書き込む内容
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.${PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_ICON_FILE "${ICON_FILE}"
        MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2026 Kohe Tokoi. All rights reserved."

        # Xcode のスキームを生成する
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.yourcompany.${PROJECT_NAME}"
        XCODE_GENERATE_SCHEME ON
    )

    # リソースファイルなどをアプリケーションバンドル内の Resources に組み込む
    set_source_files_properties(${RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    # Xcode 上で Run したときに起動するようにアプリケーションバンドルをスタートアッププロジェクトにする
    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY XCODE_GENERATE_SCHEME ON)

elseif(WIN32)

    # Windows (Visual Studio) の設定
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCE_FILES})

    # GLEW のソースを取得してビルドする
    include(FetchContent)
    FetchContent_Declare(
        glew
        GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
        GIT_TAG glew-cmake-2.3.1
    )
    set(ONLY_LIBS ON CACHE BOOL "" FORCE)
    set(BUILD_UTILS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glew)
    if(TARGET libglew_static)
        set_target_properties(libglew_static PROPERTIES INTERFACE_COMPILE_DEFINITIONS "GLEW_STATIC")
    endif()

    # freeglut のソースを取得してビルドする
    FetchContent_Declare(
        freeglut
        GIT_REPOSITORY https://github.com/FreeGLUTProject/freeglut.git
        GIT_TAG master
    )

    # freeglut の不要な共有ライブラリやデモをビルドしない
    set(FREEGLUT_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(FREEGLUT_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(freeglut)

    # OpenGL をリンクする
    find_package(OpenGL REQUIRED)

    # freeglut と GLEW のヘッダファイルのパスを通す
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${freeglut_SOURCE_DIR}/include"
        "${glew_SOURCE_DIR}/include"
    )

    # freeglut と GLEW は STATIC リンクする
    target_link_libraries(${PROJECT_NAME} PRIVATE libglew_static freeglut_static OpenGL::GL OpenGL::GLU)
    target_compile_definitions(${PROJECT_NAME} PRIVATE FREEGLUT_STATIC GLEW_STATIC)

    # このプロジェクトを Visual Studio のスタートアッププロジェクトとして設定する
    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

    # Visual Studio で実行する際の作業ディレクトリを実行ファイルと同階層にする
    set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

else()

    # Ubuntu Linux (その他UNIX系) の設定
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCE_FILES})

    # libGL.so を確実にリンクするための設定
    set(OpenGL_GL_PREFERENCE "LEGACY")

    # OpenGL と GLUT を探す
    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)

    # freeglut (glut) と gl と数学ライブラリ m をリンクする
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${OPENGL_LIBRARIES}
        GLUT::GLUT
        m
    )

endif()
