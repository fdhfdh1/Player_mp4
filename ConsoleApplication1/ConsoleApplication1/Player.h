// Player.h
// Description: This file contains the declaration of the Player class
// Author: 彭金龙
// Date: 2024-08-15

#pragma once
#ifndef PLAYER_H
#define PLAYER_H

extern "C" {
#include <libavformat/avformat.h>  // FFmpeg的格式处理库，用于处理多媒体格式
#include <libavcodec/avcodec.h>    // FFmpeg的编解码库，用于视频/音频的编解码
#include <libswresample/swresample.h> // FFmpeg的重采样库，用于音频格式的转换
#include <libswscale/swscale.h>    // FFmpeg的图像缩放库，用于图像格式转换
#include <SDL.h>                   // SDL库，用于创建窗口、渲染图像和处理音频
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}

#include <thread>
#include <queue>                   // 用于存储视频和音频帧的队列
#include <mutex>                   // 用于多线程的互斥锁
#include <condition_variable>      // 用于线程间的条件变量
#include <chrono>
#include <iostream>
#include <vector>

class Player {
public:
	Player(const char* filename);
	~Player();

	void InitializePlayer();             // 初始化播放器
	void StartPlayer();                  // 开始播放
	void StopPlayer();                   // 停止播放
	void TogglePause(); // 切换播放/暂停状态

private:
	void DecodeThread();           // 解码线程
	void VideoPlayThread();            // 视频播放线程
	void AudioPlayThread();            // 音频播放线程

	static void AudioCallback(void* userdata, Uint8* stream, int len);  // 音频回调函数
	double GetCurrentSystemTime();              // 获取当前时间

	const char* file_name_;         // 多媒体文件名
	//暂停时间
	double video_pause_time_;
	double audio_pause_time_;
	double pause_time_;

	// SDL相关
	SDL_Window* play_sdl_window_;
	SDL_Renderer* play_sdl_renderer_;
	SDL_Texture* play_sdl_texture_;

	// 解码相关
	AVFormatContext* format_ctx_;
	AVCodecContext* video_codec_ctx_;
	AVCodecContext* audio_codec_ctx_;
	int video_stream_;
	int audio_stream_;

	// 同步相关
	std::mutex audio_mutex_ ;
	std::mutex video_mutex_;
	std::condition_variable audio_cond_;
	std::condition_variable video_cond_;
	std::queue<AVFrame*> audio_frame_queue_;
	std::queue<AVFrame*> video_frame_queue_;
	bool quit_;
	bool paused_; //空格 暂停 播放
	// 线程
	std::thread decode_thread_;
	std::thread video_play_thread_;
	std::thread audio_play_thread_;
};

#endif 
