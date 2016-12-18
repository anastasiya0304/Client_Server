#include "webdav\client.hpp"
#include "ClientConfig.h"
#include "dirent.h"
#include "utils.h"

#include <locale>
#include <iostream>
#include <fstream>
#include <thread>
#include <algorithm>

using namespace std;

const string confRoot = "configs";

void downloadFiles(ClientConfig& conf, WebDAV::Client * client, string serverpath = "") {
	string localPath = conf.localPathFromServer(serverpath);
	//CreateDirectory(savedir.c_str(), NULL);
	//replace(savedir.begin(), savedir.end(), '/', '\\');
	CreateDirectory(localPath.c_str(), NULL);
	auto list = client->list(utf8_encode(serverpath));
	//Перебираем всё на диске
	for (auto it = list.begin(), end = list.end(); it != end; it++) {
		//Смотрим, папка это или файл
		string objName = toString(utf8_decode(*it));
		string objPath = serverpath + objName;
		if (client->is_dir(utf8_encode(objPath))) { //Если папка, то добавляем её к пути и рекурсивно вызываем эту же функицию для полученного пути
			if (objPath[objPath.size() - 1] == '/')
				objPath = objPath.substr(0, objPath.size() - 1);
			downloadFiles(conf, client, objPath);
		} else {
			string objLocalPath = conf.localPathFromServer(objPath);
			cout << conf.login << ": " << "загрузка " << objPath << endl;
			//Получаем размер файла
			string utf8path = utf8_encode(objPath);
			auto info = client->info(utf8path);
			unsigned long long size = stoll(info["size"]);
			char * buffer = new char[size];
			//Загружаем файл в буффер
			if (!client->download_to(utf8path, buffer, size)) {
				cout << conf.login << ": " << "не удалось загрузить " << objPath << endl;
				continue;
			}
			XOR(buffer, size, conf.password.c_str(), conf.password.size());
			ofstream file(objLocalPath, ios::binary);
			file.write(buffer, size);
			file.close();
			client->clean(utf8path);
		}
	}
}

void startThread(ClientConfig conf) {
	cout << conf.login << ": " << " поток запущен" << endl;
	map<string, string> options =
	{
		{ "webdav_hostname", conf.addr },
		{ "webdav_login",    conf.login },
		{ "webdav_password", conf.password }
	};
	WebDAV::Client * client = WebDAV::Client::Init(options);

	if (!client->check()) {
		cout << conf.login << ": " << " не удалось подключиться к серверу" << endl;
		system("pause");
		return;
	}
	cout << conf.login << ": " << " подключение установлено" << endl;
	downloadFiles(conf, client);
}

int main() {
	setlocale(LC_ALL, "RUS");
	
	vector<thread> pool;

	printDivisor(string("Создание потоков"));
	//Ищем все конифиги клиентов, создаём потоки и передаём каждому потоку путь на 
	DIR * dir;
	dirent * ent;
	string login, password, addr;
	if ((dir = opendir(confRoot.c_str())) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			if(ent->d_type == DT_REG) {
				string path = confRoot + "\\" + ent->d_name;
				cout << "Создаём поток для " << login << endl;
				pool.push_back(thread(startThread, ClientConfig::read(path)));
			}
		}
		closedir(dir);
	}

	//Ждём окончания выполнения всех потоков
	for (vector<thread>::iterator it = pool.begin(), end = pool.end(); it != end; it++) {
		it->join();
	}

	system("pause");
	return 0;
}