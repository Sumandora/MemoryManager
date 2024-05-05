#ifndef FORCEWRITE_HPP
#define FORCEWRITE_HPP

#include <array>
#include <bitset>
#include <compare>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>

namespace MemoryManager {
	class MemoryManager;

	class ProtectionFlags : public std::bitset<3> { // Flags#private may not be applicable for certain tasks like allocation or protection. Use this class instead.
	public:
		explicit ProtectionFlags(std::array<char, 3> permissions);
		ProtectionFlags(
			bool readable,
			bool writable,
			bool executable
		);

		[[nodiscard]] constexpr bool isReadable() const { return (*this)[0]; }
		[[nodiscard]] constexpr bool isWriteable() const { return (*this)[1]; }
		[[nodiscard]] constexpr bool isExecutable() const { return (*this)[2]; }
	};

	class Flags : public std::bitset<4> {
	public:
		explicit Flags(std::array<char, 4> permissions); // Parses a "rwxp" string from the /proc/$/maps interface
		Flags(
			bool readable,
			bool writable,
			bool executable,
			bool _private
		);

		[[nodiscard]] constexpr bool isReadable() const { return (*this)[0]; }
		[[nodiscard]] constexpr bool isWriteable() const { return (*this)[1]; }
		[[nodiscard]] constexpr bool isExecutable() const { return (*this)[2]; }
		[[nodiscard]] constexpr bool isPrivate() const { return (*this)[3]; }

		[[nodiscard]] std::string asString() const;
	};

	bool operator==(const class Flags& flags, const ProtectionFlags& protectionFlags);

	class CachedRegion {
		std::uintptr_t remoteAddress;
		std::size_t length;
		std::unique_ptr<std::byte[]> bytes;
		friend class MemoryRegion;

	public:
		CachedRegion(std::uintptr_t remoteAddress, std::size_t length);

		// This is not really good, but it's the best I can do here to support STL code that wants to take the address of the iterated type
		struct CachedByte {
			std::byte value;
			std::byte* pointer;

			std::byte* operator&() const { return pointer; }
			operator std::byte() const { return value; }
		};

		[[nodiscard]] CachedByte operator[](std::size_t i) const;

		[[nodiscard]] std::uintptr_t getRemoteAddress() const { return remoteAddress; }
		[[nodiscard]] std::size_t getLength() const { return length; }

		class CacheIterator {
			const CachedRegion* parent;
			std::size_t index;

		public:
			CacheIterator() : parent(nullptr), index(0) {}

			CacheIterator(const CachedRegion* parent, std::size_t index)
				: parent(parent)
				, index(index)
			{
			}
			using iterator_category = std::bidirectional_iterator_tag;
			using value_type = CachedByte;
			using reference = CachedByte&;
			using pointer = std::byte*;
			using difference_type = std::ptrdiff_t;

			[[nodiscard]] CachedByte operator*() const;
			[[nodiscard]] std::byte* operator->() const;

			CacheIterator& operator++();
			CacheIterator operator++(int);

			CacheIterator& operator--();
			CacheIterator operator--(int);

			[[nodiscard]] bool operator==(const CacheIterator& rhs) const;
		};

		[[nodiscard]] CacheIterator cbegin() const;
		[[nodiscard]] CacheIterator cend() const;

		[[nodiscard]] std::reverse_iterator<CacheIterator> crbegin() const;
		[[nodiscard]] std::reverse_iterator<CacheIterator> crend() const;
	};

	class MemoryRegion {
		const MemoryManager* parent;
		std::uintptr_t beginAddress;
		std::size_t length;
		Flags flags;
		std::optional<std::string> name;
		bool special; // On Linux those include mappings which have custom names e.g. initial heap, initial stack etc...

	public:
		MemoryRegion(const MemoryManager* parent, std::uintptr_t beginAddress, std::size_t length, Flags flags, std::optional<std::string> name, bool special)
			: parent(parent)
			, beginAddress(beginAddress)
			, length(length)
			, flags(flags)
			, name(std::move(name))
			, special(special)
		{
		}

		[[nodiscard]] std::uintptr_t getBeginAddress() const { return beginAddress; }
		[[nodiscard]] std::size_t getLength() const { return length; }
		[[nodiscard]] std::uintptr_t getEndAddress() const { return beginAddress + length; }
		[[nodiscard]] const Flags& getFlags() const { return flags; }
		[[nodiscard]] const std::optional<std::string>& getName() const { return name; }
		[[nodiscard]] bool isSpecial() const { return special; }

