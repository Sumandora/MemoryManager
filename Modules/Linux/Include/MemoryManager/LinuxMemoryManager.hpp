#ifndef MEMORYMANAGER_EXTERNALMEMORYMANAGER_HPP
#define MEMORYMANAGER_EXTERNALMEMORYMANAGER_HPP

#include "MemoryManager/MemoryManager.hpp"

#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <sys/mman.h>
#include <variant>

namespace MemoryManager {
	template <bool Read, bool Write, bool Local>
	class LinuxMemoryManager;

	template <typename MemMgr, bool CanRead>
	class LinuxRegion : public std::conditional_t<CanRead, CachableMixin<LinuxRegion<MemMgr, CanRead>>, std::monostate> {
		MemMgr* parent;
		std::uintptr_t address;
		std::size_t length;
		Flags flags;

	public:
		struct NamedData {
			std::string name; // may be a path

			bool deleted = false;
			bool special = false;
		};

	private:
		std::optional<NamedData> namedData;

	public:
		constexpr LinuxRegion(
			MemMgr* parent,
			std::uintptr_t address,
			std::size_t length,
			Flags flags,
			std::optional<NamedData> namedData)
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
			if(namedData->special)
				return std::nullopt;
			return namedData.and_then([](const NamedData& d) -> std::optional<std::string> {
				if(!d.name.starts_with('/'))
					return std::nullopt;
				return d.name;
			});
		}

