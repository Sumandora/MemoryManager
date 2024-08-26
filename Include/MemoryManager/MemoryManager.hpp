#ifndef MEMORYMANAGER_HPP
#define MEMORYMANAGER_HPP

#include <array>
#include <bitset>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cwctype>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

namespace MemoryManager {
	class Flags : public std::bitset<3> {
	public:
		constexpr explicit Flags(std::array<char, 3> permissions)
		{
			set(0, permissions.at(0) == 'r');
			set(1, permissions.at(1) == 'w');
			set(2, permissions.at(2) == 'x');
		}
		// NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
		constexpr Flags(const char rwx[3])
		{
			set(0, rwx[0] == 'r');
			set(1, rwx[1] == 'w');
			set(2, rwx[2] == 'x');
		}
		constexpr Flags(
			bool readable,
			bool writable,
			bool executable)
		{
			set(0, readable);
			set(1, writable);
			set(2, executable);
		}

		[[nodiscard]] constexpr bool isReadable() const { return test(0); }
		[[nodiscard]] constexpr bool isWriteable() const { return test(1); }
		[[nodiscard]] constexpr bool isExecutable() const { return test(2); }

		constexpr void setReadable(bool b) { set(0, b); }
		constexpr void setWriteable(bool b) { set(1, b); }
		constexpr void setExecutable(bool b) { set(2, b); }

		[[nodiscard]] constexpr std::string asString() const
		{
			std::string string;
			string.reserve(3);

			string += test(0) ? 'r' : '-';
			string += test(1) ? 'w' : '-';
			string += test(2) ? 'x' : '-';
			return string;
		}
	};

	template <typename Region>
	concept AddressAware = requires(const Region reg) {
		{ reg.getAddress() } -> std::same_as<std::uintptr_t>;
	};

	template <typename Region>
	concept LengthAware = requires(const Region reg) {
		{ reg.getLength() } -> std::same_as<std::size_t>;
	};

	template <typename Region>
	concept FlagAware = requires(const Region reg) {
		{ reg.getFlags() } -> std::same_as<Flags>;
	};

	template <typename Region>
	concept NameAware = requires(const Region reg) {
		{ reg.getName() } -> std::same_as<std::optional<std::string>>;
	};

	template <typename Region>
	concept PathAware = requires(const Region reg) {
		{ reg.getPath() } -> std::same_as<std::optional<std::string>>;
	};

	template <typename Region>
	concept Readable = requires(const Region reg) {
		{ reg.read() } -> std::same_as<std::byte*>;
	};

	template <typename Region>
		requires AddressAware<Region>
	struct RegionComparator {
		using is_transparent = void;

		constexpr bool operator()(const Region& lhs, const Region& rhs) const
		{
			return lhs.getAddress() < rhs.getAddress();
		}

		constexpr bool operator()(std::uintptr_t lhs, const Region& rhs) const
		{
			return lhs < rhs.getAddress();
		}
	};

	// FancyPointer for iterating over cached regions
	struct CachedByte {
		std::byte value;
		std::byte* pointer;

		// NOLINTNEXTLINE(google-runtime-operator)
		constexpr std::byte* operator&() const { return pointer; }
		// NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
		constexpr operator std::byte() const { return value; }
	};

	template <typename Base>
	class CachableMixin {
		mutable std::unique_ptr<std::byte[]> bytes = nullptr;

		constexpr std::byte* getBytes() const
		{
			if (!bytes)
				bytes = std::unique_ptr<std::byte[]>{ static_cast<const Base*>(this)->read() };
			return bytes.get();
		}

	public:
		class CacheIterator {
			const CachableMixin* parent;
			std::size_t index;

		public:
			constexpr CacheIterator()
				: parent(nullptr)
				, index(0)
			{
			}

			constexpr CacheIterator(const CachableMixin* parent, std::size_t index)
				: parent(parent)
				, index(index)
			{
			}

			using iterator_category = std::bidirectional_iterator_tag;
			using value_type = CachedByte;
			using reference = CachedByte&;
			using pointer = std::byte*;
			using difference_type = std::ptrdiff_t;

			constexpr CachedByte operator*() const
			{
				return (*parent)[index];
			}
			constexpr std::byte* operator->() const
			{
				return &(*parent)[index];
			}

			constexpr CacheIterator& operator++()
			{
				index++;
				return *this;
			}
			constexpr CacheIterator operator++(int)
			{
				CacheIterator it = *this;
				index++;
				return it;
			}

			constexpr CacheIterator& operator--()
			{
				index--;
				return *this;
			}
			constexpr CacheIterator operator--(int)
			{
				CacheIterator it = *this;
				index--;
				return it;
			}

			constexpr auto operator<=>(std::uintptr_t pointer) const
			{
				return static_cast<const Base*>(parent)->getAddress() + index <=> pointer;
			}

			constexpr auto operator==(std::uintptr_t pointer) const
			{
				return static_cast<const Base*>(parent)->getAddress() + index == pointer;
			}

			constexpr bool operator==(const CacheIterator& rhs) const
			{
				return index == rhs.index;
			}

			// NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
			constexpr operator std::uintptr_t()
			{
				return static_cast<const Base*>(parent)->getAddress() + index;
			}
		};