		[[nodiscard]] bool isInside(std::uintptr_t address) const
		{
			return address >= beginAddress && address < beginAddress + length;
		}

		// Calling this on sections that are not readable by whatever means will cause undefined behavior
		[[nodiscard]] CachedRegion cache() const;

		struct Compare {
			using is_transparent = void;

			bool operator()(const MemoryRegion& lhs, const MemoryRegion& rhs) const
			{
				return lhs.beginAddress < rhs.beginAddress;
			}

			bool operator()(std::uintptr_t lhs, const MemoryRegion& rhs) const
			{
				return lhs < rhs.beginAddress;
			}
		};
	};

	class MemoryLayout : public std::set<MemoryRegion, MemoryRegion::Compare> {
	public:
		[[nodiscard]] const MemoryRegion* findRegion(std::uintptr_t address) const;
#ifdef MEMORYMANAGER_DEFINE_PTR_WRAPPER
		[[nodiscard]] const MemoryRegion* findRegion(const void* address) const
		{
			return findRegion(reinterpret_cast<std::uintptr_t>(address));
		}
#endif
	};

	class MemoryManager {
	public:
		virtual ~MemoryManager() = default;

		[[nodiscard]] virtual const MemoryLayout* getLayout() const = 0;
		virtual void update() = 0; // Warning: Calling this will invalidate all references to MemoryRegions

		[[nodiscard]] virtual std::size_t getPageGranularity() const = 0;

		/**
		 * Allocates a memory block
		 * @param address if set to something that is not a nullptr then indicates the location of the new memory, in that case address must be aligned to page granularity
		 * @param size may get rounded up to pagesize
		 * @returns pointer to the new memory
		 */
		[[nodiscard]] virtual std::uintptr_t allocate(std::uintptr_t address, std::size_t size, ProtectionFlags protection) const = 0;
		/**
		 * @param address must be aligned to page granularity
		 */
		virtual void deallocate(std::uintptr_t address, std::size_t size) const = 0;
		/**
		 * Changes protection of the memory page
		 * @param address must be aligned to page granularity
		 */
		virtual void protect(std::uintptr_t address, std::size_t size, ProtectionFlags protection) const = 0;
#ifdef MEMORYMANAGER_DEFINE_PTR_WRAPPER
		[[nodiscard]] std::uintptr_t allocate(void* address, std::size_t size, ProtectionFlags protection) const
		{
			return allocate(reinterpret_cast<std::uintptr_t>(address), size, protection);
		}
		void deallocate(void* address, std::size_t size) const
		{
			deallocate(reinterpret_cast<std::uintptr_t>(address), size);
		}
		void protect(void* address, std::size_t size, ProtectionFlags protection) const
		{
			protect(reinterpret_cast<std::uintptr_t>(address), size, protection);
		}
#endif

		virtual void read(std::uintptr_t address, void* content, std::size_t length) const = 0;
		virtual void write(std::uintptr_t address, const void* content, std::size_t length) const = 0;

		template <typename T>
		void read(std::uintptr_t address, T* content) const
		{
			read(address, content, sizeof(T));
		}

		template <typename T>
		T read(std::uintptr_t address) const
		{
			T obj;
			read(address, &obj, sizeof(T));
			return obj;
		}

		template <typename T>
		void write(std::uintptr_t address, const T& obj) const
		{
			write(address, &obj, sizeof(obj));
		}
#ifdef MEMORYMANAGER_DEFINE_PTR_WRAPPER
		template <typename T>
		void read(const void* address, T* content) const
		{
			read<T>(reinterpret_cast<std::uintptr_t>(address), content);
		}

		template <typename T>
		T read(const void* address) const
		{
			return read<T>(reinterpret_cast<std::uintptr_t>(address));
		}

		template <typename T>
		void write(void* address, const T& obj) const
		{
			write<T>(reinterpret_cast<std::uintptr_t>(address), obj);
		}
#endif

		/**
		 * Indicates if the memory manager requires permissions for reading from memory pages
		 */
		[[nodiscard]] virtual bool requiresPermissionsForReading() const;
		/**
		 * Indicates if the memory manager requires permissions for writing from memory pages
		 */
		[[nodiscard]] virtual bool requiresPermissionsForWriting() const;

		/**
		 * Indicates whether the memory manager is fetching memory from a remote target.
		 * If returning true, this basically states that memory doesn't need to be copied into the local address space, but can be read by dereferencing pointers
		 */
		 [[nodiscard]] virtual bool isRemoteAddressSpace() const = 0;
	};

}

#endif
