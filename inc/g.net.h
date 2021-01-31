#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/tcp.h>
#include <netdb.h>

namespace g
{
struct net
{
	struct msg
	{
		virtual void to_network() = 0;
		virtual void to_machine() = 0;
	};

	/**
	 * @brief      A host object is used as a manager of network connections
	 * this includes listening for new connections, and incomming messages.
	 * When a connection is established a new instance of the template param
	 * T is instantiated and paired to that connection
	 *
	 * @tparam     T     Class which will be instantiated for each connection.
	 */
	template<typename T>
	struct host
	{
		std::function<void(int socket, T& client)> on_connection;
		std::function<void(int socket, T& client)> on_disconnection;
		std::function<int(int socket, T& client)> on_packet;
		std::unordered_map<int, T> sockets;
		std::thread listen_thread;
		int listen_socket;

		host() = default;
		~host()
		{
			close(listen_socket);
		}

		/**
		 * @brief      Calling this method will bind a socker for accepting connections
		 * to the provided port, then start a seperate thread to manage data received
		 * from clients, new connections, and disconnection events. This method can
		 * throw std::runtime_error exceptions for a number of reasons.
		 *
		 * @param[in]  port  The port to listen on
		 */
		void listen(short port)
		{
			listen_socket = ::socket(AF_INET, SOCK_STREAM, 0);
			struct sockaddr_in name = {};
			name.sin_family      = AF_INET;
			name.sin_port        = htons(port);
			name.sin_addr.s_addr = htonl(INADDR_ANY);

			if (listen_socket < 0)
			{
				throw std::runtime_error("listen sock creation failed");
			}

			// allow port reuse for quicker restarting
#ifdef __linux__
			int use = 1;
			if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEPORT, (char*)&use, sizeof(use)))
			{
				close(listen_socket);
				throw std::runtime_error("Setting SO_REUSEPORT to listen socket failed");
			}
#endif

			// bind the listening sock to port number
			if (bind(listen_socket, (const struct sockaddr*)&name, sizeof(name)))
			{
				close(listen_socket);
				throw std::runtime_error("listen sock bind failed");
			}

			// begin listening
			if(::listen(listen_socket, 1))
			{
				close(listen_socket);
				throw std::runtime_error("listen sock listen failed");
			}

			listen_thread = std::thread([&]{
				while(true)
				{
					update();
				}
			});
		}

		void update()
		{
			int max_sock = listen_socket;
			fd_set rfds;
			FD_ZERO(&rfds);
			FD_SET(listen_socket, &rfds);

			// for each client socket check connection status and set it
			for (auto& pair : sockets)
			{
				int sock = pair.first;

				FD_SET(sock, &rfds);
				max_sock = std::max(sock, max_sock);
			}

			switch (select(max_sock + 1, &rfds, NULL, NULL, NULL))
			{
				case -1: { /* select error, possible disconnect */ }
				case 0: { /* timeout occured */ }
				default:
				{
					std::vector<int> disconnected_socks;

					// check all client sockets to see if any have messages
					for (auto& pair : sockets)
					{
						auto sock = pair.first;
						if (!FD_ISSET(sock, &rfds)) { continue; }

						// check the connection status of the client socket
						// otherwise, pass the message over to the lambda
						char c;
						switch (recv(sock, &c, 1, MSG_PEEK))
						{
							case -1:
							case 0:
								// these errors only mean there was nothing for us to
								// read at this moment. Not that the connection is
								// broken
								if (errno == EAGAIN || errno == EWOULDBLOCK) { break; }

								on_disconnection(sock, sockets[sock]);
								close(sock);
								disconnected_socks.push_back(sock);

								// if the last connection just dropped, just return.
								if (sockets.size() == 0) { return; }
								break;
							default:
								on_packet(sock, pair.second);
								break;
						}
					}

					// clean up those that have disconnected
					for (auto sock : disconnected_socks)
					{
						sockets.erase(sock);
					}

					if (FD_ISSET(listen_socket, &rfds))
					{
						struct sockaddr_in client_name = {};
						socklen_t client_name_len = 0;
						auto sock = accept(
							listen_socket,
							(struct sockaddr*)&client_name,
							&client_name_len
						);

#ifdef __linux__
						int one = 1, five = 5;
						setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one));
						setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &five, sizeof(five));
						setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &one, sizeof(one));
						setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &five, sizeof(five));
#endif
						sockets[sock] = {};

						on_connection(sock, sockets[sock]);
					}
				}
			}
		}
	};

	struct client
	{
		std::function<int(int socket)> on_packet;
		std::function<void(int socket)> on_disconnection;
		std::thread listen_thread;
		int socket;
		volatile bool is_connected = false;

		bool connect(const std::string& hostname, short port)
		{
			// resolve host
			auto host = gethostbyname(hostname.c_str());
			if (host == nullptr)
			{
				return false;
			}

			struct sockaddr_in host_addr = {};
			// fill in host_addr with resolved info
			bcopy(
				(char *)host->h_addr_list[0],
				(char *)&host_addr.sin_addr.s_addr,
				host->h_length
			);
			host_addr.sin_port   = htons(port);
			host_addr.sin_family = AF_INET;

			socket = ::socket(AF_INET, SOCK_STREAM, 0);
			// fcntl(socket, F_SETFL, O_NONBLOCK);

			auto res = ::connect(socket, (const sockaddr*)&host_addr, sizeof(host_addr));

#ifdef __linux__
			int one = 1, five = 5;
			setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one));
			setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &five, sizeof(five));
			setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &one, sizeof(one));
			setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &five, sizeof(five));
#endif

			if (0 == res)
			{
				is_connected = true;
				return true;
			}

			// otherwise, cleanup and return false.
			close(socket);
			return false;
		}


		void listen()
		{
			if (listen_thread.joinable()) { listen_thread.join(); }
			listen_thread = std::thread([&]{
				while(is_connected)
				{
					update();
				}
			});
		}


		void update()
		{
			char c;
			switch (recv(socket, &c, sizeof(c), MSG_PEEK))
			{
				case -1:
				case 0:
					// these errors only mean there was nothing for us to
					// read at this moment. Not that the connection is
					// broken
					if (errno == EAGAIN || errno == EWOULDBLOCK) { break; }

					is_connected = false;
					on_disconnection(socket);
					close(socket);
					socket = -1;
					break;
				default:
					on_packet(socket);
					break;
			}
		}
	};
};
}