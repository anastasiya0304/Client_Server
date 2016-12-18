#include "webdav\client.hpp"
#include "utils.h"
#include "dirent.h"
#include "ClientConfig.h"

#include <locale>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <thread>
#include "md5.h"

using namespace std;

ClientConfig * config;
map<string, string> hashes;
string password; //будет использоваться для шифрования файла

bool checkAndRemember(string& path) {
	MD5 md5;

	ifstream fileReader(path, /**/ios::binary);
	char buffer[512];
	while (fileReader.read(buffer, 512))
		md5.update(buffer, 512);

	md5.finalize();
	string fileHash = md5.hexdigest();
	map<string, string>::iterator finded = hashes.find(path);
	if (finded != hashes.end()) {
		if (finded->second == fileHash)
			return false;
	}
	hashes[path] = fileHash;
	return true;
}

void syncDir(string dirpath, WebDAV::Client * client) {
	string serverpath = config->serverPathFromLocal(dirpath);
	client->create_directory(utf8_encode(serverpath));
	DIR * dir;
	dirent * ent;
	cout << serverpath << endl;
	if ((dir = opendir(dirpath.c_str())) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			switch (ent->d_type) {
			case DT_DIR:
				if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
					continue;
				syncDir(dirpath + "\\" + ent->d_name, client);
				break;
			case DT_REG:
				string filepath = dirpath + "\\" + ent->d_name;
				string fileserverpath = config->serverPathFromLocal(filepath);
				cout << "файл (" << filepath << "): ";
				//Проверяем файл и если он обновился загружаем в облако
				if (checkAndRemember(filepath)) {
					cout << "шифруем и загружаем в облако" << endl;

					//Читаем файл
					ifstream uploadFile(filepath, ifstream::binary);

					//Две следующие строчки определяют размер файла
					uploadFile.seekg(0, uploadFile.end);
					unsigned long fileSize = uploadFile.tellg();
					uploadFile.seekg(0, uploadFile.beg);
					char * buffer = new char[fileSize];
					uploadFile.read(buffer, fileSize);
					uploadFile.close();

					//Шифруем файл по алгоритму XOR
					XOR(buffer, fileSize, password.c_str(), password.size());
					/*
					Заметка:
					client->upload_from не годится для больших файлов
					Причины не понятны:
					1. client->upload работает нормально
					2. Проблема возникает на 16 385-байте (2^14 + 1), в связи с этим подозрение, что ошибся разработчик webdav, выделив 2^14 байт под буфер
					Скорее всего, баг WebDAV клиента

					Решение:
					Создаём tmp файл, загружаем его через метод upload, потом удаляем...
					*/
					string tmppath = filepath + ".cloudproject.tmp";
					ofstream test(tmppath, ios::binary);
					test.write(buffer, fileSize);
					test.close();

					if (!client->upload(utf8_encode(fileserverpath), tmppath)) {
						cout << "Не удалось загрузить файл на сервер" << endl;
						hashes.erase(filepath);
					}
					remove(tmppath.c_str());
					delete[] buffer;
				}
				else
					cout << "игнор" << endl;
				break;
			}
		}
		closedir(dir);
	}
	else {
		cout << "Ошибка! не удалось прочитать " << dirpath << endl;
	}
}

int main() {
	setlocale(LC_ALL, "RUS");
	string addr, login, syncdir;
	ifstream confStream("client.conf");
	if (!confStream) {
		cout << "Отсутствует файл client.conf" << endl;
		system("pause");
		exit(0);
	}
	std::getline(confStream, addr); //Считываем адрес сервера
	std::getline(confStream, login); //Считываем логин
	std::getline(confStream, password); //Считываем пароль
	std::getline(confStream, syncdir); //Считываем папку из которой будем брать файлы
	confStream.close();
	printDivisor(string("Параметры клиента"));
	cout << "адрес сервера: " << addr << endl;
	cout << "логин: " << login << endl;
	cout << "пароль: " << password << endl;
	cout << "рабочая папка: " << syncdir << endl;

	config = new ClientConfig(addr, login, password, syncdir);

	map<string, string> options =
	{
		{ "webdav_hostname", addr },
		{ "webdav_login",    login },
		{ "webdav_password", password }
	};
	WebDAV::Client * client = WebDAV::Client::Init(options);

	if (!client->check()) {
		cout << "Не удалось подключиться к серверу" << endl;
		system("pause");
		exit(0);
	}
	printEmpty(2);
	printDivisor(string("Соединение успешно установлено"));
	printEmpty();

	//Читаем все хэши файлов

	ifstream uploadedStream("uploaded");
	string bufstr;
	while (std::getline(uploadedStream, bufstr)) {
		int divisor = bufstr.find_first_of(':');
		hashes[bufstr.substr(0, divisor)] = bufstr.substr(divisor + 1);
	}
	uploadedStream.close();

	//Рекурсивно роходимся по папкам и передаём новые файлы в облако

	syncDir(string(config->workdir), client);

	//Запоминаем новые + старые хэши в файле

	ofstream uploadedSaveStream("uploaded");
	map<string, string>::iterator end = hashes.end();
	end--;
	map<string, string>::iterator it = hashes.begin();
	for (; it != end; it++) {
		uploadedSaveStream << it->first << ":" << it->second << endl;
	}
	uploadedSaveStream << it->first << ":" << it->second;
	uploadedSaveStream.flush();
	uploadedSaveStream.close();

	delete config;
	system("pause");

	return 0;
}