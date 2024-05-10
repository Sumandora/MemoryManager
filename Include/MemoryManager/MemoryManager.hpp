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
		constexpr explicit ProtectionFlags(std::array<char, 3> permissions)
		{
			set(0, permissions.at(0) == 'r');
			set(1, permissions.at(1) == 'w');
			set(2, permissions.at(2) == 'x');
		}
		constexpr ProtectionFlags(
			bool readable,
			bool writable,
			bool executable)
		{
			set(0, readable);
			set(1, writable);
			set(2, executable);
		}

		[[nodiscard]] constexpr bool isReadable() const { return (*this)[0]; }
		[[nodiscard]] constexpr bool isWriteable() const { return (*this)[1]; }
		[[nodiscard]] constexpr bool isExecutable() const { return (*this)[2]; }
	};

	class Flags : public std::bitset<4> {
	public:
		constexpr explicit Flags(std::array<char, 4> permissions) // Parses a "rwxp" string from the /proc/$/maps interface
		{
			set(0, permissions.at(0) == 'r');
			set(1, permissions.at(1) == 'w');
			set(2, permissions.at(2) == 'x');
			set(3, permissions.at(3) == 'p');
		}
		constexpr Flags(
			bool readable,
			bool writable,
			bool executable,
			bool _private)
		{
			set(0, readable);
			set(1, writable);
			set(2, executable);
			set(3, _private);
		}

		[[nodiscard]] constexpr bool isReadable() const { return (*this)[0]; }
		[[nodiscard]] constexpr bool isWriteable() const { return (*this)[1]; }
		[[nodiscard]] constexpr bool isExecutable() const { return (*this)[2]; }
		[[nodiscard]] constexpr bool isPrivate() const { return (*this)[3]; }

		[[nodiscard]] constexpr std::string asString() const
		{
			std::string string;
			string += test(0) ? 'r' : '-';
			string += test(1) ? 'w' : '-';
			string += test(2) ? 'x' : '-';
			string += test(3) ? 'p' : '-';
			return string;
		}
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

			// Since unary-& is gone, we need to somehow allow this by other means
			[[nodiscard]] CachedByte* getPointer() { return this; }
			[[nodiscard]] const CachedByte* getPointer() const { return this; }

			std::byte* operator&() const { return pointer; }
			operator std::byte() const { return value; }
		};

		[[nodiscard]] CachedByte operator[](std::size_t i) const;

		[[nodiscard]] std::uintptr_t getRemoteAddress() const { return remoteAddress; }
		[[nodiscard]] std::size_t getLength() const { return length; }
		[[nodiscard]] std::byte* getBytes() const { return bytes.get(); }

		class CacheIterator {
			const CachedRegion* parent;
			std::size_t index;

		public:
			CacheIterator()
				: parent(nullptr)
				, index(0)
			{
			}

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
		MemoryManager* parent;
		std::uintptr_t beginAddress;
		std::size_t length;
		Flags flags;
		std::optional<std::string> name;
		bool special; // On Linux those include mappings which have custom names e.g. initial heap, initial stack etc...

	public:
		MemoryRegion(MemoryManager* parent, std::uintptr_t beginAddress, std::size_t length, Flags flags, std::optional<std::string> name, bool special)
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
		template<typename ParentSelf>
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
		/**
		 * Indicates if the memory manager requires permissions for reading from memory pages
		 */
		static constexpr bool RequiresPermissionsForReading = false;
		/**
		 * Indicates if the memory manager requires permissions for writing to memory pages
		 */
		static constexpr bool RequiresPermissionsForWriting = false;

		/**
		 * Indicates whether the memory manager is fetching memory from a remote target.
		 * If returning true, this basically states that memory doesn't need to be copied into the local address space, but can be read by dereferencing pointers
		 */
		static constexpr bool IsRemoteAddressSpace = false;

		template <typename Self>
		[[nodiscard]] const auto& getLayout(this Self&& self)
		{
			return self.getLayout_impl();
		}

		// Warning: Calling this will invalidate all references to MemoryRegions
		template <typename Self>
		void update(this Self&& self)
		{
			return self.update_impl();
		}

		template <typename Self>
		[[nodiscard]] std::size_t getPageGranularity(this Self&& self)
		{
			return self.getPageGranularity_impl();
		}

		/**
		 * Allocates a memory block
		 * @param address if set to something that is not a nullptr then indicates the location of the new memory, in that case address must be aligned to page granularity
		 * @param size may get rounded up to pagesize
		 * @returns pointer to the new memory
		 */
		template <typename Self>
		[[nodiscard]] std::uintptr_t allocate(this Self&& self, std::uintptr_t address, std::size_t size, ProtectionFlags protection)
		{
			return self.allocate_impl(address, size, protection);
		}
		/**
		 * @param address must be aligned to page granularity
		 */
		template <typename Self>
		void deallocate(this Self&& self, std::uintptr_t address, std::size_t size)
		{
			return self.deallocate_impl(address, size);
		}
		/**
		 * Changes protection of the memory page
		 * @param address must be aligned to page granularity
		 */
		template <typename Self>
		void protect(this Self&& self, std::uintptr_t address, std::size_t size, ProtectionFlags protection)
		{
			return self.protect_impl(address, size, protection);
		}

#ifdef MEMORYMANAGER_DEFINE_PTR_WRAPPER
		template <typename Self>
		[[nodiscard]] std::uintptr_t allocate(this Self&& self, void* address, std::size_t size, ProtectionFlags protection)
		{
			return self.allocate_impl(reinterpret_cast<std::uintptr_t>(address), size, protection);
		}
		template <typename Self>
		[[nodiscard]] std::uintptr_t deallocate(this Self&& self, void* address, std::size_t size)
		{
			self.deallocate_impl(reinterpret_cast<std::uintptr_t>(address), size);
		}
		template <typename Self>
		[[nodiscard]] std::uintptr_t protect(this Self&& self, void* address, std::size_t size, ProtectionFlags protection)
		{
			self.protect_impl(reinterpret_cast<std::uintptr_t>(address), size, protection);
		}
#endif

		template <typename Self>
		void read(this Self&& self, std::uintptr_t address, void* content, std::size_t length)
		{
			return self.read_impl(address, content, length);
		}
		template <typename Self>
		void write(this Self&& self, std::uintptr_t address, const void* content, std::size_t length)
		{
			return self.write_impl(address, content, length);
		}

		template <typename T, typename Self>
		void read(this Self&& self, std::uintptr_t address, T* content)
		{
			self.read(address, content, sizeof(T));
		}

		template <typename T, typename Self>
		T read(this Self&& self, std::uintptr_t address)
		{
			T obj;
			self.read(address, &obj, sizeof(T));
			return obj;
		}

		template <typename T, typename Self>
		void write(this Self&& self, std::uintptr_t address, const T& obj)
		{
			self.write(address, &obj, sizeof(obj));
		}
#ifdef MEMORYMANAGER_DEFINE_PTR_WRAPPER
		template <typename T, typename Self>
		void read(this Self&& self, const void* address, T* content)
		{
			self.template read<T>(reinterpret_cast<std::uintptr_t>(address), content);
		}

		template <typename T, typename Self>
		T read(this Self&& self, const void* address)
		{
			return self.template read<T>(reinterpret_cast<std::uintptr_t>(address));
		}

		template <typename T, typename Self>
		void write(this Self&& self, void* address, const T& obj)
		{
			self.template write<T>(reinterpret_cast<std::uintptr_t>(address), obj);
		}
#endif
	};

	template<typename ParentSelf>
	[[nodiscard]] CachedRegion MemoryRegion::cache() const
	{
		CachedRegion region{ beginAddress, length };
		parent->read<ParentSelf>(beginAddress, region.bytes.get(), length);
		return region;
	}

}

#endif
