// Player.cpp
// Description: This file is an implementation of the Player class
// Author: 彭金龙
// Date: 2024-08-15

#include "Player.h"
Player::Player(const char* filename)
	: file_name_(filename), 
	play_sdl_window_(nullptr),
	play_sdl_renderer_(nullptr), 
	play_sdl_texture_(nullptr),
	format_ctx_(nullptr), 
	video_codec_ctx_(nullptr),
	audio_codec_ctx_(nullptr),
	video_stream_(-1), 
	audio_stream_(-1), 
	quit_(false),
	paused_(false),
	video_pause_time_(0),
	audio_pause_time_(0),
	pause_time_(0) {

} 

Player::~Player() {
	StopPlayer();
	SDL_DestroyTexture(play_sdl_texture_);
	SDL_DestroyRenderer(play_sdl_renderer_);
	SDL_DestroyWindow(play_sdl_window_);
	SDL_CloseAudio();
	SDL_Quit();
}

double Player::GetCurrentSystemTime() {
	using namespace std::chrono;
	return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
}

void Player::AudioCallback(void* userdata, Uint8* stream, int len) {
	Player* player = static_cast<Player*>(userdata);
	static std::vector<float> audio_buffer;

	// 如果处于暂停状态，直接返回静音数据
	if (player->paused_) {
		SDL_memset(stream, 0, len);
		return;
	}
	
	constexpr size_t sample_size = sizeof(float);
	size_t required_samples = len / sample_size;

	if (audio_buffer.size() < required_samples) {
		std::unique_lock<std::mutex> lock(player->audio_mutex_);
		player->audio_cond_.wait(lock, [&] { return !player->audio_frame_queue_.empty() || player->quit_; });
		if (player->quit_) return;

		AVFrame* frame = player->audio_frame_queue_.front();
		player->audio_frame_queue_.pop();

		for (int i = 0; i < frame->nb_samples; ++i) {
			for (int ch = 0; ch < 2; ++ch) {
				float* samples = reinterpret_cast<float*>(frame->data[ch]);
				audio_buffer.push_back(samples[i]);
			}
		}
	}

	SDL_memcpy(stream, reinterpret_cast<Uint8*>(audio_buffer.data()), len);
	audio_buffer.erase(audio_buffer.begin(), audio_buffer.begin() + required_samples);
}

