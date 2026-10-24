#pragma once

#include "panlid/logging/LogEntry.hh"
#include "panlid/logging/LogLevel.hh"
#include "panlid/logging/LoggingTarget.hh"
#include <atomic>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace panlid::logging {

class Logger {
  public:
    using EntryListener = std::function<void(const LogEntry&)>;

    static void SetEnabled(bool enabled);

    [[nodiscard]] static bool IsEnabled();

    static void SetLevel(LogLevel level);

    [[nodiscard]] static LogLevel GetLevel();

    static void SetGameIdentifier(std::string identifier);

    static void SetVersionIdentifier(std::string identifier);

    static void SetStorage(const std::filesystem::path& directory);

    static void AddFilteredText(std::string_view text);

    [[nodiscard]] static std::string ApplyFilters(std::string_view message);

    static void Error(const std::exception& e, std::string_view description,
                      LoggingTarget target = LoggingTarget::Runtime, bool recursive = false);

    static void Error(const std::exception& e, std::string_view description, std::string_view name,
                      bool recursive = false);

    static void Log(std::string_view message, LoggingTarget target = LoggingTarget::Runtime,
                    LogLevel level = LogLevel::Verbose, bool outputToListeners = true);

    static void Log(std::string_view message, std::string_view name,
                    LogLevel level = LogLevel::Verbose, bool outputToListeners = true);

    [[nodiscard]] static Logger& GetLogger(LoggingTarget target = LoggingTarget::Runtime);

    [[nodiscard]] static Logger& GetLogger(std::string_view name);

    static int AddListener(EntryListener listener);

    static void RemoveListener(int id);

    static void Flush();

    [[nodiscard]] const std::optional<LoggingTarget>& GetTarget() const { return target_; }

    [[nodiscard]] const std::string& GetName() const { return name_; }

    [[nodiscard]] const std::string& GetFilename() const { return filename_; }

    [[nodiscard]] bool IsOutputToListeners() const { return outputToListeners_; }

    void SetOutputToListeners(bool value) { outputToListeners_ = value; }

    void Debug(std::string_view message = {});

    void Add(std::string_view message = {}, LogLevel level = LogLevel::Verbose,
             const std::exception* exception = nullptr, bool outputToListeners = true);

  private:
    explicit Logger(LoggingTarget target);
    explicit Logger(std::string_view name);

    static void error(const std::exception& e, std::string_view description,
                      std::optional<LoggingTarget> target, std::string_view name, bool recursive);

    static void log(std::string_view message, std::optional<LoggingTarget> target,
                    std::string_view loggerName, LogLevel level, const std::exception* exception,
                    bool outputToListeners);

    static void flushForShutdown();

    void add(std::string_view message, LogLevel level, const std::exception* exception,
             bool outputToListeners);

    void writePendingLines();
    void flushAndClose();
    void onStorageChanged();

    std::optional<LoggingTarget> target_;
    std::string name_;
    std::string filename_;
    std::atomic<bool> outputToListeners_{true};

    std::mutex fileMutex_;
    std::vector<std::string> pendingLines_;
    std::ofstream file_;
    bool headerAdded_{};
};

} // namespace panlid::logging
