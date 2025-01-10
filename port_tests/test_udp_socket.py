import time
import socket


if __name__ == '__main__':
    # Множество адресов клиентов, которые подключаются к UDP серверу.
    clients_set = set()
    # Время "сна" потока сервера. Данный параметр необходим для эмуляции
    # задержек при ожидании клиентами входных данных.
    program_timeout_sec = 0.9

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_socket.bind(('127.0.0.1', 8080))

    while True:
        client_data = server_socket.recvfrom(512)
        # Если получены валидные данные от клиента
        if client_data:
            print(f'Got data from client: {client_data}')

            # Если клиент сообщил об отключении
            if client_data[0] == b'Disconnecting':
                # Удаление адреса очередного клиента из множества
                clients_set.remove(client_data[1])

                # Поскольку клиенты из примера начинают отключаться,
                # можно ускорить работу скрипта.
                program_timeout_sec = 0.3
            else:
                # В случае получения валидных данных от клиента необходимо
                # запомнить его адрес для отправки ответа
                clients_set.add(client_data[1])

            # Если во множестве адресов клиентов, подключённых к серверу есть
            # хотя бы один адрес
            if clients_set:
                # Для каждого клиента из множества выполнить отправку данных
                for client in clients_set:
                    server_socket.sendto(
                        str.encode(f'Server data for client {client}'), client
                    )
            # Если все клиенты отключились
            else:
                print('All clients disconnected!')
                break

        # Вызов метода для принудительного "сна" потока для эмуляции
        # задержек на сервере.
        time.sleep(program_timeout_sec)

    server_socket.close()
