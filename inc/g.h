#pragma once

#define MTYPE float
#include <GLFW/glfw3.h>
#include <xmath.h>
#include <openssl/sha.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>
#include <assert.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <initializer_list>
#include <functional>
#include <thread>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <algorithm>


namespace g {

struct split
{
public:
	struct it 
	{
	public:
		it(std::string &str, std::string delim, size_t pos);

		void operator++();

		bool operator!=(it &i);

		std::string operator*();

	protected:
		std::string &_str;
		std::string _delim;
		size_t _pos, _next_pos;
	};

	/**
	 * @brief iterable class that splits a string into tokens
	 *        separated by occurences of delim
	 * @param str String whose tokens we want to iterate over.
	 * @param delim String used as delimiter to create tokens.
	 */
	split(std::string &str, std::string delim);

	it begin();

	it end();

private:
	std::string &_str;
	std::string _delim;
};


std::string base64_encode(uint8_t const* buf, size_t len);


struct core
{
	struct opts
	{
		const char* name;
		struct {
			bool display = true;
			size_t width = 640;
			size_t height = 480;
		} gfx;
	};

	/**
	 * @brief      Overriding the initialize function gives you the chance
	 * to run setup and initalization code before the update loop begins.
	 *
	 * @return     true if the initialization completed successfully, false
	 * otherwise.
	 */
	virtual bool initialize () { return true; }

	/**
	 * @brief      The update function is effectively the main loop of your
	 * game. Override this to run your game logic every frame/tick
	 *
	 * @param[in]  dt    Amount of time elapsed since the previous call to
	 * update had finished.
	 */
	virtual void update (float dt) { }

	/**
	 * @brief      Calling start will first call initialize, do any additional
	 * setup that might have been indicated by the opts parameter, then starts
	 * the update loop until the `running` flag is set to false.
	 *
	 * @param[in]  opts  Special configuration options. See the declaration of
	 * core::opts for more details.
	 */
	void start(const core::opts& opts);