void Player::InitializePlayer() {
	// 初始化SDL
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	play_sdl_window_ = SDL_CreateWindow("Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1080, 700, 0);
	play_sdl_renderer_ = SDL_CreateRenderer( play_sdl_window_, -1, SDL_RENDERER_SOFTWARE);
	play_sdl_texture_ = SDL_CreateTexture(play_sdl_renderer_, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, 1080, 700);

	// 打开多媒体文件
	format_ctx_ = avformat_alloc_context();
	avformat_open_input(&format_ctx_, file_name_, nullptr, nullptr);
	avformat_find_stream_info(format_ctx_, nullptr);

	// 查找视频和音频流
	for (int i = 0; i < format_ctx_->nb_streams; i++) {
		if (format_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_ < 0) {
			video_stream_ = i;
		}
		if (format_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_ < 0) {
			audio_stream_ = i;
		}
	}

	// 初始化视频解码器
	video_codec_ctx_ = avcodec_alloc_context3(nullptr);
	avcodec_parameters_to_context(video_codec_ctx_, format_ctx_->streams[video_stream_]->codecpar);
	const AVCodec* video_codec = avcodec_find_decoder(video_codec_ctx_->codec_id);
	avcodec_open2(video_codec_ctx_, video_codec, nullptr);

	// 初始化音频解码器
	audio_codec_ctx_ = avcodec_alloc_context3(nullptr);
	AVStream* st = format_ctx_->streams[audio_stream_];
	avcodec_parameters_to_context(audio_codec_ctx_, format_ctx_->streams[audio_stream_]->codecpar);
	const AVCodec* audio_codec = avcodec_find_decoder(st->codecpar->codec_id);
	avcodec_open2(audio_codec_ctx_, audio_codec, nullptr);
}

void Player::StartPlayer() {
	// 启动解码线程、视频播放线程和音频播放线程
	decode_thread_ = std::thread(&Player::DecodeThread, this);
	video_play_thread_ = std::thread(&Player::VideoPlayThread, this);
	audio_play_thread_ = std::thread(&Player::AudioPlayThread, this);
}

void Player::StopPlayer() {
	quit_ = true;
	video_cond_.notify_all();
	audio_cond_.notify_all();
	if (decode_thread_.joinable()) decode_thread_.join();
	if (video_play_thread_.joinable()) video_play_thread_.join();
	if (audio_play_thread_.joinable()) audio_play_thread_.join();
}

void Player::DecodeThread() {
	AVPacket* packet = av_packet_alloc();
	double video_start_time = GetCurrentSystemTime();

  	while (av_read_frame(format_ctx_, packet) >= 0 && !quit_) {
		std::cout << paused_ << std::endl;
		if (packet->stream_index == video_stream_) {
			AVFrame* frame = av_frame_alloc();
			avcodec_send_packet(video_codec_ctx_, packet);
			if (avcodec_receive_frame(video_codec_ctx_, frame) == 0) {
				double frame_time = frame->pts * av_q2d(format_ctx_->streams[video_stream_]->time_base);
				double current_time = GetCurrentSystemTime();
				/*std::cout << "frame_time-"<<frame_time << std::endl;
				std::cout << "current_time-" << current_time << std::endl;
				std::cout << "video_start_time_-" << video_start_time_ << std::endl;*/
				double delay = frame_time - (current_time - video_start_time);
				/*std::cout << "delay-" << delay << std::endl;*/
				if (delay > 0) {
					std::this_thread::sleep_for(std::chrono::duration<double>(delay));
				}

				std::unique_lock<std::mutex> lock(video_mutex_);
				video_frame_queue_.push(frame);
				video_cond_.notify_one();
			}
		}
		else if (packet->stream_index == audio_stream_) {
			AVFrame* frame = av_frame_alloc();

			if (avcodec_send_packet(audio_codec_ctx_, packet) == 0) {
				if (avcodec_receive_frame(audio_codec_ctx_, frame) == 0) {
					std::unique_lock<std::mutex> lock(audio_mutex_);
					audio_frame_queue_.push(frame);
					audio_cond_.notify_one();
				}
				else {
					av_frame_free(&frame);
				}
			}
			else {
				av_frame_free(&frame);
			}
		}
	}
	quit_ = true;
	video_cond_.notify_all();
	audio_cond_.notify_all();

	av_packet_free(&packet);
	avcodec_free_context(&video_codec_ctx_);
	avcodec_free_context(&audio_codec_ctx_);
	avformat_close_input(&format_ctx_);
}

void Player::VideoPlayThread() {
	double video_start_time = GetCurrentSystemTime();

	while (!quit_) {
		std::unique_lock<std::mutex> lock(video_mutex_);
		video_cond_.wait(lock, [&] { return !video_frame_queue_.empty() || quit_ || !paused_; });
		if (quit_ || paused_) continue;
		if (!video_frame_queue_.empty()) {
			AVFrame* frame = video_frame_queue_.front();
			video_frame_queue_.pop();

			double frame_time = frame->pts * av_q2d(format_ctx_->streams[video_stream_]->time_base);


			double current_time = GetCurrentSystemTime()- video_pause_time_;

			std::cout << "frame_time-" << frame_time << std::endl;
			std::cout << "current_time-" << current_time << std::endl;
			std::cout << "video_start_time_-" << video_pause_time_ << std::endl;
			std::cout << "video_start_time-" << video_start_time << std::endl;
			double delay = frame_time - (current_time - video_start_time);
			std::cout << "delay-" << delay << std::endl;
			if (delay > 0) {
				std::this_thread::sleep_for(std::chrono::duration<double>(delay));
			}

			// 获取视频帧的实际尺寸
			int frame_width = frame->width;
			int frame_height = frame->height;
			// 更新SDL纹理的尺寸
			SDL_DestroyTexture(play_sdl_texture_);
			play_sdl_texture_ = SDL_CreateTexture(play_sdl_renderer_, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, frame_width, frame_height);

			SDL_UpdateYUVTexture(play_sdl_texture_,
				nullptr,
				frame->data[0], frame->linesize[0],
				frame->data[1], frame->linesize[1],
				frame->data[2], frame->linesize[2]);
			SDL_RenderClear(play_sdl_renderer_);
			SDL_RenderCopy(play_sdl_renderer_, play_sdl_texture_, nullptr, nullptr);
			SDL_RenderPresent(play_sdl_renderer_);

			av_frame_free(&frame);
		
		}
	
	}
}
void Player::TogglePause() {
	std::unique_lock<std::mutex> lock_audio(audio_mutex_);
	std::unique_lock<std::mutex> lock_video(video_mutex_);

	paused_ = !paused_;
	if (!paused_) {

		double current_time = GetCurrentSystemTime();
		video_pause_time_ += current_time - pause_time_;

		video_cond_.notify_all();
		audio_cond_.notify_all();
	}
	else {
		// 记录暂停时间
		pause_time_ = GetCurrentSystemTime();
	}
}

void Player::AudioPlayThread() {
	SDL_AudioSpec audioSpec;
	audioSpec.freq = audio_codec_ctx_->sample_rate;
	audioSpec.format = AUDIO_F32;
	audioSpec.channels = 2;
	audioSpec.silence = 0;
	audioSpec.samples = 1024;
	audioSpec.callback = Player::AudioCallback;
	audioSpec.userdata = this;
	SDL_OpenAudio(&audioSpec, nullptr);
	SDL_PauseAudio(0);

	while (!quit_) {
		std::unique_lock<std::mutex> lock(audio_mutex_);
		audio_cond_.wait(lock, [&] { return  quit_ || !paused_; });
		if (quit_) break;
		// 当音频暂停时，暂停音频回放
		if (paused_) {

			SDL_PauseAudio(1);
			audio_cond_.wait(lock, [&] { return quit_ || !paused_; });
			if (quit_) break;
			SDL_PauseAudio(0);
		}
	}
	SDL_CloseAudio();
}
