// main.cpp
// Description: This file is the entry point for the program
// Author: 彭金龙
// Date: 2024-08-15
//#include <QApplication>
//#include <QFileDialog>

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include "Player.h"

int main(int argc, char* argv[]) {
	// 播放文件
	QApplication a(argc, argv);

	// 弹出文件选择对话框
	QString file_name = QFileDialog::getOpenFileName(nullptr, QStringLiteral("选择视频文件"), "", QStringLiteral("视频文件 (*.mp4 *.avi *.mkv)"));

	// 如果选择了文件
	if (!file_name.isEmpty()) {
		// 将QString转换为C字符串
		QByteArray byte_array = file_name.toLocal8Bit();
		const char* file_name_cstr = byte_array.data();

		// 创建播放器实例
		Player player(file_name_cstr);
		player.InitializePlayer();
		player.StartPlayer();

		// SDL事件处理
		
		SDL_Event e;
		bool running = true;
		while (running) {
			while (SDL_PollEvent(&e)) {
				if (e.type == SDL_QUIT) {
					running = false;
					player.StopPlayer();
				}
				else if (e.type == SDL_KEYDOWN) {
					if (e.key.keysym.sym == SDLK_SPACE) { // 检测空格键按下
						player.TogglePause(); // 切换播放/暂停状态
					}
				}
			}
		}
	}
	else {
		// 如果没有选择文件，退出程序
		return 0;
	}

	return a.exec();
}
