#ifndef FORCEWRITE_HPP
#define FORCEWRITE_HPP

#include <array>
#include <bitset>
#include <cstdint>
#include <optional>
#include <set>

namespace MemoryManager {
	class MemoryRegion;
	class MemoryLayout;
	class MemoryManager;

	class Pointer {
		const MemoryManager* parent;
		std::uintptr_t address;

	public:
		Pointer(const MemoryManager* parent, std::uintptr_t address);

		void read(void* result, std::size_t length);
		void write(const void* content, std::size_t length);

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

		constexpr operator std::uintptr_t() const { return address; };
		inline std::strong_ordering operator<=>(std::uintptr_t other) const
		{
			return address <=> other;
		}
		inline std::strong_ordering operator<=>(const Pointer& other) const
		{
			return *this <=> other.address;
		}
	};

	class MemoryRegionFlags : public std::bitset<4> {
	public:
		explicit MemoryRegionFlags(std::array<char, 4> permissions); // Parses a rwxp string from the /proc/$/maps interface

		[[nodiscard]] constexpr bool isReadable() const { return (*this)[0]; }
		[[nodiscard]] constexpr bool isWriteable() const { return (*this)[1]; }
		[[nodiscard]] constexpr bool isExecutable() const { return (*this)[2]; }
		[[nodiscard]] constexpr bool isPrivate() const { return (*this)[3]; }
	};

	class MemoryRegion {
		const MemoryManager* parent;
		std::uintptr_t beginAddress;

	public:
		std::size_t length;
		MemoryRegionFlags flags;
		std::optional<std::string> name;

		MemoryRegion(const MemoryManager* parent, std::uintptr_t beginAddress, std::size_t length, MemoryRegionFlags flags, std::optional<std::string> name)
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

		inline std::strong_ordering operator<=>(std::uintptr_t other) const
		{
			return beginAddress <=> other;
		}

		inline std::strong_ordering operator<=>(const MemoryRegion& other) const
		{
			return *this <=> other.beginAddress;
		}

		inline bool operator==(std::uintptr_t other) const
		{
			return this->beginAddress == other;
		}

		inline bool operator==(const MemoryRegion& other) const
		{
			return this->beginAddress == other.beginAddress;
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

	private:
		friend Pointer;

		virtual void read(std::uintptr_t address, void* content, std::size_t length) const = 0;
		virtual void write(std::uintptr_t address, const void* content, std::size_t length) const = 0;
	};

}

#endif
