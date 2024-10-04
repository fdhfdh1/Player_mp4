// main.cpp
// Description: This file is the entry point for the program
// Author: �����
// Date: 2024-08-15
//#include <QApplication>
//#include <QFileDialog>

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include "Player.h"

int main(int argc, char* argv[]) {
	// �����ļ�
	QApplication a(argc, argv);

	// �����ļ�ѡ��Ի���
	QString file_name = QFileDialog::getOpenFileName(nullptr, QStringLiteral("ѡ����Ƶ�ļ�"), "", QStringLiteral("��Ƶ�ļ� (*.mp4 *.avi *.mkv)"));

	// ���ѡ�����ļ�
	if (!file_name.isEmpty()) {
		// ��QStringת��ΪC�ַ���
		QByteArray byte_array = file_name.toLocal8Bit();
		const char* file_name_cstr = byte_array.data();

		// ����������ʵ��
		Player player(file_name_cstr);
		player.InitializePlayer();
		player.StartPlayer();

		// SDL�¼�����
		
		SDL_Event e;
		bool running = true;
		while (running) {
			while (SDL_PollEvent(&e)) {
				if (e.type == SDL_QUIT) {
					running = false;
					player.StopPlayer();
				}
				else if (e.type == SDL_KEYDOWN) {
					if (e.key.keysym.sym == SDLK_SPACE) { // ���ո������
						player.TogglePause(); // �л�����/��ͣ״̬
					}
				}
			}
		}
	}
	else {
		// ���û��ѡ���ļ����˳�����
		return 0;
	}

	return a.exec();
}
