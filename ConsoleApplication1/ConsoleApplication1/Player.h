// Player.h
// Description: This file contains the declaration of the Player class
// Author: �����
// Date: 2024-08-15

#pragma once
#ifndef PLAYER_H
#define PLAYER_H

extern "C" {
#include <libavformat/avformat.h>  // FFmpeg�ĸ�ʽ����⣬���ڴ����ý���ʽ
#include <libavcodec/avcodec.h>    // FFmpeg�ı����⣬������Ƶ/��Ƶ�ı����
#include <libswresample/swresample.h> // FFmpeg���ز����⣬������Ƶ��ʽ��ת��
#include <libswscale/swscale.h>    // FFmpeg��ͼ�����ſ⣬����ͼ���ʽת��
#include <SDL.h>                   // SDL�⣬���ڴ������ڡ���Ⱦͼ��ʹ�����Ƶ
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}

#include <thread>
#include <queue>                   // ���ڴ洢��Ƶ����Ƶ֡�Ķ���
#include <mutex>                   // ���ڶ��̵߳Ļ�����
#include <condition_variable>      // �����̼߳����������
#include <chrono>
#include <iostream>
#include <vector>

class Player {
public:
	Player(const char* filename);
	~Player();

	void InitializePlayer();             // ��ʼ��������
	void StartPlayer();                  // ��ʼ����
	void StopPlayer();                   // ֹͣ����
	void TogglePause(); // �л�����/��ͣ״̬

private:
	void DecodeThread();           // �����߳�
	void VideoPlayThread();            // ��Ƶ�����߳�
	void AudioPlayThread();            // ��Ƶ�����߳�

	static void AudioCallback(void* userdata, Uint8* stream, int len);  // ��Ƶ�ص�����
	double GetCurrentSystemTime();              // ��ȡ��ǰʱ��

	const char* file_name_;         // ��ý���ļ���
	//��ͣʱ��
	double video_pause_time_;
	double audio_pause_time_;
	double pause_time_;

	// SDL���
	SDL_Window* play_sdl_window_;
	SDL_Renderer* play_sdl_renderer_;
	SDL_Texture* play_sdl_texture_;

	// �������
	AVFormatContext* format_ctx_;
	AVCodecContext* video_codec_ctx_;
	AVCodecContext* audio_codec_ctx_;
	int video_stream_;
	int audio_stream_;

	// ͬ�����
	std::mutex audio_mutex_ ;
	std::mutex video_mutex_;
	std::condition_variable audio_cond_;
	std::condition_variable video_cond_;
	std::queue<AVFrame*> audio_frame_queue_;
	std::queue<AVFrame*> video_frame_queue_;
	bool quit_;
	bool paused_; //�ո� ��ͣ ����
	// �߳�
	std::thread decode_thread_;
	std::thread video_play_thread_;
	std::thread audio_play_thread_;
};

#endif 