		[[nodiscard]] std::optional<std::string> getName() const
		{
			return namedData.transform([](const NamedData& d) {
				auto pos = d.name.rfind('/');
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

		[[nodiscard]] std::byte* read() const
			requires CanRead
		{
			auto* data = new std::byte[getLength()];
			parent->read(getAddress(), data, getLength());
			return data;
		}
	};

	template <bool Read, bool Write, bool Local = false>
	class LinuxMemoryManager {
		using Self = LinuxMemoryManager<Read, Write, Local>;

	public:
		static constexpr bool CanRead = Local || Read;
		static constexpr bool CanWrite = Local || Write;
		static constexpr bool RequiresPermissionsForReading = Local && Write;
		static constexpr bool RequiresPermissionsForWriting = Local && Read;

		using RegionT = LinuxRegion<Self, CanRead>;

	private:
		std::string pid;

		MemoryLayout<RegionT> layout;

		using FileHandle = int;

		[[no_unique_address]] std::conditional_t<Read || Write, FileHandle, std::monostate> memInterface;

		static constexpr int INVALID_FILE_HANDLE = -1;

		static decltype(memInterface) openFileHandle(const std::string& pid)
		{
			if constexpr (!Read && !Write)
				return std::monostate{};
			else {
				int flag;
				if constexpr (Read && Write)
					flag = O_RDWR;
				else if constexpr (Read)
					flag = O_RDONLY;
				else if constexpr (Write)
					flag = O_WRONLY;

				FileHandle h = open(("/proc/" + pid + "/mem").c_str(), flag);
				if (h == INVALID_FILE_HANDLE)
					throw std::runtime_error(strerror(errno));
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

		// Since we have a file handle as one of the member variables these two operations are not possible
		LinuxMemoryManager(const LinuxMemoryManager& other) = delete;
		LinuxMemoryManager& operator=(const LinuxMemoryManager& other) = delete;

		void close()
			requires Read || Write
		{
			if(memInterface == INVALID_FILE_HANDLE)
				return;

			::close(memInterface);
			memInterface = INVALID_FILE_HANDLE;
		}

		void reopen()
			requires Read || Write
		{
			if(memInterface != INVALID_FILE_HANDLE)
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

				std::uintptr_t begin, end;
				std::array<char, 3> perms{};
				std::string name;
				name.resize(line.length());

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err34-c"
				sscanf(line.c_str(), "%zx-%zx %c%c%c%*c %*x %*x:%*x %*x %s[]",
					&begin, &end, &perms[0], &perms[1], &perms[2], name.data());
#pragma clang diagnostic pop

				name.resize(strlen(name.data()));

				Flags f{ perms };

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
						f.setReadable(false); // I mean they technically are, but only under a lot of conditions
					}
				}

				newLayout.emplace(this, begin, end - begin, f, name.empty() ? std::nullopt : std::optional{ typename RegionT::NamedData{ name, deleted, special } });
			}

			fileStream.close();

			layout = std::move(newLayout);
		}

		[[nodiscard]] std::size_t getPageGranularity() const
		{
			// The page size could, in theory, be a different one for each process, but under Linux that doesn't happen to my knowledge.
			static std::size_t pageSize = getpagesize();
			return pageSize;
		}

		[[nodiscard]] std::uintptr_t allocate(std::uintptr_t address, std::size_t size, Flags protection) const
			requires Local
		{
			int flags = MAP_PRIVATE | MAP_ANONYMOUS;
			if (address > 0)
				flags |= MAP_FIXED_NOREPLACE;
			void* res = mmap(reinterpret_cast<void*>(address), size, flagsToPosix(protection), flags, -1, 0);
			if (res == MAP_FAILED)
				throw std::runtime_error(strerror(errno));
			return reinterpret_cast<uintptr_t>(res);
		}
		void deallocate(std::uintptr_t address, std::size_t size) const
			requires Local
		{
			int res = munmap(reinterpret_cast<void*>(address), size);

			if (res == -1)
				throw std::runtime_error(strerror(errno));
		}
		void protect(std::uintptr_t address, std::size_t size, Flags protection) const
			requires Local
		{
			int res = mprotect(reinterpret_cast<void*>(address), size, flagsToPosix(protection));

			if (res == -1)
				throw std::runtime_error(strerror(errno));
		}

		void read(std::uintptr_t address, void* content, std::size_t length) const
			requires Read || Local
		{
			if constexpr (Local && !Read) {
				std::memcpy(content, reinterpret_cast<void*>(address), length);
				return;
			}

#ifdef __GLIBC__
			auto res = pread64(memInterface, content, length, static_cast<off64_t>(address));
#else
			auto res = pread(memInterface, content, length, static_cast<off_t>(address));
#endif
			if (res == -1)
				throw std::runtime_error(strerror(errno));
		}

		void write(std::uintptr_t address, const void* content, std::size_t length) const
			requires Write || Local
		{
			if constexpr (Local && !Write) {
				std::memcpy(reinterpret_cast<void*>(address), content, length);
				return;
			}

#ifdef __GLIBC__
			auto res = pwrite64(memInterface, content, length, static_cast<off64_t>(address));
#else
			auto res = pwrite(memInterface, content, length, static_cast<off_t>(address));
#endif
			if (res == -1)
				throw std::runtime_error(strerror(errno));
		}
	};

	static_assert(CompleteMemoryRegion<LinuxMemoryManager<true, true, true>::RegionT>);
	static_assert(CompleteMemoryRegion<LinuxMemoryManager<true, true, false>::RegionT>);

	static_assert(CompleteMemoryRegion<LinuxMemoryManager<true, false, true>::RegionT>);
	static_assert(CompleteMemoryRegion<LinuxMemoryManager<true, false, false>::RegionT>);

	static_assert(CompleteMemoryRegion<LinuxMemoryManager<false, false, true>::RegionT>);
	static_assert(!CompleteMemoryRegion<LinuxMemoryManager<false, false, false>::RegionT>);

	static_assert(CompleteMemoryRegion<LinuxMemoryManager<true, false, true>::RegionT>);
	static_assert(CompleteMemoryRegion<LinuxMemoryManager<true, false, false>::RegionT>);

	static_assert(CompleteMemoryManager<LinuxMemoryManager<true, true, true>>);
	static_assert(CompleteMemoryManager<LinuxMemoryManager<true, false, true>>);
	static_assert(CompleteMemoryManager<LinuxMemoryManager<false, false, true>>);

	// It's possible to satisfy those in the future, but that requires ptracing
	static_assert(!CompleteMemoryManager<LinuxMemoryManager<true, true, false>>);
	static_assert(!CompleteMemoryManager<LinuxMemoryManager<true, false, false>>);
	static_assert(!CompleteMemoryManager<LinuxMemoryManager<false, false, false>>);

	// For now those will be enough:
	static_assert(Reader<LinuxMemoryManager<true, true, false>>);
	static_assert(Writer<LinuxMemoryManager<true, true, false>>);

	static_assert(Reader<LinuxMemoryManager<true, false, false>>);
	static_assert(!Writer<LinuxMemoryManager<true, false, false>>);

	static_assert(!Reader<LinuxMemoryManager<false, false, false>>);
	static_assert(!Writer<LinuxMemoryManager<false, false, false>>);
}

#endif
