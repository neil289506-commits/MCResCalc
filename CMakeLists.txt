cmake_minimum_required(VERSION 3.21)
project(MinecraftResourceCalculator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ------------------------------------------------------------------
# MSVC 讀取沒有 BOM 的原始碼時，預設是用「系統代碼頁」去解讀，
# 在繁體中文 Windows 上就是 CP950 (Big5)。本專案的 .cpp/.h 是 UTF-8 無 BOM，
# 裡面有中文註解與 QStringLiteral("材料") 這類中文字串，被當成 Big5 解讀後
# 位元組會對不齊（一個 UTF-8 中文字 3 位元組，Big5 兩個兩個吃），
# 結果把字串的結尾引號或換行一起吞掉，編譯器就會在完全正常的程式碼上
# 報出莫名其妙的語法錯誤（例如把 slots[r * 3 + c] 當成 attribute 而丟出 C2337）。
#
# /utf-8 等同 /source-charset:utf-8 + /execution-charset:utf-8，
# 強制把來源與執行期字元集都設成 UTF-8，這是這個問題的標準解法。
# ------------------------------------------------------------------
if (MSVC)
    add_compile_options(/utf-8)
endif()

# ------------------------------------------------------------------
# Qt6：Widgets 是主視窗，Network 抓 Mojang API，Quick / QuickWidgets / Quick3D / Qml
# 是拿來在 QWidgets 視窗裡嵌一塊 Qt Quick 3D 場景做方塊預覽用的。
# ------------------------------------------------------------------
find_package(Qt6 6.11 REQUIRED COMPONENTS
    Core
    Widgets
    Network
    Concurrent
    Qml
    Quick
    QuickWidgets
    Quick3D
)

# ------------------------------------------------------------------
# QuaZip：用來解壓縮 client.jar（本質就是 zip）。
# 不同的 vcpkg / 發行版打包方式，target 名稱可能不一樣，這裡列出最常見的兩種寫法，
# 如果 find_package 這行報錯，把下面兩行的註解對調試試看，或用
# `vcpkg list | findstr quazip` 確認你實際裝的 port 名稱與 target。
# ------------------------------------------------------------------
find_package(QuaZip-Qt6 REQUIRED)
set(QUAZIP_TARGET QuaZip::QuaZip)
# find_package(QuaZip REQUIRED)
# set(QUAZIP_TARGET QuaZip::QuaZip)

add_executable(MinecraftResourceCalculator
    src/main.cpp
    src/UI/MainWindow.h
    src/UI/MainWindow.cpp
    src/UI/Block3DPreview.h
    src/UI/Block3DPreview.cpp
    src/Models/VersionModels.h
    src/Models/RecipeModels.h
    src/Models/MaterialNode.h
    src/Models/MaterialNode.cpp
    src/Models/CraftingStep.h
    src/Services/GameCacheService.h
    src/Services/GameCacheService.cpp
    src/Services/MojangApiService.h
    src/Services/MojangApiService.cpp
    src/Services/JarRecipeExtractor.h
    src/Services/JarRecipeExtractor.cpp
    src/Services/TagResolver.h
    src/Services/TagResolver.cpp
    src/Services/LangService.h
    src/Services/LangService.cpp
    src/Services/RecipeIndex.h
    src/Services/RecipeIndex.cpp
    src/Services/ResourceCalculatorService.h
    src/Services/ResourceCalculatorService.cpp
    src/Services/TextureService.h
    src/Services/TextureService.cpp
    src/Services/ModelShapeResolver.h
    src/Services/ModelShapeResolver.cpp
    resources.qrc
app.rc
)

target_include_directories(MinecraftResourceCalculator PRIVATE src)

target_link_libraries(MinecraftResourceCalculator PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickWidgets
    Qt6::Quick3D
    ${QUAZIP_TARGET}
)

if (WIN32)
    set_target_properties(MinecraftResourceCalculator PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
