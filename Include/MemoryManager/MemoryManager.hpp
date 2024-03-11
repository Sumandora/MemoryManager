#ifndef FORCEWRITE_HPP
#define FORCEWRITE_HPP

#include <array>
#include <bitset>
#include <compare>
#include <cstdint>
#include <optional>
#include <set>

namespace MemoryManager {
	class MemoryManager;

	class Pointer {
		const MemoryManager* parent;
		std::uintptr_t address;

	public:
		Pointer(const MemoryManager* parent, std::uintptr_t address);

		void read(void* result, std::size_t length) const;
		void write(const void* content, std::size_t length) const;

		template <typename T, bool UseHeap = (sizeof(T) > sizeof(T*))> // If the structure you are copying is big then returning it would yield a second copy.
		std::conditional_t<UseHeap, T*, T> read()
		{
			if constexpr (UseHeap) {
				T* obj = new T;
				read(obj, sizeof(T));
				return obj;
			} else {
				T obj;
				read(&obj, sizeof(T));
				return obj;
			}
		}

		template <typename T>
		void write(const T& obj)
		{
			write(&obj, sizeof(obj));
		}

		constexpr operator std::uintptr_t () const {
			return address;
		}
	};

	class Flags : public std::bitset<4> {
	public:
		explicit Flags(std::array<char, 4> permissions); // Parses a "rwxp" string from the /proc/$/maps interface

		[[nodiscard]] constexpr bool isReadable() const { return (*this)[0]; }
		[[nodiscard]] constexpr bool isWriteable() const { return (*this)[1]; }
		[[nodiscard]] constexpr bool isExecutable() const { return (*this)[2]; }
		[[nodiscard]] constexpr bool isPrivate() const { return (*this)[3]; }

		[[nodiscard]] std::string asString() const;
	};

	class MemoryRegion {
		const MemoryManager* parent;
		std::uintptr_t beginAddress;

	public:
		std::size_t length;
		Flags flags;
		std::optional<std::string> name;

		MemoryRegion(const MemoryManager* parent, std::uintptr_t beginAddress, std::size_t length, Flags flags, std::optional<std::string> name)
			: parent(parent)
			, beginAddress(beginAddress)
			, length(length)
			, flags(flags)
			, name(std::move(name))
		{
		}

		[[nodiscard]] Pointer begin() const { return { parent, beginAddress }; }
		[[nodiscard]] Pointer end() const { return { parent, beginAddress + length }; }

		[[nodiscard]] bool isInside(std::uintptr_t address) const
		{
			return address >= beginAddress && address < beginAddress + length;
		}

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

		[[nodiscard]] Pointer getPointer(std::uintptr_t address) const { return { this, address }; }
#ifdef MEMORYMANAGER_DEFINE_PTR_WRAPPER
		[[nodiscard]] Pointer getPointer(const void* address) const
		{
			return getPointer(reinterpret_cast<std::uintptr_t>(address));
		}
#endif

		[[nodiscard]] virtual const MemoryLayout* getLayout() const = 0;
		virtual void update() = 0; // Warning: Calling this will invalidate all references to MemoryRegions

		/**
		 * Allocates a memory block TODO: std::allocator?
		 * @param address if set to something that is not a nullptr then indicates the location of the new memory
		 * @param size may get rounded up to pagesize
		 * @param protection indicated in PROT_ flags from posix
		 * @return on fail returns MAP_FAILED (posix) otherwise returns pointer to the new memory
		 */
		[[nodiscard]] virtual std::uintptr_t allocate(std::uintptr_t address, std::size_t size, int protection) const = 0;
#ifdef MEMORYMANAGER_DEFINE_PTR_WRAPPER
		[[nodiscard]] std::uintptr_t allocate(const void* address, std::size_t size, int protection) const
		{
			return allocate(reinterpret_cast<std::uintptr_t>(address), size, protection);
		}
#endif
		virtual void deallocate(std::uintptr_t address, std::size_t size) const = 0;

	protected:
		friend Pointer;

		virtual void read(std::uintptr_t address, void* content, std::size_t length) const = 0;
		virtual void write(std::uintptr_t address, const void* content, std::size_t length) const = 0;
	};

}

#endif
