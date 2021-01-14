#pragma once

#define MTYPE float
#include <xmath.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>

#include <unordered_map>
#include <initializer_list>
#include <functional>
#include <thread>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <algorithm>


namespace g {

struct net
{
	template<typename T>
	struct host
	{
		host(short port)
		{
			listen_sock = socket(AF_INET, SOCK_STREAM, 0);
			struct sockaddr_in name = {};
			name.sin_family      = AF_INET;
			name.sin_port        = htons(port);
			name.sin_addr.s_addr = htonl(INADDR_ANY);

			if (listen_sock < 0)
			{
				throw std::runtime_error("listen sock creation failed");
			}

			// allow port reuse for quicker restarting
			int use = 1;
			if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEPORT, (char*)&use, sizeof(use)))
			{
				close(listen_sock);
				throw std::runtime_error("Setting SO_REUSEPORT to listen socket failed");
			}

			// bind the listening sock to port number
			if (bind(listen_sock, (const struct sockaddr*)&name, sizeof(name)))
			{
				close(listen_sock);
				throw std::runtime_error("listen sock bind failed");
			}

			// begin listening
			if(listen(listen_sock, 1))
			{
				close(listen_sock);
				throw std::runtime_error("listen sock listen failed");
			}

		    std::thread listener([&] {
		    	int max_sock = listen_sock;
		    	fd_set rfds;
		    	FD_ZERO(&rfds);
		    	FD_SET(listen_sock, &rfds);

		    	// for each client socket check connection status and set it
		    	for (auto& pair : sockets)
		    	{
		    		int sock = pair.first;

		    		// check the connection status of the client socket
		    		char c;
		    		switch (recv(sock, &c, 1, MSG_PEEK))
		    		{
		    			case -1:
		    			case 0:
		    				close(sock);
		    				sockets.erase(sock);
		    				break;
		    			default: break;
		    		}

		    		FD_SET(sock, &rfds);
		    		max_sock = std::max(sock, max_sock);
		    	}

		    	switch (select(max_sock + 1, &rfds, NULL, NULL, NULL))
		    	{
		    		case -1: { throw std::runtime_error("Select error"); }
		    		case 0: { throw std::runtime_error("Timeout"); }
		    		default:
		    		{
		    			// check all client sockets to see if any have messages
		    			for (auto& pair : sockets)
		    			{
		    				if (!FD_ISSET(pair.first, &rfds)) { continue; }

		    				on_packet(pair.first, pair.second);
		    			}

		    			if (FD_ISSET(listen_sock, &rfds))
		    			{
		    				struct sockaddr_in client_name = {};
		    				socklen_t client_name_len = 0;
		    				auto sock = accept(
								listen_sock,
								(struct sockaddr*)&client_name,
								&client_name_len
							);

		    				sockets.emplace(sock, {});

		    				on_connection(sock, sockets[sock]);
		    			}
		    		}
		    	}
		    });
		}
		~host();

		std::function<void(int socket, T& client)> on_connection;
		std::function<int(int socket, T& client)> on_packet;
		std::unordered_map<int, T> sockets;
		int listen_sock;
	};
};

struct core
{
	struct opts
	{

	};


	virtual bool initialize () { return true; }

	virtual void update (float dt) { }



	void start(const core::opts& opts);

	bool running = false;
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
