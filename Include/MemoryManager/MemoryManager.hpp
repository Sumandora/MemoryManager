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
		constexpr ProtectionFlags(const char rwx[3])
		{
			set(0, rwx[0] == 'r');
			set(1, rwx[1] == 'w');
			set(2, rwx[2] == 'x');
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
		constexpr Flags(const char rwx[4])
		{
			set(0, rwx[0] == 'r');
			set(1, rwx[1] == 'w');
			set(2, rwx[2] == 'x');
			set(3, rwx[3] == 'p');
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

	constexpr bool operator==(const Flags& flags, const ProtectionFlags& protectionFlags)
	{
		return
			flags.isReadable() == protectionFlags.isReadable() &&
			flags.isWriteable() == protectionFlags.isWriteable() &&
			flags.isExecutable() == protectionFlags.isExecutable();
	}

	class CachedRegion {
		std::uintptr_t remoteAddress;
		std::size_t length;
		std::unique_ptr<std::byte[]> bytes;
		friend class MemoryRegion;

	public:
		constexpr CachedRegion(std::uintptr_t remoteAddress, std::size_t length)
			: remoteAddress(remoteAddress)
			, length(length)
			, bytes(std::unique_ptr<std::byte[]>{ new std::byte[length] })
		{
		}

		// This is not really good, but it's the best I can do here to support STL code that wants to take the address of the iterated type
		struct CachedByte {
			std::byte value;
			std::byte* pointer;

			// Since unary-& is gone, we need to somehow allow this by other means
			template<typename Self>
			[[nodiscard]] constexpr auto getThis(this Self&& self) { return self; }

			constexpr std::byte* operator&() const { return pointer; }
			constexpr operator std::byte() const { return value; }
		};

		[[nodiscard]] CachedByte operator[](std::size_t i) const {
			return { bytes[i], reinterpret_cast<std::byte*>(remoteAddress + i) };
		}

		[[nodiscard]] constexpr std::uintptr_t getRemoteAddress() const { return remoteAddress; }
		[[nodiscard]] constexpr std::size_t getLength() const { return length; }
		[[nodiscard]] constexpr std::byte* getBytes() const { return bytes.get(); }

		class CacheIterator {
			const CachedRegion* parent;
			std::size_t index;

		public:
			constexpr CacheIterator()
				: parent(nullptr)
				, index(0)
			{
			}

			constexpr CacheIterator(const CachedRegion* parent, std::size_t index)
				: parent(parent)
				, index(index)
			{
			}
			using iterator_category = std::bidirectional_iterator_tag;
			using value_type = CachedByte;
			using reference = CachedByte&;
			using pointer = std::byte*;
			using difference_type = std::ptrdiff_t;

			constexpr CachedByte operator*() const {
				return (*parent)[index];
			}
			constexpr std::byte* operator->() const {
				return &(*parent)[index];
			}

			constexpr CacheIterator& operator++() {
				index++;
				return *this;
			}
			constexpr CacheIterator operator++(int) {
				CacheIterator it = *this;
				index++;
				return it;
			}

			constexpr CacheIterator& operator--() {
				index--;
				return *this;
			}
			constexpr CacheIterator operator--(int) {
				CacheIterator it = *this;
				index--;
				return it;
			}

			constexpr auto operator<=>(std::uintptr_t pointer) const {
				return parent->remoteAddress + index <=> pointer;
			}
			constexpr auto operator<=>(void* pointer) const {
				return this->operator<=>(reinterpret_cast<std::uintptr_t>(pointer));
			}

			constexpr auto operator==(std::uintptr_t pointer) const {
				return parent->remoteAddress + index == pointer;
			}
			constexpr auto operator==(void* pointer) const {
				return this->operator==(reinterpret_cast<std::uintptr_t>(pointer));
			}

			constexpr bool operator==(const CacheIterator& rhs) const {
				return index == rhs.index;
			}
		};

		[[nodiscard]] constexpr CacheIterator cbegin() const {
			return { this, 0 };
		}
		[[nodiscard]] constexpr CacheIterator cend() const {
			return { this, getLength() };
		}

		[[nodiscard]] constexpr std::reverse_iterator<CacheIterator> crbegin() const {
			return std::make_reverse_iterator(cend());
		}
		[[nodiscard]] constexpr std::reverse_iterator<CacheIterator> crend() const {
			return std::make_reverse_iterator(cbegin());
		}
	};

	class MemoryRegion {
		MemoryManager* parent;
		std::uintptr_t beginAddress;
		std::size_t length;
		Flags flags;
		std::optional<std::string> name;
		// Special regions are generally not supposed to be read
		// on Linux those include initial heap, initial stack, and more
		bool special;
		mutable std::unique_ptr<CachedRegion> cacheRegion;

	public:
		constexpr MemoryRegion(MemoryManager* parent, std::uintptr_t beginAddress, std::size_t length, Flags flags, std::optional<std::string> name, bool special)
			: parent(parent)
			, beginAddress(beginAddress)
			, length(length)
			, flags(flags)
			, name(std::move(name))
			, special(special)
			, cacheRegion(nullptr)
		{
		}

		[[nodiscard]] constexpr std::uintptr_t getBeginAddress() const { return beginAddress; }
		[[nodiscard]] constexpr std::size_t getLength() const { return length; }
		[[nodiscard]] constexpr std::uintptr_t getEndAddress() const { return beginAddress + length; }
		[[nodiscard]] constexpr const Flags& getFlags() const { return flags; }
		[[nodiscard]] constexpr const std::optional<std::string>& getName() const { return name; }
		[[nodiscard]] constexpr bool isSpecial() const { return special; }

		[[nodiscard]] constexpr bool isInside(std::uintptr_t address) const
		{
			return address >= beginAddress && address < beginAddress + length;
		}

		// Calling this on sections that are not readable by whatever means will cause undefined behavior
		template<typename ParentSelf>
		[[nodiscard]] constexpr const std::unique_ptr<CachedRegion>& cache() const;

		struct Compare {
			using is_transparent = void;

			constexpr bool operator()(const MemoryRegion& lhs, const MemoryRegion& rhs) const
			{
				return lhs.beginAddress < rhs.beginAddress;
			}

			constexpr bool operator()(std::uintptr_t lhs, const MemoryRegion& rhs) const
			{
				return lhs < rhs.beginAddress;
			}
		};
	};

	class MemoryLayout : public std::set<MemoryRegion, MemoryRegion::Compare> {
	public:
		[[nodiscard]] constexpr const MemoryRegion* findRegion(std::uintptr_t address) const
		{
			if(empty())
				return nullptr;
			auto it = upper_bound(address);
			if(it == begin())
				return nullptr;
			it--;
			auto& region = *it;
			if(region.isInside(address))
				return &region;
			return nullptr;
		}
#ifdef MEMORYMANAGER_DEFINE_PTR_WRAPPER
		[[nodiscard]] constexpr const MemoryRegion* findRegion(const void* address) const
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
		static constexpr bool RequiresPermissionsForReading = {};
		/**
		 * Indicates if the memory manager requires permissions for writing to memory pages
		 */
		static constexpr bool RequiresPermissionsForWriting = {};

		/**
		 * Indicates whether the memory manager is fetching memory from a remote target.
		 * If returning true, this basically states that memory doesn't need to be copied into the local address space, but can be read by dereferencing pointers
		 */
		static constexpr bool IsRemoteAddressSpace = {};

		template <typename Self>
		[[nodiscard]] constexpr const auto& getLayout(this Self&& self)
		{
			return self.getLayout_impl();
		}

		// Warning: Calling this will invalidate all references to MemoryRegions
		template <typename Self>
		constexpr void update(this Self&& self)
		{
			return self.update_impl();
		}

		template <typename Self>
		[[nodiscard]] constexpr std::size_t getPageGranularity(this Self&& self)
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
		[[nodiscard]] constexpr std::uintptr_t allocate(this Self&& self, std::uintptr_t address, std::size_t size, ProtectionFlags protection)
		{
			return self.allocate_impl(address, size, protection);
		}
		/**
		 * @param address must be aligned to page granularity
		 */
		template <typename Self>
		constexpr void deallocate(this Self&& self, std::uintptr_t address, std::size_t size)
		{
			return self.deallocate_impl(address, size);
		}
		/**
		 * Changes protection of the memory page
		 * @param address must be aligned to page granularity
		 */
		template <typename Self>
		constexpr void protect(this Self&& self, std::uintptr_t address, std::size_t size, ProtectionFlags protection)
		{
			return self.protect_impl(address, size, protection);
		}

#ifdef MEMORYMANAGER_DEFINE_PTR_WRAPPER
		template <typename Self>
		[[nodiscard]] constexpr std::uintptr_t allocate(this Self&& self, void* address, std::size_t size, ProtectionFlags protection)
		{
			return self.allocate_impl(reinterpret_cast<std::uintptr_t>(address), size, protection);
		}
		template <typename Self>
		constexpr void deallocate(this Self&& self, void* address, std::size_t size)
		{
			self.deallocate_impl(reinterpret_cast<std::uintptr_t>(address), size);
		}
		template <typename Self>
		constexpr void protect(this Self&& self, void* address, std::size_t size, ProtectionFlags protection)
		{
			self.protect_impl(reinterpret_cast<std::uintptr_t>(address), size, protection);
		}
#endif

		template <typename Self>
		constexpr void read(this Self&& self, std::uintptr_t address, void* content, std::size_t length)
		{
			return self.read_impl(address, content, length);
		}
		template <typename Self>
		constexpr void write(this Self&& self, std::uintptr_t address, const void* content, std::size_t length)
		{
			return self.write_impl(address, content, length);
		}

		template <typename T, typename Self>
		constexpr void read(this Self&& self, std::uintptr_t address, T* content)
		{
			self.read(address, content, sizeof(T));
		}

		template <typename T, typename Self>
		[[nodiscard]] constexpr T read(this Self&& self, std::uintptr_t address)
		{
			T obj;
			self.read(address, &obj, sizeof(T));
			return obj;
		}

		template <typename T, typename Self>
		constexpr void write(this Self&& self, std::uintptr_t address, const T& obj)
		{
			self.write(address, &obj, sizeof(obj));
		}
#ifdef MEMORYMANAGER_DEFINE_PTR_WRAPPER
		template <typename T, typename Self>
		constexpr void read(this Self&& self, const void* address, T* content)
		{
			self.template read<T>(reinterpret_cast<std::uintptr_t>(address), content);
		}

		template <typename T, typename Self>
		[[nodiscard]] constexpr T read(this Self&& self, const void* address)
		{
			return self.template read<T>(reinterpret_cast<std::uintptr_t>(address));
		}

		template <typename T, typename Self>
		constexpr void write(this Self&& self, void* address, const T& obj)
		{
			self.template write<T>(reinterpret_cast<std::uintptr_t>(address), obj);
		}
#endif
	};

	template<typename ParentSelf>
	[[nodiscard]] constexpr const std::unique_ptr<CachedRegion>& MemoryRegion::cache() const
	{
		if(cacheRegion)
			return cacheRegion;
		cacheRegion = std::make_unique<CachedRegion>(beginAddress, length);
		parent->read<ParentSelf>(beginAddress, cacheRegion->bytes.get(), length);
		return cacheRegion;
	}

}

#endif