		[[nodiscard]] CachedByte operator[](std::size_t i) const
		{
			return { getBytes()[i], reinterpret_cast<std::byte*>(static_cast<const Base*>(this)->getAddress() + i) };
		}

		[[nodiscard]] constexpr CacheIterator cbegin() const
		{
			return { this, 0 };
		}
		[[nodiscard]] constexpr CacheIterator cend() const
		{
			return { this, static_cast<const Base*>(this)->getLength() };
		}

		[[nodiscard]] constexpr std::reverse_iterator<CacheIterator> crbegin() const
		{
			return std::make_reverse_iterator(cend());
		}
		[[nodiscard]] constexpr std::reverse_iterator<CacheIterator> crend() const
		{
			return std::make_reverse_iterator(cbegin());
		}
	};

	// clang-format off
	template<typename Region>
	concept Iterable =
		AddressAware<Region> &&
		LengthAware<Region> &&
		Readable<Region> &&
		std::derived_from<Region, CachableMixin<Region>>;

	template<typename Region>
	concept MemoryRegion =
		AddressAware<Region> ||
		LengthAware<Region> ||
		FlagAware<Region> ||
		NameAware<Region> ||
		PathAware<Region> ||
		Readable<Region> ||
		Iterable<Region>;

	// This one can be used to check if you have implemented the complete interface
	template<typename Region>
	concept CompleteMemoryRegion =
		AddressAware<Region> &&
		LengthAware<Region> &&
		FlagAware<Region> &&
		NameAware<Region> &&
		PathAware<Region> &&
		Readable<Region> &&
		Iterable<Region>;
	// clang-format on

	template <typename Region, typename Comparator = RegionComparator<Region>>
		requires MemoryRegion<Region>
	class MemoryLayout : public std::set<Region, Comparator> {
	public:
		[[nodiscard]] constexpr const Region* findRegion(std::uintptr_t address) const
		{
			if (this->empty())
				return nullptr;
			auto it = this->upper_bound(address);
			if (it == this->begin())
				return nullptr;
			it--;
			auto& region = *it;
			if (address >= region.getAddress() && address < region.getAddress() + region.getLength())
				return &region;
			return nullptr;
		}
	};

	// MemoryManager
	template <typename MemMgr>
	concept LayoutAware = AddressAware<typename MemMgr::RegionT> && requires(std::remove_const_t<MemMgr> manager) {
		{ std::as_const(manager).getLayout() } -> std::same_as<const MemoryLayout<typename MemMgr::RegionT>&>;

		// Warning: Calling this will invalidate all references to MemoryRegions
		{ manager.update() };
	};

	template <typename MemMgr>
	concept GranularityAware = requires(const MemMgr manager) {
		{ manager.getPageGranularity() } -> std::same_as<std::size_t>;
	};

	template <typename MemMgr>
	concept Allocator = requires(const MemMgr manager, std::uintptr_t address, std::size_t size, Flags protection) {
		/**
		 * Allocates a memory block
		 * @param address if not 0: indicates the location of the new memory, in that case address must be aligned to page granularity
		 * @param size may get rounded up to pagesize
		 * @returns pointer to the new memory
		 */
		{ manager.allocate(address, size, protection) } -> std::same_as<std::uintptr_t>;
	};

	template <typename MemMgr>
	concept Deallocator = requires(const MemMgr manager, std::uintptr_t address, std::size_t size) {
		/**
		 * Deallocates a memory block
		 * @param address must be aligned to page granularity
		 */
		{ manager.deallocate(address, size) };
	};

	template <typename MemMgr>
	concept Protector = requires(const MemMgr manager, std::uintptr_t address, std::size_t size, Flags protection) {
		/**
		 * Changes protection of the memory page
		 * @param address must be aligned to page granularity
		 */
		{ manager.protect(address, size, protection) };
	};

	template <typename MemMgr>
	concept Reader = requires(const MemMgr manager, std::uintptr_t address, void* content, std::size_t length) {
		/**
		 * Indicates if the memory manager requires permissions for reading from memory pages
		 */
		{ MemMgr::RequiresPermissionsForReading } -> std::convertible_to<bool>;

		{ manager.read(address, content, length) };
	};

	template <typename MemMgr>
	concept Writer = requires(const MemMgr manager, std::uintptr_t address, const void* content, std::size_t length) {
		/**
		 * Indicates if the memory manager requires permissions for writing to memory pages
		 */
		{ MemMgr::RequiresPermissionsForWriting } -> std::convertible_to<bool>;

		{ manager.write(address, content, length) };
	};

	// clang-format off
	template<typename MemMgr>
	concept MemoryManager =
		LayoutAware<MemMgr> ||
	    GranularityAware<MemMgr> ||
	    Allocator<MemMgr> ||
	    Deallocator<MemMgr> ||
	    Protector<MemMgr> ||
	    Reader<MemMgr> ||
	    Writer<MemMgr>;

	// This one can be used to check if you have implemented the complete interface
	template<typename MemMgr>
	concept CompleteMemoryManager =
		LayoutAware<MemMgr> &&
		GranularityAware<MemMgr> &&
		Allocator<MemMgr> &&
		Deallocator<MemMgr> &&
		Protector<MemMgr> &&
		Reader<MemMgr> &&
		Writer<MemMgr>;
	// clang-format on
}

#endif
