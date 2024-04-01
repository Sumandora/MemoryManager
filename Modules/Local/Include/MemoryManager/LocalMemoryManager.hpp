#ifndef MEMORYMANAGER_LOCALMEMORYMANAGER_HPP
#define MEMORYMANAGER_LOCALMEMORYMANAGER_HPP

#include "MemoryManager/ExternalMemoryManager.hpp"

namespace MemoryManager {

	class LocalMemoryManager : public ExternalMemoryManager {
	public:
		explicit LocalMemoryManager(Mode forceMode = Mode::WRITE);

		[[nodiscard]] bool doesRead() const override;
		[[nodiscard]] bool doesWrite() const override;

		[[nodiscard]] bool doesForceRead() const;
		[[nodiscard]] bool doesForceWrite() const;

		[[nodiscard]] std::uintptr_t allocate(std::uintptr_t address, std::size_t size, int protection) const override;
		void deallocate(std::uintptr_t address, std::size_t size) const override;
		void protect(std::uintptr_t address, std::size_t size, int protection) const override;

		void read(std::uintptr_t address, void *content, std::size_t length) const override;
		void write(std::uintptr_t address, const void *content, std::size_t length) const override;

		bool requiresPermissionsForReading() override;
		bool requiresPermissionsForWriting() override;

		using MemoryManager::read;
		using MemoryManager::write;
		using MemoryManager::allocate;
		using MemoryManager::deallocate;
		using MemoryManager::protect;
	};
}

#endif
