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
string password; //����� �������������� ��� ���������� �����

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
				cout << "���� (" << filepath << "): ";
				//��������� ���� � ���� �� ��������� ��������� � ������
				if (checkAndRemember(filepath)) {
					cout << "������� � ��������� � ������" << endl;

					//������ ����
					ifstream uploadFile(filepath, ifstream::binary);

					//��� ��������� ������� ���������� ������ �����
					uploadFile.seekg(0, uploadFile.end);
					unsigned long fileSize = uploadFile.tellg();
					uploadFile.seekg(0, uploadFile.beg);
					char * buffer = new char[fileSize];
					uploadFile.read(buffer, fileSize);
					uploadFile.close();

					//������� ���� �� ��������� XOR
					XOR(buffer, fileSize, password.c_str(), password.size());
					/*
					�������:
					client->upload_from �� ������� ��� ������� ������
					������� �� �������:
					1. client->upload �������� ���������
					2. �������� ��������� �� 16 385-����� (2^14 + 1), � ����� � ���� ����������, ��� ������ ����������� webdav, ������� 2^14 ���� ��� �����
					������ �����, ��� WebDAV �������

					�������:
					������ tmp ����, ��������� ��� ����� ����� upload, ����� �������...
					*/
					string tmppath = filepath + ".cloudproject.tmp";
					ofstream test(tmppath, ios::binary);
					test.write(buffer, fileSize);
					test.close();

					if (!client->upload(utf8_encode(fileserverpath), tmppath)) {
						cout << "�� ������� ��������� ���� �� ������" << endl;
						hashes.erase(filepath);
					}
					remove(tmppath.c_str());
					delete[] buffer;
				}
				else
					cout << "�����" << endl;
				break;
			}
		}
		closedir(dir);
	}
	else {
		cout << "������! �� ������� ��������� " << dirpath << endl;
	}
}

int main() {
	setlocale(LC_ALL, "RUS");
	string addr, login, syncdir;
	ifstream confStream("client.conf");
	if (!confStream) {
		cout << "����������� ���� client.conf" << endl;
		system("pause");
		exit(0);
	}
	std::getline(confStream, addr); //��������� ����� �������
	std::getline(confStream, login); //��������� �����
	std::getline(confStream, password); //��������� ������
	std::getline(confStream, syncdir); //��������� ����� �� ������� ����� ����� �����
	confStream.close();
	printDivisor(string("��������� �������"));
	cout << "����� �������: " << addr << endl;
	cout << "�����: " << login << endl;
	cout << "������: " << password << endl;
	cout << "������� �����: " << syncdir << endl;

	config = new ClientConfig(addr, login, password, syncdir);

	map<string, string> options =
	{
		{ "webdav_hostname", addr },
		{ "webdav_login",    login },
		{ "webdav_password", password }
	};
	WebDAV::Client * client = WebDAV::Client::Init(options);

	if (!client->check()) {
		cout << "�� ������� ������������ � �������" << endl;
		system("pause");
		exit(0);
	}
	printEmpty(2);
	printDivisor(string("���������� ������� �����������"));
	printEmpty();

	//������ ��� ���� ������

	ifstream uploadedStream("uploaded");
	string bufstr;
	while (std::getline(uploadedStream, bufstr)) {
		int divisor = bufstr.find_first_of(':');
		hashes[bufstr.substr(0, divisor)] = bufstr.substr(divisor + 1);
	}
	uploadedStream.close();

	//���������� ��������� �� ������ � ������� ����� ����� � ������

	syncDir(string(config->workdir), client);

	//���������� ����� + ������ ���� � �����

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