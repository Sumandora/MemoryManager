#ifndef MEMORYMANAGER_LOCALMEMORYMANAGER_HPP
#define MEMORYMANAGER_LOCALMEMORYMANAGER_HPP

#include "MemoryManager/ExternalMemoryManager.hpp"
#include <sys/mman.h>

namespace MemoryManager {

	template<RWMode Mode>
	class LocalMemoryManager : public ExternalMemoryManager<Mode> {
	public:
		static constexpr bool DoesRead = true;
		static constexpr bool DoesWrite = true;

		static constexpr bool DoesForceRead = Mode == RWMode::READ || Mode == RWMode::READ_AND_WRITE;
		static constexpr bool DoesForceWrite = Mode == RWMode::WRITE || Mode == RWMode::READ_AND_WRITE;

		static constexpr bool RequiresPermissionsForReading = !DoesForceRead;
		static constexpr bool RequiresPermissionsForWriting = !DoesForceWrite;
		static constexpr bool IsRemoteAddressSpace = DoesForceRead; // Setting this to true would indicate that memcpy would have the same effect as using read

		explicit LocalMemoryManager()
			: ExternalMemoryManager<Mode>("self")
		{
		}

	protected:
		static constexpr int protectionFlagsToPosix(ProtectionFlags flags) {
			int prot = 0;

			if(flags.isReadable())
				prot |= PROT_READ;
			if(flags.isWriteable())
				prot |= PROT_WRITE;
			if(flags.isExecutable())
				prot |= PROT_EXEC;

			return prot;
		}
	public:

		[[nodiscard]] std::uintptr_t allocate_impl(std::uintptr_t address, std::size_t size, ProtectionFlags protection) const
		{
			int flags = MAP_PRIVATE | MAP_ANONYMOUS;
			void* addr = reinterpret_cast<void*>(address);
			if(addr != nullptr)
				flags |= MAP_FIXED_NOREPLACE;
			void* res = mmap(addr, size, protectionFlagsToPosix(protection), flags, -1, 0);
			if(res == MAP_FAILED)
				throw std::runtime_error(strerror(errno));
			return reinterpret_cast<uintptr_t>(res);
		}
		void deallocate_impl(std::uintptr_t address, std::size_t size) const
		{
			int res = munmap(reinterpret_cast<void*>(address), size);

			if(res == -1)
				throw std::runtime_error(strerror(errno));
		}
		void protect_impl(std::uintptr_t address, std::size_t size, ProtectionFlags protection) const {
			int res = mprotect(reinterpret_cast<void*>(address), size, protectionFlagsToPosix(protection));

			if(res == -1)
				throw std::runtime_error(strerror(errno));
		}

		void read_impl(std::uintptr_t address, void *content, std::size_t length) const
		{
			if constexpr (!DoesForceRead)
				std::memcpy(content, reinterpret_cast<void*>(address), length);
			else
				ExternalMemoryManager<Mode>::read_impl(address, content, length);
		}
		void write_impl(std::uintptr_t address, const void *content, std::size_t length) const
		{
			if constexpr (!DoesForceWrite)
				std::memcpy(reinterpret_cast<void*>(address), content, length);
			else
				ExternalMemoryManager<Mode>::write_impl(address, content, length);
		}

		using ExternalMemoryManager<Mode>::allocate;
		using ExternalMemoryManager<Mode>::deallocate;
		using ExternalMemoryManager<Mode>::protect;
		using ExternalMemoryManager<Mode>::read;
		using ExternalMemoryManager<Mode>::write;
	};
}

#endif
