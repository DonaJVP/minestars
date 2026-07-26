/*
MineStars - MultiCraft - Minetest/Luanti
Copyright (C) 2025 Logiki, <donatto555@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "../threading/thread.h"
#include "randomchar.h"

#pragma once

//This allows multithreading funtions, as we go.

template<typename T, typename T_>
class QuickThreading: public Thread {
public:
	QuickThreading() = default;
	QuickThreading(std::function<T()> f, std::function<void(QuickThreading<T, T_>*, T)> cb = [](QuickThreading<T, T_> *o, T_) {}): Thread("QT-"+randomString(6)), func(f), finish(cb) {}
	~QuickThreading() = default;
	void *run() override {
		T_ result = func();
		finish(this, result);
		_delthis();
		return 0;
	} //This will return the args, which we will use later
	bool ready() { return _ready; }
private:
	std::function<T()> func;
	std::function<void(QuickThreading<T, T_> *o, T_)> finish;
	void _delthis() {
		//wait();
		_ready = true;
		delete this;

	}
	bool _ready = false;
	//T_ args;
};

