#pragma once

#include <filesystem>
#include <memory>

#include <spdlog/spdlog.h>

namespace logging {
	std::shared_ptr<spdlog::logger> setup_logger(const std::filesystem::path& path);
}