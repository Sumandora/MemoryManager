#ifndef MEMORYMANAGER_EXTERNALMEMORYMANAGER_HPP
#define MEMORYMANAGER_EXTERNALMEMORYMANAGER_HPP

#include "MemoryManager.hpp"

namespace MemoryManager {

	class ExternalMemoryManager : public MemoryManager {
	public:
		enum class Mode {
			NONE,
			READ,
			WRITE,
			READ_AND_WRITE
		};

	private:
		std::string pid;
		Mode mode; // Should reads use the proc mem interface?

		MemoryLayout layout;
		int memInterface;

	public:
		explicit ExternalMemoryManager(std::size_t pid, Mode mode = Mode::READ_AND_WRITE);
		explicit ExternalMemoryManager(std::string pid, Mode mode = Mode::READ_AND_WRITE);
		~ExternalMemoryManager() override;

		[[nodiscard]] virtual bool isRead() const;
		[[nodiscard]] virtual bool isWrite() const;

		[[nodiscard]] const MemoryLayout* getLayout() const override;
		void update() override;

	protected:
		void read(std::uintptr_t address, void *content, std::size_t length) const override;
		void write(std::uintptr_t address, const void *content, std::size_t length) const override;
	};
}

#endif
