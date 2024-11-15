#ifndef MEMORYMANAGER_LINUXMEMORYMANAGER_HPP
#define MEMORYMANAGER_LINUXMEMORYMANAGER_HPP

#include "MemoryManager/MemoryManager.hpp"

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <fstream>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/types.h>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include <variant>

namespace MemoryManager {
	template <bool Read, bool Write, bool Local>
	class LinuxMemoryManager;

	struct LinuxNamedData {
		std::string name; // may be a path

		bool deleted = false;
		bool special = false;
	};

	template <typename MemMgr, bool CanRead, bool Local>
	class LinuxRegion {
		const MemMgr* parent;
		std::uintptr_t address;
		std::size_t length;
		Flags flags;
		std::optional<LinuxNamedData> namedData;
		mutable std::unique_ptr<std::byte[]> cachedMemory;

	public:
		constexpr LinuxRegion(
			const MemMgr* parent,
			std::uintptr_t address,
			std::size_t length,
			Flags flags,
			std::optional<LinuxNamedData> namedData)
			: parent(parent)
			, address(address)
			, length(length)
			, flags(flags)
			, namedData(std::move(namedData))
		{
		}

		[[nodiscard]] std::uintptr_t getAddress() const
		{
			return address;
		}

		[[nodiscard]] std::size_t getLength() const
		{
			return length;
		}

		[[nodiscard]] Flags getFlags() const
		{
			return flags;
		}

		[[nodiscard]] std::optional<std::string> getPath() const
		{
			return namedData.and_then([](const LinuxNamedData& namedData) -> std::optional<std::string> {
				if (namedData.special)
					return std::nullopt;
				if (!namedData.name.starts_with('/'))
					return std::nullopt;
				return namedData.name;
			});
		}

		[[nodiscard]] std::optional<std::string> getName() const
		{
			return namedData.transform([](const LinuxNamedData& d) {
				const auto pos = d.name.rfind('/');
				return pos == std::string::npos || pos == d.name.size() ? d.name : d.name.substr(pos + 1);
			});
		}

		[[nodiscard]] bool isDeleted() const
		{
			return namedData.has_value() && namedData->deleted;
		}

		[[nodiscard]] bool isSpecial() const
		{
			return namedData.has_value() && namedData->special;
		}

		[[nodiscard]] bool doesUpdateView() const
			requires CanRead
		{
			if constexpr (Local)
				return flags.isReadable();
			return false;
		}

		[[nodiscard]] std::span<const std::byte> view(bool updateCache = false) const
			requires CanRead
		{
			if constexpr (Local)
				if (doesUpdateView() && !updateCache)
					return std::span{ reinterpret_cast<std::byte*>(getAddress()), getLength() };

			if (!cachedMemory || updateCache) {
				cachedMemory.reset(new std::byte[getLength()]);
				parent->read(getAddress(), cachedMemory.get(), getLength());
			}

			return std::span{ cachedMemory.get(), getLength() };
		}
	};

	template <bool Read, bool Write, bool Local = false>
	class LinuxMemoryManager {
		using Self = LinuxMemoryManager<Read, Write, Local>;

	public:
		static constexpr bool CanRead = Local || Read;
		static constexpr bool CanWrite = Local || Write;
		static constexpr bool RequiresPermissionsForReading = Local && !Read;
		static constexpr bool RequiresPermissionsForWriting = Local && !Write;

		using RegionT = LinuxRegion<Self, CanRead, Local>;

	private:
		std::string pid;

		MemoryLayout<RegionT> layout;

		using FileHandle = std::invoke_result_t<decltype([](const char* a1, int a2) { return ::open(a1, a2); }), const char*, int>;

		[[no_unique_address]] std::conditional_t<Read || Write, FileHandle, std::monostate> memInterface;

		static constexpr int INVALID_FILE_HANDLE = -1;

		static decltype(memInterface) openFileHandle(const std::string& pid)
		{
			if constexpr (!Read && !Write)
				return std::monostate{};
			else {
				int flag = 0;
				if constexpr (Read && Write)
					flag = O_RDWR;
				else if constexpr (Read)
					flag = O_RDONLY;
				else if constexpr (Write)
					flag = O_WRONLY;

				const FileHandle h = ::open(("/proc/" + pid + "/mem").c_str(), flag);
				if (h == INVALID_FILE_HANDLE)
					throw std::runtime_error(::strerror(errno));
				return h;
			}
		}

