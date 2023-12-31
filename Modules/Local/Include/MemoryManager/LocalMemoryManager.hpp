#ifndef MEMORYMANAGER_LOCALMEMORYMANAGER_HPP
#define MEMORYMANAGER_LOCALMEMORYMANAGER_HPP

#include "MemoryManager/ExternalMemoryManager.hpp"

namespace MemoryManager {

	class LocalMemoryManager : public ExternalMemoryManager {
	public:
		explicit LocalMemoryManager(Mode mode = Mode::WRITE);

		[[nodiscard]] bool isRead() const override;
		[[nodiscard]] bool isWrite() const override;

		[[nodiscard]] void* allocate(std::uintptr_t address, std::size_t size, int protection) const override;
		void deallocate(std::uintptr_t address, std::size_t size) const override;

	protected:
		void read(std::uintptr_t address, void *content, std::size_t length) const override;
		void write(std::uintptr_t address, const void *content, std::size_t length) const override;
	};
}

#endif
