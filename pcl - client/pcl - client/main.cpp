
#include <iostream>
#pragma comment( lib, "ws2_32.lib" )
#include <Windows.h>
#include <conio.h>
#include <vector>
#include <fstream>
#include"PCL\matrix.h"

#define PORT 777
#define SERVERADDR "127.0.0.1"
const char* return_message = "message was sent";

matrix get_matrix(SOCKET& my_sock);
void send_matrix(SOCKET& my_sock, matrix& mtrx);
void output_matrix(const matrix& mtrx);
matrix input_matrix(const char* file_name);

int main(int argc, char* argv[])
{
	//setlocale(LC_ALL, "Russian");
	const int n_buffer = 1001;
	char buff[n_buffer];
	std::cout << "Client for multiplication" << std::endl;

	// инициализация библиотеки Winsock
	if (WSAStartup(0x202, (WSADATA *)&buff[0]))
	{
		printf("WSAStart error %d\n", WSAGetLastError());
		return -1;
	}

	// создание сокета
	SOCKET my_sock;
	my_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (my_sock < 0)
	{
		printf("Socket() error %d\n", WSAGetLastError());
		return -1;
	}

	// установка соединения
	sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(PORT);
	HOSTENT *hst;
	if (inet_addr(SERVERADDR) != INADDR_NONE)
		dest_addr.sin_addr.s_addr = inet_addr(SERVERADDR);
	else
		if (hst = gethostbyname(SERVERADDR))
			((unsigned long *)&dest_addr.sin_addr)[0] =
			((unsigned long **)hst->h_addr_list)[0][0];
		else
		{
			printf("Invalid address %s\n", SERVERADDR);
			closesocket(my_sock);
			WSACleanup();
			return -1;
		}

	// пытаемся установить соединение 
	if (connect(my_sock, (sockaddr *)&dest_addr,
		sizeof(dest_addr)))
	{
		printf("Connect error %d\n", WSAGetLastError());
		return -1;
	}


	matrix a = input_matrix("matrix/matrix_a.txt");
	matrix b = input_matrix("matrix/matrix_b.txt");

	send_matrix(my_sock, a);
	send_matrix(my_sock, b);
	matrix c = get_matrix(my_sock);

	output_matrix(c);

	closesocket(my_sock);
	WSACleanup();
	return -1;
}


matrix input_matrix(const char* file_name) {
	std::ifstream in(file_name, std::ios_base::in | std::ios_base::binary);
	int n = 1;
	int m = 0;

	int m_cur = 1;
	while (in) {
		char ch;
		in.get(ch);
		if (ch == ' ') ++m_cur;
		else if (ch == '\n') { ++n; m = m_cur; m_cur = 1; }
	}

	matrix mtrx(n, m);

	in.close();
	in.open(file_name, std::ios_base::in | std::ios_base::binary);

	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < m; ++j) {
			int x;
			in >> x;
			mtrx.elements[i][j] = x;

		}
	}

	in.close();
	return mtrx;
}


void output_matrix(const matrix& mtrx) {
	std::ofstream out("matrix/matrix_c.txt", std::ios_base::trunc | std::ios_base::binary);
	
	int n = mtrx.n;
	int m = mtrx.m;
	
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < m; ++j) {
			out << mtrx.elements[i][j];
			if (j != m - 1) out << " ";
		}
		if (i != n - 1) out << "\n";
	}

	out.close();
}




matrix get_matrix(SOCKET& my_sock) {
	const int n_buf = 1001;
	char buff[n_buf];
	
	int bytes_recv;
	int index_i = 0;
	int index_j = 0;
	bool flag = false;
	std::vector<std::vector<int>> v;

	int n;
	int m;

	send(my_sock, return_message, strlen(return_message) + 1, 0);

	while (true)
	{
		bytes_recv = recv(my_sock, buff, sizeof(buff), 0);

		if (bytes_recv == NULL || bytes_recv == EOF)
			break;

		if (bytes_recv == (sizeof(int) * 2 + 2) && buff[sizeof(int) * 2] == 's') {
			n = *((int*)&buff[0]);
			m = *((int*)&buff[sizeof(int)]);

			std::cout << "download is starting" << std::endl;
			std::cout << "n = " << n << "     " << "m = " << m << std::endl << std::endl;
			v.resize(n);
			for (int i = 0; i < n; ++i) v[i].resize(m);

			flag = true;
		}
		else if (flag) {
			char* ch = &buff[0];
			for (int i = 0; i < bytes_recv - 1; i += sizeof(int)) {
				int* number = (int*)ch;

				v[index_i][index_j++] = *number;
				if (index_j >= m) { ++index_i; index_j = 0; }
				ch += sizeof(int);
			}
			if (index_i >= n) { 
				break;
			}
		}
	}

	matrix new_matrix(n,m);
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < m; ++j)
			new_matrix.elements[i][j] = v[i][j];
	}
	return new_matrix;
}

void send_matrix(SOCKET& my_sock, matrix& mtrx) {
	const int n_buffer = 1001;
	char buff[n_buffer];

	const int m = mtrx.m;
	const int n = mtrx.n;

	char size[sizeof(int) * 2 + 2];
	char* c_n = (char*)&n;
	char* c_m = (char*)&m;

	for (int i = 0; i < sizeof(int); ++i) size[i] = c_n[i];
	for (int i = 0; i < sizeof(int); ++i) size[i + sizeof(int)] = c_m[i];
	size[sizeof(int) * 2] = 's';
	size[sizeof(int) * 2 + 1] = '\0';
	
	recv(my_sock, &buff[0], sizeof(buff) - 1, 0);
	send(my_sock, &size[0], 2 * sizeof(int) + 2, 0);

	int size_buffer = 0;

	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < m; ++j) {
			char* ch;
			ch = (char*)(&mtrx.elements[i][j]);

			if (size_buffer >= n_buffer - 1) {
				for (int k = size_buffer; k < n_buffer; ++k)
					buff[k] = '\0';

				send(my_sock, &buff[0], n_buffer, 0);
				size_buffer = 0;
				j--;
				if (j < 0) { --i; j = m - 1; }
			}
			else {
				for (int k = 0; k < sizeof(int); ++k)
					buff[size_buffer + k] = ch[k];
				size_buffer += sizeof(int);
			}
		}
	}

	if (size_buffer > 0) {
		for (int k = size_buffer; k < size_buffer + 1; ++k)
			buff[k] = '\0';

		send(my_sock, &buff[0], size_buffer + 1, 0);
	}
}