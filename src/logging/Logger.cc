#include "panlid/logging/Logger.hh"
#include "panlid/logging/LogEntry.hh"
#include "panlid/logging/LogLevel.hh"
#include "panlid/logging/LoggingTarget.hh"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <iterator>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace panlid::logging {

namespace {

#ifdef NDEBUG
constexpr bool kIsDebugBuild = false;
#else
constexpr bool kIsDebugBuild = true;
#endif

struct State {
    std::mutex Mutex;
    std::map<std::string, std::unique_ptr<Logger>> Loggers;
    std::vector<std::string> Filters;
    std::vector<std::pair<int, Logger::EntryListener>> Listeners;
    int NextListenerId = 0;
    std::optional<std::filesystem::path> Storage;
    std::string GameIdentifier = "game";
    std::string VersionIdentifier = "unknown";
    std::atomic<bool> Enabled{true};
    std::atomic<LogLevel> Level{kIsDebugBuild ? LogLevel::Debug : LogLevel::Verbose};
    std::int64_t SessionStartupTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                               std::chrono::system_clock::now().time_since_epoch())
                                               .count();
};

State& GetState() {
    static State state;
    return state;
}

std::string ToLower(std::string_view text) {
    std::string result{text};
    std::ranges::transform(result, result.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

std::string_view Trim(std::string_view text) {
    constexpr std::string_view kWhitespace = " \t\r\n\f\v";

    const auto first = text.find_first_not_of(kWhitespace);
    if (first == std::string_view::npos) {
        return {};
    }

    return text.substr(first, text.find_last_not_of(kWhitespace) - first + 1);
}

std::vector<std::string> SplitLines(std::string_view text) {
    std::vector<std::string> lines;
    std::size_t start = 0;

    while (true) {
        const auto end = text.find('\n', start);
        const auto count = end == std::string_view::npos ? std::string_view::npos : end - start;
        lines.emplace_back(Trim(text.substr(start, count)));

        if (end == std::string_view::npos) {
            return lines;
        }

        start = end + 1;
    }
}

std::string Timestamp() {
    return std::format("{:%Y-%m-%d %H:%M:%S}",
                       std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
}

std::optional<std::filesystem::path> GetStorage() {
    auto& state = GetState();
    std::scoped_lock lock{state.Mutex};
    return state.Storage;
}

std::string DescribeSession() {
    auto& state = GetState();
    std::scoped_lock lock{state.Mutex};
    return std::format("{} {}", state.GameIdentifier, state.VersionIdentifier);
}

std::vector<Logger::EntryListener> CopyListeners() {
    auto& state = GetState();
    std::scoped_lock lock{state.Mutex};

    std::vector<Logger::EntryListener> listeners;
    listeners.reserve(state.Listeners.size());
    for (const auto& [id, listener] : state.Listeners) {
        listeners.emplace_back(listener);
    }

    return listeners;
}

std::vector<Logger*> GetAllLoggers() {
    auto& state = GetState();
    std::scoped_lock lock{state.Mutex};

    std::vector<Logger*> loggers;
    loggers.reserve(state.Loggers.size());
    for (const auto& [name, logger] : state.Loggers) {
        loggers.emplace_back(logger.get());
    }

    return loggers;
}

void CycleLogs(const std::filesystem::path& directory) {
    try {
        if (!std::filesystem::exists(directory)) {
            return;
        }

        const auto cutoff = std::filesystem::file_time_type::clock::now() - std::chrono::days{7};

        for (const auto& entry : std::filesystem::directory_iterator{directory}) {
            if (entry.is_regular_file() && entry.path().extension() == ".log" &&
                entry.last_write_time() < cutoff) {
                std::filesystem::remove(entry.path());
            }
        }
    } catch (const std::exception& e) {
        Logger::Log(std::format("Cycling logs failed ({})", e.what()));
    }
}

} // namespace

void Logger::SetEnabled(bool enabled) { GetState().Enabled = enabled; }

bool Logger::IsEnabled() { return GetState().Enabled; }

void Logger::SetLevel(LogLevel level) { GetState().Level = level; }

LogLevel Logger::GetLevel() { return GetState().Level; }

void Logger::SetGameIdentifier(std::string identifier) {
    auto& state = GetState();
    std::scoped_lock lock{state.Mutex};
    state.GameIdentifier = std::move(identifier);
}

void Logger::SetVersionIdentifier(std::string identifier) {
    auto& state = GetState();
    std::scoped_lock lock{state.Mutex};
    state.VersionIdentifier = std::move(identifier);
}

void Logger::SetStorage(const std::filesystem::path& directory) {
    {
        auto& state = GetState();
        std::scoped_lock lock{state.Mutex};
        state.Storage = directory;
    }

    CycleLogs(directory);

    for (Logger* logger : GetAllLoggers()) {
        logger->onStorageChanged();
    }
}

void Logger::AddFilteredText(std::string_view text) {
    if (text.empty()) {
        return;
    }

    auto& state = GetState();
    std::scoped_lock lock{state.Mutex};
    state.Filters.emplace_back(text);
}

std::string Logger::ApplyFilters(std::string_view message) {
    auto& state = GetState();
    std::string result{message};

    std::scoped_lock lock{state.Mutex};

    for (const auto& filter : state.Filters) {
        for (auto position = result.find(filter); position != std::string::npos;
             position = result.find(filter, position + filter.size())) {
            result.replace(position, filter.size(), filter.size(), '*');
        }
    }

    return result;
}

void Logger::Error(const std::exception& e, std::string_view description, LoggingTarget target,
                   bool recursive) {
    error(e, description, target, {}, recursive);
}

void Logger::Error(const std::exception& e, std::string_view description, std::string_view name,
                   bool recursive) {
    error(e, description, std::nullopt, name, recursive);
}

void Logger::error(const std::exception& e, std::string_view description,
                   std::optional<LoggingTarget> target, std::string_view name, bool recursive) {
    log(description, target, name, LogLevel::Error, &e, true);

    if (!recursive) {
        return;
    }

    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception& inner) {
        error(inner, std::format("{} (inner)", description), target, name, true);
    } catch (...) { // NOLINT(bugprone-empty-catch): non-std nested exceptions are ignored
    }
}

void Logger::Log(std::string_view message, LoggingTarget target, LogLevel level,
                 bool outputToListeners) {
    log(message, target, {}, level, nullptr, outputToListeners);
}

void Logger::Log(std::string_view message, std::string_view name, LogLevel level,
                 bool outputToListeners) {
    log(message, std::nullopt, name, level, nullptr, outputToListeners);
}

void Logger::log(std::string_view message, std::optional<LoggingTarget> target,
                 std::string_view loggerName, LogLevel level, const std::exception* exception,
                 bool outputToListeners) {
    try {
        Logger& logger = target ? GetLogger(*target) : GetLogger(loggerName);
        logger.Add(message, level, exception, outputToListeners);
    } catch (...) { // NOLINT(bugprone-empty-catch): logging must never throw into the caller
    }
}

Logger& Logger::GetLogger(LoggingTarget target) { return GetLogger(magic_enum::enum_name(target)); }

Logger& Logger::GetLogger(std::string_view name) {
    auto& state = GetState();
    std::scoped_lock lock{state.Mutex};

    const std::string nameLower = ToLower(name);

    if (const auto it = state.Loggers.find(nameLower); it != state.Loggers.end()) {
        return *it->second;
    }

    std::unique_ptr<Logger> logger;

    if (const auto target =
            magic_enum::enum_cast<LoggingTarget>(name, magic_enum::case_insensitive)) {
        logger.reset(new Logger{*target});
    } else {
        logger.reset(new Logger{name});
    }

    return *state.Loggers.emplace(nameLower, std::move(logger)).first->second;
}

int Logger::AddListener(EntryListener listener) {
    auto& state = GetState();
    std::scoped_lock lock{state.Mutex};

    const int id = state.NextListenerId++;
    state.Listeners.emplace_back(id, std::move(listener));
    return id;
}

void Logger::RemoveListener(int id) {
    auto& state = GetState();
    std::scoped_lock lock{state.Mutex};

    std::erase_if(state.Listeners, [id](const auto& entry) { return entry.first == id; });
}

void Logger::Flush() {
    for (Logger* logger : GetAllLoggers()) {
        logger->flushAndClose();
    }
}

void Logger::FlushForShutdown() {
    Flush();

    auto& state = GetState();
    std::scoped_lock lock{state.Mutex};
    state.Listeners.clear();
}

Logger::Logger(LoggingTarget target) : Logger{magic_enum::enum_name(target)} { target_ = target; }

Logger::Logger(std::string_view name) : name_{ToLower(name)} {
    if (Trim(name_).empty()) {
        throw std::invalid_argument(
            "The name of a logger must be non-null and may not contain only white space.");
    }

    filename_ = std::format("{}.{}.log", GetState().SessionStartupTimestamp, name_);
}

void Logger::Debug(std::string_view message) {
    if constexpr (kIsDebugBuild) {
        Add(message, LogLevel::Debug);
    }
}

void Logger::Add(std::string_view message, LogLevel level, const std::exception* exception,
                 bool outputToListeners) {
    add(message, level, exception, outputToListeners && IsOutputToListeners());
}

void Logger::add(std::string_view message, LogLevel level, const std::exception* exception,
                 bool outputToListeners) {
    if (!IsEnabled() || level < GetLevel()) {
        return;
    }

    const std::string filteredMessage = ApplyFilters(message);
    std::string logOutput = filteredMessage;
    std::optional<std::string> exceptionText;

    if (exception != nullptr) {
        exceptionText = ApplyFilters(exception->what());
        logOutput += '\n';
        logOutput += *exceptionText;
    }

    const std::string timestamp = Timestamp();
    const std::string levelName = ToLower(magic_enum::enum_name(level));

    std::vector<std::string> lines = SplitLines(logOutput);
    for (auto& line : lines) {
        line = std::format("{} [{}]: {}", timestamp, levelName, line);
    }

    if (outputToListeners) {
        const LogEntry entry{.Level = level,
                             .Target = target_,
                             .LoggerName = name_,
                             .Message = filteredMessage,
                             .Exception = exceptionText};

        for (const auto& listener : CopyListeners()) {
            listener(entry);
        }

        if constexpr (kIsDebugBuild) {
            for (const auto& line : lines) {
                std::cout << '[' << name_ << "] " << line << '\n';
            }
        }
    }

    if (target_ == LoggingTarget::Information) {
        return;
    }

    std::scoped_lock lock{fileMutex_};

    pendingLines_.insert(pendingLines_.end(), std::make_move_iterator(lines.begin()),
                         std::make_move_iterator(lines.end()));
    writePendingLines();
}

void Logger::writePendingLines() {
    const auto directory = GetStorage();
    if (!directory || pendingLines_.empty()) {
        return;
    }

    const auto lines = std::exchange(pendingLines_, {});

    std::error_code error;
    std::filesystem::create_directories(*directory, error);

    if (!file_.is_open()) {
        file_.open(*directory / filename_, std::ios::app);

        if (!file_.is_open()) {
            return;
        }

        if (!headerAdded_) {
            file_ << "----------------------------------------------------------\n"
                  << std::format("{} Log (LogLevel: {})\n", name_,
                                 magic_enum::enum_name(GetLevel()))
                  << std::format("Running {}\n", DescribeSession())
                  << "----------------------------------------------------------\n";

            headerAdded_ = true;
        }
    }

    for (const auto& line : lines) {
        file_ << line << '\n';
    }

    file_.flush();
}

void Logger::flushAndClose() {
    std::scoped_lock lock{fileMutex_};

    writePendingLines();
    file_.close();
}

void Logger::onStorageChanged() {
    std::scoped_lock lock{fileMutex_};

    file_.close();
    headerAdded_ = false;
    writePendingLines();
}

} // namespace panlid::logging