		static constexpr int flagsToPosix(Flags flags)
		{
			int prot = 0;

			if (flags.isReadable())
				prot |= PROT_READ;
			if (flags.isWriteable())
				prot |= PROT_WRITE;
			if (flags.isExecutable())
				prot |= PROT_EXEC;

			return prot;
		}

	public:
		explicit LinuxMemoryManager()
			requires Local
			: LinuxMemoryManager("self")
		{
		}

		explicit LinuxMemoryManager(pid_t pid)
			: LinuxMemoryManager(std::to_string(pid))
		{
		}

		explicit LinuxMemoryManager(const std::string& pid)
			: pid(pid)
			, memInterface(openFileHandle(pid))
		{
		}

		~LinuxMemoryManager()
		{
			if constexpr (Read || Write)
				close();
		}

		// Since one of the member variables is a file handle, these two operations are not possible
		LinuxMemoryManager(const LinuxMemoryManager& other) = delete;
		LinuxMemoryManager& operator=(const LinuxMemoryManager& other) = delete;

		void close()
			requires Read || Write
		{
			if (memInterface == INVALID_FILE_HANDLE)
				return;

			::close(memInterface);
			memInterface = INVALID_FILE_HANDLE;
		}

		void reopen()
			requires Read || Write
		{
			if (memInterface != INVALID_FILE_HANDLE)
				return;

			memInterface = openFileHandle(pid);
		}

		[[nodiscard]] const MemoryLayout<RegionT>& getLayout() const
		{
			return layout;
		}

		void update()
		{
			std::fstream fileStream{ "/proc/" + pid + "/maps", std::fstream::in };
			if (!fileStream) {
				throw std::exception{};
			}

			MemoryLayout<RegionT> newLayout{};
			for (std::string line; std::getline(fileStream, line);) {
				if (line.empty())
					continue; // ?

				std::uintptr_t begin = 0;
				std::uintptr_t end = 0;
				std::array<char, 3> perms{};
				std::string name;

				int offset = -1;

				// NOLINTNEXTLINE(cert-err34-c)
				(void)sscanf(line.c_str(), "%zx-%zx %c%c%c%*c %*x %*x:%*x %*x%n", &begin, &end, &perms[0], &perms[1], &perms[2], &offset);

				while (offset < line.length() && line[offset] == ' ')
					offset++;
				if (offset < line.length())
					name = line.c_str() + offset;

				Flags flags{ perms };

				bool deleted = false;
				bool special = false;
				if (!name.empty()) {
					constexpr static const char* deletedTag = " (deleted)";
					constexpr static std::size_t deletedTagLen = std::char_traits<char>::length(deletedTag);
					if (name.ends_with(deletedTag)) {
						deleted = true;
						name = name.substr(0, name.length() - deletedTagLen);
					}

					if (name[0] == '[') {
						special = true;
						flags.setReadable(false); // They technically are, but only under a lot of conditions
					}
				}

				newLayout.emplace(this, begin, end - begin, flags,
					name.empty() ? std::nullopt : std::optional{ LinuxNamedData{ .name = name, .deleted = deleted, .special = special } });
			}

			fileStream.close();

			layout = std::move(newLayout);
		}

		[[nodiscard]] std::size_t getPageGranularity() const
		{
			// The page size could, in theory, be a different one for each process, but under Linux that shouldn't happen.
			static const std::size_t pageSize = getpagesize();
			return pageSize;
		}

		[[nodiscard]] std::uintptr_t allocate(std::uintptr_t address, std::size_t size, Flags protection) const
			requires Local
		{
			int flags = MAP_PRIVATE | MAP_ANONYMOUS;
			if (address > 0) {
				flags |= MAP_FIXED_NOREPLACE;
			}
			const void* res = mmap(reinterpret_cast<void*>(address), size, flagsToPosix(protection), flags, -1, 0);
			if (res == MAP_FAILED)
				throw std::runtime_error(strerror(errno));
			return reinterpret_cast<uintptr_t>(res);
		}
		void deallocate(std::uintptr_t address, std::size_t size) const
			requires Local
		{
			const auto res = munmap(reinterpret_cast<void*>(address), size);

			if (res == -1)
				throw std::runtime_error(strerror(errno));
		}
		void protect(std::uintptr_t address, std::size_t size, Flags protection) const
			requires Local
		{
			const auto res = mprotect(reinterpret_cast<void*>(address), size, flagsToPosix(protection));

			if (res == -1)
				throw std::runtime_error(strerror(errno));
		}

