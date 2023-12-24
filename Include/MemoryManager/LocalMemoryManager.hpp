#ifndef MEMORYMANAGER_LOCALMEMORYMANAGER_HPP
#define MEMORYMANAGER_LOCALMEMORYMANAGER_HPP

#include "ExternalMemoryManager.hpp"

namespace MemoryManager {

	class LocalMemoryManager : public ExternalMemoryManager {
	public:
		explicit LocalMemoryManager(Mode mode = Mode::WRITE);

		[[nodiscard]] bool isRead() const override;
		[[nodiscard]] bool isWrite() const override;

		void read(std::uintptr_t address, void *content, std::size_t length) const override;
		void write(std::uintptr_t address, const void *content, std::size_t length) const override;
	};
}

#endif
