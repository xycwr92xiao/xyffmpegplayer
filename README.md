

# up主添加

源项目地址：[itisyang/XyPlayer: 一个视频音频播放器，开源版 potplayer ，用于总结播放器开发技术。 (github.com)](https://github.com/xycwr92xiao/XyPlayer)


## 简介

- 使用 FFmpeg-4.2.2 (x64) 解码，SDL2-2.0.10 (x64) 渲染。  
- 在 Windows 下使用 Qt5.14.x (MinGW 64版本) 开发。  
- 项目目录下的 .pro 文件，  QtCreator 打开编译调试。  

 


## Windows平台编译调试
1. 程序运行需要的运行时库在XyPlayer\dll

2. 使用 QtCreator 打开 playerdemo.pro。  
3. 编译运行。  





# 原GitHub作者

# playerdemo


一个视频播放器，开源版 potplayer。  
用于学习和交流音视频技术。  
欢迎音视频开发爱好者交流相关问题。  
https://xycwr92xiao.github.io/XyPlayer/

## 简介
- 使用 FFmpeg-4.2.2 (x64) 解码，SDL2-2.0.10 (x64) 渲染。  
- 在 Windows 下使用 Qt5.14.x (MinGW x64) 开发。  
- 项目目录下的 .pro 文件，支持在多平台（Windows、Linux、Mac）下 QtCreator 打开编译调试。  

![playerdemo_play](https://cdn.staticaly.com/gh/itisyang/MyImages@master/images/playerdemo_play.png)

## 沟通
- Issues: 欢迎给我提 issues，关于本项目的问题，请优先提 issues，我会尽量当天回复。

## Windows平台编译调试
1. 下载 FFmpeg、SDL2 动态库，dll放在 bin 目录下，头文件和lib文件替换掉lib文件夹中的内容。(直接从官网下载即可)  
    FFmpeg 库下载地址 [https://ffmpeg.zeranoe.com/builds/](https://ffmpeg.zeranoe.com/builds/)  
    SDL2 库下载地址 [https://www.libsdl.org/download-2.0.php](https://www.libsdl.org/download-2.0.php)  
2. 使用 QtCreator 打开 playerdemo.pro。  
3. 编译运行。  

## Linux平台编译调试  
1. 安装 SDL2相关的开发包 libsdl2-dev。  
    ```
    sudo apt-get install libsdl2-dev
    ```
2. 安装 FFmpeg相关的开发包 libavformat-dev、libavutil-dev、libavcodec-dev、libswscale-dev...
    ```
    sudo apt-get install libavformat-dev
    sudo apt-get install libavutil-dev
    sudo apt-get install libavcodec-dev
    sudo apt-get install libswscale-dev
    ```
3. 使用 QtCreator 打开 playerdemo.pro。  
4. 编译运行。  

## Macos平台编译调试
1. 安装 FFmpeg相关的开发包。
    ```
    brew install ffmpeg
    ```
2. 使用 QtCreator 打开 playerdemo.pro。  
3. 修改 playerdemo.pro 配置 ffmpeg、SDL2 头文件和库目录。
    ```
    # 将下面的路径改为自己设备上的路径
    macx {
        INCLUDEPATH += /usr/local/Cellar/sdl2/2.24.1/include
        LIBS += -L/usr/local/Cellar/sdl2/2.24.1/lib -lSDL2
        INCLUDEPATH += /usr/local/Cellar/ffmpeg@5.1.1/5.1.1_1/include
        LIBS += -L/usr/local/Cellar/ffmpeg@5.1.1/5.1.1_1/lib -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale
    }
    ```
    *通过 brew info ffmpeg 查询 ffmpeg 安装目录*  
    *ffmpeg 安装时会自动安装SDL2依赖，通过 brew info SDL2 查询 SDL2 安装目录*  

4. 编译运行。  

## 其他

    编译时，注意统一静态库与动态库的版本、位数。若开发环境为64位，库及头文件均要64位。  
    tag中的打包版本使用actions打包，不包含ffmpeg和SDL2动态库，后续研究如何一起打包。

# xyffmpegplayer