		void read(std::uintptr_t address, void* content, std::size_t length) const
			requires CanRead
		{
			if constexpr (Local && !Read) {
				std::memcpy(content, reinterpret_cast<void*>(address), length);
				return;
			}

#ifdef __GLIBC__
			const auto res = pread64(memInterface, content, length, static_cast<off64_t>(address));
#else
			const auto res = pread(memInterface, content, length, static_cast<off_t>(address));
#endif
			if (res == -1)
				throw std::runtime_error(strerror(errno));
		}

		void write(std::uintptr_t address, const void* content, std::size_t length) const
			requires CanWrite
		{
			if constexpr (Local && !Write) {
				std::memcpy(reinterpret_cast<void*>(address), content, length);
				return;
			}

#ifdef __GLIBC__
			const auto res = pwrite64(memInterface, content, length, static_cast<off64_t>(address));
#else
			const auto res = pwrite(memInterface, content, length, static_cast<off_t>(address));
#endif
			if (res == -1)
				throw std::runtime_error(strerror(errno));
		}

		static_assert(AddressAware<RegionT>);
		static_assert(LengthAware<RegionT>);
		static_assert(FlagAware<RegionT>);
		static_assert(NameAware<RegionT>);
		static_assert(PathAware<RegionT>);
		static_assert(!CanRead || Viewable<RegionT>);
	};

	static_assert(LayoutAware<LinuxMemoryManager<true, true, true>>);
	static_assert(LayoutAware<LinuxMemoryManager<true, false, true>>);
	static_assert(LayoutAware<LinuxMemoryManager<false, true, true>>);

	static_assert(LayoutAware<LinuxMemoryManager<true, true, false>>);
	static_assert(LayoutAware<LinuxMemoryManager<true, false, false>>);
	static_assert(LayoutAware<LinuxMemoryManager<false, true, false>>);

	static_assert(GranularityAware<LinuxMemoryManager<true, true, true>>);
	static_assert(GranularityAware<LinuxMemoryManager<true, false, true>>);
	static_assert(GranularityAware<LinuxMemoryManager<false, true, true>>);

	static_assert(GranularityAware<LinuxMemoryManager<true, true, false>>);
	static_assert(GranularityAware<LinuxMemoryManager<true, false, false>>);
	static_assert(GranularityAware<LinuxMemoryManager<false, true, false>>);

	static_assert(Allocator<LinuxMemoryManager<true, true, true>>);
	static_assert(Allocator<LinuxMemoryManager<true, false, true>>);
	static_assert(Allocator<LinuxMemoryManager<false, true, true>>);

	static_assert(Deallocator<LinuxMemoryManager<true, true, true>>);
	static_assert(Deallocator<LinuxMemoryManager<true, false, true>>);
	static_assert(Deallocator<LinuxMemoryManager<false, true, true>>);

	static_assert(Protector<LinuxMemoryManager<true, true, true>>);
	static_assert(Protector<LinuxMemoryManager<true, false, true>>);
	static_assert(Protector<LinuxMemoryManager<false, true, true>>);

	static_assert(Reader<LinuxMemoryManager<true, true, true>>);
	static_assert(Reader<LinuxMemoryManager<true, false, true>>);
	static_assert(Reader<LinuxMemoryManager<false, true, true>>);

	static_assert(Reader<LinuxMemoryManager<true, true, false>>);
	static_assert(Reader<LinuxMemoryManager<true, false, false>>);

	static_assert(Writer<LinuxMemoryManager<true, true, true>>);
	static_assert(Writer<LinuxMemoryManager<true, false, true>>);
	static_assert(Writer<LinuxMemoryManager<false, true, true>>);

	static_assert(Writer<LinuxMemoryManager<true, true, false>>);
	static_assert(Writer<LinuxMemoryManager<false, true, false>>);

}

#endif