	volatile bool running = false;
};


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
		std::unordered_set<int> senders;
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
			int use = 1;
			if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEPORT, (char*)&use, sizeof(use)))
			{
				close(listen_socket);
				throw std::runtime_error("Setting SO_REUSEPORT to listen socket failed");
			}

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

		bool is_client_ws(int sock)
		{
			char buf[1024] = {};
			std::unordered_map<std::string, std::string> headers;
			auto bytes = recv(sock, buf, sizeof(buf), MSG_PEEK | MSG_DONTWAIT);
		
			auto lines = std::string(buf);
			int i = 0;
			for (auto line : g::split(lines, "\r\n"))
			{
				std::cerr << "[" << line << "]\n"; 
				if (i == 0 && std::string::npos == line.find("GET")) { return false; }
				else
				{
					std::string key;
					for (auto part : g::split(line, ": "))
					{
						if (key.length() == 0) { key = part; }
						else
						{
							headers[key] = part;
							break;
						}
					}
				}
				i++;
			}

			const auto sec_key = "Sec-WebSocket-Key";
			if (headers.count(sec_key))
			{
				char* next = buf;
				auto key = headers[sec_key] + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
				unsigned char hash[SHA_DIGEST_LENGTH];
				SHA1((const unsigned char*)key.c_str(), key.length(), hash);
				auto sec_accept = g::base64_encode(hash, SHA_DIGEST_LENGTH);

				{
					std::string ex_key = "dGhlIHNhbXBsZSBub25jZQ==258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
					unsigned char ex_sha[SHA_DIGEST_LENGTH];
					SHA1((const unsigned char*)ex_key.c_str(), ex_key.length(), ex_sha);
					auto ex_accept = g::base64_encode(ex_sha, SHA_DIGEST_LENGTH);

					assert(ex_accept == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
				}

				next += sprintf(next, "HTTP/1.1 101 Switching Protocols\r\n");
				next += sprintf(next, "Upgrade: websocket\r\n");
				next += sprintf(next, "Connection: %s\r\n", headers["Connection"].c_str());
				next += sprintf(next, "Sec-WebSocket-Protocol: %s\r\n", headers["Sec-WebSocket-Protocol"].c_str());
				next += sprintf(next, "Sec-WebSocket-Extensions: %s\r\n", headers["Sec-WebSocket-Extensions"].c_str());
				next += sprintf(next, "Sec-WebSocket-Accept: %s\r\n", sec_accept.c_str());
				next += sprintf(next, "Content-Length: 0\r\n");
				next += sprintf(next, "\r\n");
				printf(">> RESPONSE\n");
				write(1, buf, (next - buf));
				send(sock, buf, (next - buf), 0);
				std::cout << "sha: " << hash << "\n"; 
			}
			else
			{
				return false;
			}

			// purge the http request we just got
			read(sock, buf, bytes);

			return true;
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
						switch (recv(sock, &c, 1, MSG_PEEK | MSG_DONTWAIT))
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
								if (senders.count(sock) == 0)
								{ 
									// this socket hasn't sent anything yet
									// lets check to see if it's a WS.
									if (is_client_ws(sock))
									{
										senders.insert(sock); 
										break;
									}
								}
								on_packet(sock, pair.second);
								senders.insert(sock);
								break;
						}
					}

					// clean up those that have disconnected
					for (auto sock : disconnected_socks)
					{
						sockets.erase(sock);
						senders.erase(sock);
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
				(char *)host->h_addr,
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


/**
 * @brief Simple, very low overhead datastructure for storing unordered data.
 * @tparam T Type whose instances will be stored in the bounded_list.
 * @tparam CAP Maximum number of elements that may be stored in the list.
 */
template<typename T, size_t CAP>
struct bounded_list
{
	struct itr
	{
		itr(T* data, size_t pos, size_t len) { _data = data; _pos = pos; _len = len; }

		bool operator!=(const itr& o) { return _data != o._data || _pos != o._pos || _len != o._len; }
		itr& operator++() { _pos++; return *this; }
		T& operator*() { return _data[_pos]; }

	protected:
		T* _data = nullptr;
		size_t _pos = 0;
		size_t _len = 0;
	};

	bounded_list()
	{
		_size = 0;
	}

	bounded_list(std::initializer_list<T> items)
	{
		_size = 0;
		for (auto item : items)
		{
			push_back(item);
		}
	}

	bool emplace_back()
	{
		if (_size >= CAP) { return false; }

		_size++;

		return true;
	}

	/**
	 * @brief Append element to the end of the list.
	 * @param e Element to add to the bounded_list
	 * @return True if the bounded_list has enough space, False if there
	 *         wasn't enough space.
	 */
	bool push_back(T e)
	{
		if (_size >= CAP) { return false; }

		_list[_size++] = e;

		return true;
	}

	/**
	 * @brief Remove the last element from the list.
	 * @param e Set to the value of the element removed.
	 * @return False if the list is empty, True otherwise.
	 */
	bool pop_back(T& e)
	{
		if (_size <= 0) { return false; }

		e = _list[--_size];

		return true;
	}

	/**
	 * @brief Remove the last element from the list.
	 * @return False if the list is empty, True otherwise.
	 */
	bool pop_back()
	{
		if (_size <= 0) { return false; }

		--_size;

		return true;
	}

	/**
	 * @brief Get the last element of the list.
	 * @param e Set to the value of the last element.
	 * @return False if the list is empty, True otherwise.
	 */
	bool peek_back(T& e)
	{
		if (_size <= 0) { return false; }

		e = _list[_size - 1];

		return true;
	}

	/**
	 * @brief Get the last element of the list.
	 * @return Reference to the last element in the list.
	 * @note Invokes undefined behavior for empty lists.
	 */
	T& peek_back() { return _list[_size - 1]; }

	const T& peek_back_c() const { return _list[_size - 1]; }

	/**
	 * @brief Removes an element at a specific index.
	 * @param idx Index to element to remove from the list.
	 * @return False if the list is empty, or idx is outside of the
	 *         bounds True otherwise.
	 */
	bool remove_at(size_t idx)
	{
		if (idx >= _size) { return false; }
		if (_size == 0) { return false; }

		_list[idx] = _list[_size - 1];
		_size--;

		return true;
	}

	/**
	 * @brief Removes an element from the list.
	 * @return True if a matching element is found in the list
	 *         otherwise False.
	 * @note In the event of duplicate entries, only the first occurance
	 *       is removed.
	 */
	bool remove(T e)
	{
		for (size_t i = 0; i < _size; i++)
		{
			if (e == _list[i]) { return remove_at(i); }
		}

		return false;
	}

	/**
	 * @brief      Clears all elements from the object.
	 */
	void clear()
	{
		_size = 0;
	}

	/**
	 * @return Number of elements contained by the list.
	 */
	size_t size() const { return _size; }


	/**
	 * @brief      Get underlying storage array for the list
	 *
	 * @return     Return raw pointer to the start of the list
	 */
	T* list() { return _list; }

	itr begin() { return itr(_list, 0, _size); }
	itr end() { return itr(_list, _size, _size); }

	inline T& operator[](size_t idx) { return _list[idx]; }

	inline T& item(size_t idx) { return _list[idx]; }

	inline const T& item_c(size_t idx) const { return _list[idx]; }

	inline T* item_ptr(size_t idx) { return &_list[idx]; }

	inline T item(size_t idx) const { return _list[idx]; }
private:
	T _list[CAP];
	size_t _size;
};


} // namespace g
