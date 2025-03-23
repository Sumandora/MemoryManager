#ifndef MEMORYMANAGER_HPP
#define MEMORYMANAGER_HPP

#include <array>
#include <bitset>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <span>
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

		constexpr void setReadable(bool b) & { set(0, b); }
		constexpr void setWriteable(bool b) & { set(1, b); }
		constexpr void setExecutable(bool b) & { set(2, b); }

		[[nodiscard]] constexpr std::string toString() const
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
	concept SharedAware = requires(const Region reg) {
		{ reg.isShared() } -> std::convertible_to<bool>;
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
	concept Viewable = requires(const Region reg, bool refresh) {
		// Indicates if the view represents memory, that updates as it's changed.
		{ reg.doesUpdateView() } -> std::convertible_to<bool>;

		// Taking the pointer of an element in the range is not required to yield the pointer to the element in the targeted address space.
		// To get the pointer to an element in the target memory space, take the distance from the beginning and add it to the regions base address.
		// Like this: `region.getAddress() + std::distance(view.begin(), it)`

		// When dealing with updating views, but a non-updating view is preferred then passing true, will yield a constant copy regardlessly.
		// If refresh = true is passed, please note that all previously requested constant views will be invalidated and should no longer be kept or read.
		{ reg.view() } -> std::same_as<std::span<const std::byte>>; // equivalent to refresh = false
		{ reg.view(refresh) } -> std::same_as<std::span<const std::byte>>;
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

	// clang-format off
	template<typename Region>
	concept MemoryRegion =
		AddressAware<Region> ||
		LengthAware<Region> ||
		FlagAware<Region> ||
		SharedAware<Region> ||
		NameAware<Region> ||
		PathAware<Region> ||
		Viewable<Region>;
	// clang-format on

	template <typename Region, typename Comparator = RegionComparator<Region>>
	class MemoryLayout : public std::set<Region, Comparator> {
	public:
		[[nodiscard]] constexpr const Region* findRegion(std::uintptr_t address) const
			requires AddressAware<Region> && LengthAware<Region>
		{
			if (this->empty())
				return nullptr;
			auto it = this->upper_bound(address);
			if (it == this->begin())
				return nullptr;
			it--;
			const auto& region = *it;
			if (address >= region.getAddress() && address < region.getAddress() + region.getLength())
				return &region;
			return nullptr;
		}
	};

	// MemoryManager
	template <typename MemMgr>
	concept LayoutAware = requires(std::remove_const_t<MemMgr> manager) {
		{ std::as_const(manager).getLayout() } -> std::same_as<const MemoryLayout<typename MemMgr::RegionT>&>;

		// Warning: Calling this will invalidate all references to MemoryRegions
		{ manager.update() };
	};

	template <typename MemMgr>
	concept GranularityAware = requires(const MemMgr manager) {
		{ manager.getPageGranularity() } -> std::same_as<std::size_t>;
	};

	template <typename MemMgr>
	concept PositionedAllocator = requires(const MemMgr manager, std::uintptr_t address, std::size_t size, Flags protection) {
		/**
		 * Allocates a memory region at an address
		 * @param address indicates the location of the new memory; the address must be aligned to page granularity
		 * @param size may get rounded up to pagesize
		 * @returns pointer to the new memory or nothing if the address already has an mapping associated with it
		 */
		{ manager.allocateAt(address, size, protection) } -> std::same_as<std::optional<std::uintptr_t>>;
	};

	template <typename MemMgr>
	concept Allocator = requires(const MemMgr manager, std::size_t size, Flags protection) {
		/**
		 * Allocates a memory region
		 * @param size may get rounded up to pagesize
		 * @returns pointer to the new memory
		 */
		{ manager.allocate(size, protection) } -> std::same_as<std::uintptr_t>;
	};

	template <typename MemMgr>
	concept Deallocator = requires(const MemMgr manager, std::uintptr_t address, std::size_t size) {
		/**
		 * Deallocates a memory region
		 * @param address must be aligned to page granularity
		 */
		{ manager.deallocate(address, size) };
	};

	template <typename MemMgr>
	concept Protector = requires(const MemMgr manager, std::uintptr_t address, std::size_t size, Flags protection) {
		/**
		 * Changes protection of a memory region
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
	    PositionedAllocator<MemMgr> ||
	    Allocator<MemMgr> ||
	    Deallocator<MemMgr> ||
	    Protector<MemMgr> ||
	    Reader<MemMgr> ||
	    Writer<MemMgr>;
	// clang-format on
}

#endif
