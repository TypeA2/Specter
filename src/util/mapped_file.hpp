#pragma once

#include <cstddef>

/* Memory-mapped file */
class mapped_file {
	void* _addr;
	size_t _size;

	public:
	explicit mapped_file(int fd);

	~mapped_file();

	[[nodiscard]] size_t size() const;

	template <typename T = void>
	T* get() const {
		return static_cast<T*>(_addr);
	}

	template <typename T = void>
	T* get_at(size_t off) const {
		return reinterpret_cast<T*>(static_cast<char*>(_addr) + off);
	}
};
