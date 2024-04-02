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

	class ProtectionFlags : public std::bitset<3> { // Flags#private may not be applicable for certain tasks like allocation or protection. Use this class instead.
	public:
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

		[[nodiscard]] constexpr bool isReadable() const { return (*this)[0]; }
		[[nodiscard]] constexpr bool isWriteable() const { return (*this)[1]; }
		[[nodiscard]] constexpr bool isExecutable() const { return (*this)[2]; }
		[[nodiscard]] constexpr bool isPrivate() const { return (*this)[3]; }

		[[nodiscard]] std::string asString() const;
	};

	class MemoryRegion {
		const MemoryManager* parent;
		std::uintptr_t beginAddress;
		std::size_t length;
		Flags flags;
		std::optional<std::string> name;

	public:
		MemoryRegion(const MemoryManager* parent, std::uintptr_t beginAddress, std::size_t length, Flags flags, std::optional<std::string> name)
			: parent(parent)
			, beginAddress(beginAddress)
			, length(length)
			, flags(flags)
			, name(std::move(name))
		{
		}

		[[nodiscard]] std::uintptr_t getBeginAddress() const { return beginAddress; }
		[[nodiscard]] std::size_t getLength() const { return length; }
		[[nodiscard]] std::uintptr_t getEndAddress() const { return beginAddress + length; }
		[[nodiscard]] const Flags& getFlags() const { return flags; }
		[[nodiscard]] const std::optional<std::string>& getName() const { return name; }

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
		void protect(void* address, std::size_t size, int protection) const
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

		// Indicates if the memory manager requires permissions for reading from memory pages
		[[nodiscard]] virtual bool requiresPermissionsForReading() const;
		// Indicates if the memory manager requires permissions for writing from memory pages
		[[nodiscard]] virtual bool requiresPermissionsForWriting() const;
	};

}

#endif
