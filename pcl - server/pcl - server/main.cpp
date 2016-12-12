#include < iostream >
#pragma comment( lib, "ws2_32.lib" )

#include < Windows.h >
#include < conio.h >
#include <vector>
#include"PCL\matrix.h"

const char* return_message = "message was sent";
char* cbuff = "common buffer";
int nclients = 0;

#define PRINTNUSERS if (nclients)\
  printf("%d user on-line\n\n",nclients);\
  else printf("No User on line\n\n");
#define MY_PORT 777

matrix get_matrix(SOCKET& my_sock);
void send_matrix(SOCKET& my_sock, matrix& mtrx);
DWORD WINAPI WorkWithClient(LPVOID client_socket);


int main(int argc, char* argv[])
{
	char buff[1024];

	std::cout << "Server for multiplication" << std::endl;

	if (WSAStartup(0x0202, (WSADATA *)&buff[0]))
	{
		printf("Error WSAStartup %d\n",
			WSAGetLastError());
		return -1;
	}

	// создание сокета
	SOCKET mysocket;
	if ((mysocket = socket(AF_INET, SOCK_STREAM, 0))<0)
	{
		printf("Error socket %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	//связывание сокета с локальным адресом
	sockaddr_in local_addr;
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(MY_PORT);
	local_addr.sin_addr.s_addr = 0;
	// вызываем bind для связывания
	if (bind(mysocket, (sockaddr *)&local_addr, sizeof(local_addr))) {
		printf("Error bind %d\n", WSAGetLastError());
		closesocket(mysocket);
		WSACleanup();
		return -1;
	}

	// ожидание подключений
	if (listen(mysocket, 20))
	{
		printf("Error listen %d\n", WSAGetLastError());
		closesocket(mysocket);
		WSACleanup();
		return -1;
	}

	printf("Waiting for connection\n");

	// извлекаем сообщение из очереди
	SOCKET client_socket;    // сокет для клиента
	sockaddr_in client_addr;    // адрес клиента

	int client_addr_size = sizeof(client_addr);

	while ((client_socket = accept(mysocket, (sockaddr *)
		&client_addr, &client_addr_size)))
	{
		nclients++;

		HOSTENT *hst;
		hst = gethostbyaddr((char *)
			&client_addr.sin_addr.s_addr, 4, AF_INET);


		std::cout << std::endl << "new connect!"<<std::endl;
		PRINTNUSERS

			DWORD thID;
		CreateThread(NULL, NULL, WorkWithClient,
			&client_socket, NULL, &thID);
	}
	return 0;
}

DWORD WINAPI WorkWithClient(LPVOID client_socket) {
	SOCKET my_sock;
	my_sock = ((SOCKET *)client_socket)[0];

	matrix a = get_matrix(my_sock);
	matrix b = get_matrix(my_sock);
	std::cout << "calculation..." << std::endl << std::endl;
	matrix c = a*b;

	send_matrix(my_sock, c);

	nclients--;
	printf("disconnect\n"); PRINTNUSERS
	closesocket(my_sock);
	return 0;
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

	std::cout << "result matrix was sent" << std::endl << std::endl;
}


