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

	enum class LinuxSharedState : std::uint8_t {
		SHARED,
		MAY_SHARE,
		PRIVATE,
	};

	template <typename MemMgr, bool CanRead, bool Local>
	class LinuxRegion {
		const MemMgr* parent;
		std::uintptr_t address;
		std::size_t length;
		Flags flags;
		LinuxSharedState shared_state;
		std::optional<LinuxNamedData> named_data;
		mutable std::unique_ptr<std::byte[]> cached_memory;

	public:
		constexpr LinuxRegion(
			const MemMgr* parent,
			std::uintptr_t address,
			std::size_t length,
			Flags flags,
			LinuxSharedState shared_state,
			std::optional<LinuxNamedData> named_data)
			: parent(parent)
			, address(address)
			, length(length)
			, flags(flags)
			, shared_state(shared_state)
			, named_data(std::move(named_data))
		{
		}

		[[nodiscard]] std::uintptr_t get_address() const
		{
			return address;
		}

		[[nodiscard]] std::size_t get_length() const
		{
			return length;
		}

		[[nodiscard]] Flags get_flags() const
		{
			return flags;
		}

		[[nodiscard]] LinuxSharedState get_shared_state() const
		{
			return shared_state;
		}

		[[nodiscard]] bool is_shared() const
		{
			return shared_state != LinuxSharedState::PRIVATE;
		}

		[[nodiscard]] std::optional<std::string> get_path() const
		{
			return named_data.and_then([](const LinuxNamedData& named_data) -> std::optional<std::string> {
				if (named_data.special)
					return std::nullopt;
				if (!named_data.name.starts_with('/'))
					return std::nullopt;
				return named_data.name;
			});
		}

		[[nodiscard]] std::optional<std::string> get_name() const
		{
			return named_data.transform([](const LinuxNamedData& d) {
				const auto pos = d.name.rfind('/');
				return pos == std::string::npos || pos == d.name.size() ? d.name : d.name.substr(pos + 1);
			});
		}

		[[nodiscard]] bool is_deleted() const
		{
			return named_data.has_value() && named_data->deleted;
		}

		[[nodiscard]] bool is_special() const
		{
			return named_data.has_value() && named_data->special;
		}

		[[nodiscard]] bool does_update_view() const
			requires CanRead
		{
			if constexpr (Local)
				return flags.is_readable();
			return false;
		}

		[[nodiscard]] std::span<const std::byte> view(bool update_cache = false) const
			requires CanRead
		{
			if constexpr (Local)
				if (does_update_view() && !update_cache)
					return std::span{ reinterpret_cast<std::byte*>(get_address()), get_length() };

			if (!cached_memory || update_cache) {
				cached_memory.reset(new std::byte[get_length()]);
				parent->read(get_address(), cached_memory.get(), get_length());
			}

			return std::span{ cached_memory.get(), get_length() };
		}
	};

	template <bool Read, bool Write, bool Local = false>
	class LinuxMemoryManager {
		using Self = LinuxMemoryManager<Read, Write, Local>;

	public:
		static constexpr bool CAN_READ = Local || Read;
		static constexpr bool CAN_WRITE = Local || Write;
		static constexpr bool STORES_FILE_HANDLE = Read || Write;
		static constexpr bool REQUIRES_PERMISSIONS_FOR_READING = Local && !Read;
		static constexpr bool REQUIRES_PERMISSIONS_FOR_WRITING = Local && !Write;

		using RegionT = LinuxRegion<Self, CAN_READ, Local>;

	private:
		std::string pid;

		MemoryLayout<RegionT> layout;

		using FileHandle = std::invoke_result_t<decltype([](const char* a1, int a2) { return ::open(a1, a2); }), const char*, int>;

		[[no_unique_address]] std::conditional_t<STORES_FILE_HANDLE, FileHandle, std::monostate> mem_interface;

		static constexpr int INVALID_FILE_HANDLE = -1;

		static decltype(mem_interface) open_file_handle(const std::string& pid)
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

		static constexpr int flags_to_posix(Flags flags)
		{
			int prot = 0;

			if (flags.is_readable())
				prot |= PROT_READ;
			if (flags.is_writeable())
				prot |= PROT_WRITE;
			if (flags.is_executable())
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
			, mem_interface(open_file_handle(pid))
		{
		}

		~LinuxMemoryManager()
		{
			if constexpr (STORES_FILE_HANDLE)
				close();
		}

		// Since one of the member variables is a file handle, these two operations are not possible
		LinuxMemoryManager(const LinuxMemoryManager& other) = delete;
		LinuxMemoryManager& operator=(const LinuxMemoryManager& other) = delete;

		void close()
			requires STORES_FILE_HANDLE
		{
			if (mem_interface == INVALID_FILE_HANDLE)
				return;

			::close(mem_interface);
			mem_interface = INVALID_FILE_HANDLE;
		}

		void reopen()
			requires STORES_FILE_HANDLE
		{
			if (mem_interface != INVALID_FILE_HANDLE)
				return;

			mem_interface = open_file_handle(pid);
		}

		[[nodiscard]] bool is_closed() const
			requires STORES_FILE_HANDLE
		{
			return mem_interface == INVALID_FILE_HANDLE;
		}

		[[nodiscard]] const MemoryLayout<RegionT>& get_layout() const
		{
			return layout;
		}

		void update()
		{
			std::fstream file_stream{ "/proc/" + pid + "/maps", std::fstream::in };
			if (!file_stream) {
				throw std::exception{};
			}

			MemoryLayout<RegionT> new_layout{};
			for (std::string line; std::getline(file_stream, line);) {
				if (line.empty())
					continue; // ?

				std::uintptr_t begin = 0;
				std::uintptr_t end = 0;
				std::array<char, 3> perms{};
				char shared = 0;
				std::string name;

				int offset = -1;

				// NOLINTNEXTLINE(cert-err34-c)
				(void)sscanf(line.c_str(), "%zx-%zx %c%c%c%c %*x %*x:%*x %*x%n",
					&begin, &end, &perms[0], &perms[1], &perms[2], &shared, &offset);

				while (offset < line.length() && line[offset] == ' ')
					offset++;
				if (offset < line.length())
					name = line.c_str() + offset;

				Flags flags{ perms };

				bool deleted = false;
				bool special = false;
				if (!name.empty()) {
					constexpr static const char* DELETED_TAG = " (deleted)";
					constexpr static std::size_t DELETED_TAG_LEN = std::char_traits<char>::length(DELETED_TAG);
					if (name.ends_with(DELETED_TAG)) {
						deleted = true;
						name = name.substr(0, name.length() - DELETED_TAG_LEN);
					}

					if (name[0] == '[') {
						special = true;
						flags.set_readable(false); // They technically are, but only under a lot of conditions
					}
				}

				LinuxSharedState shared_state{};
				switch (shared) {
				case 'S':
					shared_state = LinuxSharedState::SHARED;
					break;
				case 's':
					shared_state = LinuxSharedState::MAY_SHARE;
					break;
				case 'p':
					shared_state = LinuxSharedState::PRIVATE;
					break;
				default:
					std::unreachable();
				}

				new_layout.emplace(this, begin, end - begin, flags, shared_state,
					name.empty() ? std::nullopt : std::make_optional(LinuxNamedData{ .name = name, .deleted = deleted, .special = special }));
			}

			file_stream.close();

			layout = std::move(new_layout);
		}

		[[nodiscard]] std::size_t get_page_granularity() const
		{
			// The page size could, in theory, be a different one for each process, but under Linux that shouldn't happen.
			static const std::size_t CACHED_PAGE_SIZE = getpagesize();
			return CACHED_PAGE_SIZE;
		}

		[[nodiscard]] std::uintptr_t allocate(std::size_t size, Flags protection) const
			requires Local
		{
			const void* res = mmap(nullptr, size, flags_to_posix(protection), MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			if (res == MAP_FAILED)
				throw std::runtime_error(strerror(errno));
			return reinterpret_cast<uintptr_t>(res);
		}
		[[nodiscard]] std::optional<std::uintptr_t> allocate_at(std::uintptr_t address, std::size_t size, Flags protection) const
			requires Local
		{
			const void* res = mmap(reinterpret_cast<void*>(address), size, flags_to_posix(protection), MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
			if (res == MAP_FAILED) {
				const int err = errno;

				if (err == EEXIST) {
					return std::nullopt;
				}

				throw std::runtime_error(strerror(err));
			}
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
			const auto res = mprotect(reinterpret_cast<void*>(address), size, flags_to_posix(protection));

			if (res == -1)
				throw std::runtime_error(strerror(errno));
		}

	private:
		void ensure_open() const
		{
			if constexpr (STORES_FILE_HANDLE)
				if (is_closed())
					throw std::logic_error{ std::to_string(mem_interface) };
		}

	public:
		void read(std::uintptr_t address, void* content, std::size_t length) const
			requires CAN_READ
		{
			if constexpr (Local && !Read) {
				std::memcpy(content, reinterpret_cast<void*>(address), length);
				return;
			}

			ensure_open();

#ifdef __GLIBC__
			const auto res = pread64(mem_interface, content, length, static_cast<off64_t>(address));
#else
			const auto res = pread(mem_interface, content, length, static_cast<off_t>(address));
#endif
			if (res == -1)
				throw std::runtime_error(strerror(errno));
		}

		void write(std::uintptr_t address, const void* content, std::size_t length) const
			requires CAN_WRITE
		{
			if constexpr (Local && !Write) {
				std::memcpy(reinterpret_cast<void*>(address), content, length);
				return;
			}

			ensure_open();

#ifdef __GLIBC__
			const auto res = pwrite64(mem_interface, content, length, static_cast<off64_t>(address));
#else
			const auto res = pwrite(mem_interface, content, length, static_cast<off_t>(address));
#endif
			if (res == -1)
				throw std::runtime_error(strerror(errno));
		}

		static_assert(AddressAware<RegionT>);
		static_assert(LengthAware<RegionT>);
		static_assert(FlagAware<RegionT>);
		static_assert(SharedAware<RegionT>);
		static_assert(NameAware<RegionT>);
		static_assert(PathAware<RegionT>);
		static_assert(!CAN_READ || Viewable<RegionT>);
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

	static_assert(PositionedAllocator<LinuxMemoryManager<true, true, true>>);
	static_assert(PositionedAllocator<LinuxMemoryManager<true, false, true>>);
	static_assert(PositionedAllocator<LinuxMemoryManager<false, true, true>>);

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
