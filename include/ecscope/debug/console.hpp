#pragma once

#include "debug_system.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

namespace ECScope::Debug {

/**
 * @brief Interactive debug console with command system and remote debugging
 */
class Console {
public:
    enum class LogLevel {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warning = 3,
        Error = 4,
        Critical = 5
    };
    
    struct LogEntry {
        LogLevel level;
        std::string message;
        std::string category;
        std::chrono::system_clock::time_point timestamp;
        uint32_t thread_id;
        std::string file;
        int line;
        
        LogEntry() = default;
        LogEntry(LogLevel lvl, std::string msg, std::string cat = "")
            : level(lvl), message(std::move(msg)), category(std::move(cat)),
              timestamp(std::chrono::system_clock::now()) {}
    };
    
    struct CommandResult {
        bool success = false;
        std::string output;
        std::string error;
        double execution_time_ms = 0.0;
    };
    
    using CommandHandler = std::function<CommandResult(const std::vector<std::string>&)>;
    
    struct CommandInfo {
        std::string name;
        std::string description;
        std::string usage;
        std::vector<std::string> aliases;
        CommandHandler handler;
        bool admin_only = false;
    };

    struct ConsoleConfig {
        size_t max_log_entries = 10000;
        size_t max_command_history = 1000;
        LogLevel min_log_level = LogLevel::Debug;
        bool enable_auto_completion = true;
        bool enable_remote_access = false;
        uint16_t remote_port = 7777;
        std::string remote_bind_address = "127.0.0.1";
        bool enable_log_file = true;
        std::string log_file_path = "debug.log";
    };

    explicit Console(const ConsoleConfig& config = {});
    ~Console();

    // Core lifecycle
    void Initialize();
    void Update(float deltaTime);
    void Render();
    void Shutdown();

    // Logging interface
    void Log(LogLevel level, const std::string& message, const std::string& category = "");
    void Trace(const std::string& message, const std::string& category = "");
    void Debug(const std::string& message, const std::string& category = "");
    void Info(const std::string& message, const std::string& category = "");
    void Warning(const std::string& message, const std::string& category = "");
    void Error(const std::string& message, const std::string& category = "");
    void Critical(const std::string& message, const std::string& category = "");

    // Command system
    void RegisterCommand(const CommandInfo& command);
    void UnregisterCommand(const std::string& name);
    CommandResult ExecuteCommand(const std::string& command_line);
    
    // Auto-completion
    std::vector<std::string> GetCompletions(const std::string& partial_command);
    std::vector<std::string> GetCommandSuggestions(const std::string& partial_name);
    
    // Variables system
    template<typename T>
    void RegisterVariable(const std::string& name, T* variable, const std::string& description = "");
    
    void SetVariable(const std::string& name, const std::string& value);
    std::string GetVariable(const std::string& name);
    std::vector<std::string> GetVariableNames() const;

    // Remote debugging
    void EnableRemoteAccess(bool enable);
    bool IsRemoteAccessEnabled() const { return m_remote_enabled; }
    void SendRemoteMessage(const std::string& client_id, const std::string& message);

    // Console state
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    void Toggle() { m_visible = !m_visible; }

    // Log filtering and searching
    void SetLogFilter(LogLevel min_level) { m_config.min_log_level = min_level; }
    void SetCategoryFilter(const std::string& category) { m_category_filter = category; }
    void ClearCategoryFilter() { m_category_filter.clear(); }
    void SearchLogs(const std::string& query);
    void ClearSearch() { m_search_query.clear(); }

    // Configuration
    const ConsoleConfig& GetConfig() const { return m_config; }
    void UpdateConfig(const ConsoleConfig& config);

    // Statistics
    struct Stats {
        size_t total_log_entries = 0;
        size_t commands_executed = 0;
        size_t remote_connections = 0;
        double average_command_time_ms = 0.0;
    };
    
    const Stats& GetStats() const { return m_stats; }

private:
    ConsoleConfig m_config;
    Stats m_stats;
    
    // Console state
    bool m_initialized = false;
    bool m_visible = false;
    bool m_remote_enabled = false;
    
    // Log system
    std::vector<LogEntry> m_log_entries;
    std::queue<LogEntry> m_pending_logs;
    std::mutex m_log_mutex;
    std::ofstream m_log_file;
    
    // Filtering and search
    std::string m_category_filter;
    std::string m_search_query;
    std::vector<size_t> m_filtered_indices;
    
    // Command system
    std::unordered_map<std::string, CommandInfo> m_commands;
    std::unordered_map<std::string, std::string> m_command_aliases;
    std::vector<std::string> m_command_history;
    size_t m_history_index = 0;
    
    // Variables system
    struct VariableInfo {
        void* ptr;
        std::type_index type;
        std::string description;
        std::function<std::string()> getter;
        std::function<void(const std::string&)> setter;
    };
    std::unordered_map<std::string, VariableInfo> m_variables;
    
    // Input handling
    std::string m_input_buffer;
    size_t m_cursor_position = 0;
    bool m_auto_complete_active = false;
    std::vector<std::string> m_auto_complete_candidates;
    size_t m_auto_complete_index = 0;
    
    // Remote debugging
    std::unique_ptr<class RemoteDebugServer> m_remote_server;
    
    // Internal methods
    void ProcessPendingLogs();
    void UpdateFiltering();
    void RenderLogView();
    void RenderCommandInput();
    void RenderCommandHistory();
    void RenderVariableEditor();
    void RenderRemoteClients();
    
    void HandleInput();
    void HandleAutoComplete();
    void AddToHistory(const std::string& command);
    
    CommandResult ParseAndExecuteCommand(const std::string& command_line);
    std::vector<std::string> TokenizeCommand(const std::string& command_line);
    
    void RegisterBuiltinCommands();
    void InitializeRemoteServer();
    void ShutdownRemoteServer();
    
    // Built-in commands
    CommandResult CmdHelp(const std::vector<std::string>& args);
    CommandResult CmdClear(const std::vector<std::string>& args);
    CommandResult CmdEcho(const std::vector<std::string>& args);
    CommandResult CmdSet(const std::vector<std::string>& args);
    CommandResult CmdGet(const std::vector<std::string>& args);
    CommandResult CmdList(const std::vector<std::string>& args);
    CommandResult CmdHistory(const std::vector<std::string>& args);
    CommandResult CmdLogLevel(const std::vector<std::string>& args);
    CommandResult CmdSave(const std::vector<std::string>& args);
    CommandResult CmdLoad(const std::vector<std::string>& args);
    CommandResult CmdProfiler(const std::vector<std::string>& args);
    CommandResult CmdMemory(const std::vector<std::string>& args);
    CommandResult CmdSystem(const std::vector<std::string>& args);
    CommandResult CmdEntity(const std::vector<std::string>& args);
    CommandResult CmdAsset(const std::vector<std::string>& args);
    CommandResult CmdScript(const std::vector<std::string>& args);
};

/**
 * @brief Remote debug server for networked debugging
 */
class RemoteDebugServer {
public:
    struct RemoteClient {
        std::string id;
        std::string address;
        std::chrono::system_clock::time_point connected_time;
        bool authenticated = false;
        std::string username;
        
        // Communication
        std::unique_ptr<class NetworkSocket> socket;
        std::queue<std::string> outgoing_messages;
        std::mutex message_mutex;
    };

    explicit RemoteDebugServer(Console& console, uint16_t port, const std::string& bind_address);
    ~RemoteDebugServer();

    void Start();
    void Stop();
    void Update();

    // Client management
    std::vector<std::string> GetConnectedClients() const;
    void DisconnectClient(const std::string& client_id);
    void SendMessage(const std::string& client_id, const std::string& message);
    void BroadcastMessage(const std::string& message);

    // Authentication
    void SetPassword(const std::string& password) { m_password = password; }
    bool RequiresAuthentication() const { return !m_password.empty(); }

private:
    Console& m_console;
    uint16_t m_port;
    std::string m_bind_address;
    std::string m_password;
    
    std::atomic<bool> m_running{false};
    std::thread m_server_thread;
    std::unique_ptr<class NetworkSocket> m_server_socket;
    
    std::vector<std::unique_ptr<RemoteClient>> m_clients;
    mutable std::mutex m_clients_mutex;
    
    void ServerThreadFunc();
    void AcceptConnections();
    void ProcessClientMessages();
    void HandleClientMessage(RemoteClient& client, const std::string& message);
    void HandleClientDisconnect(RemoteClient& client);
    
    std::string GenerateClientId();
    bool AuthenticateClient(RemoteClient& client, const std::string& password);
};

/**
 * @brief Script execution environment for console
 */
class ScriptEngine {
public:
    struct ScriptResult {
        bool success = false;
        std::string output;
        std::string error;
        double execution_time_ms = 0.0;
    };

    explicit ScriptEngine(Console& console);
    ~ScriptEngine();

    // Script execution
    ScriptResult ExecuteScript(const std::string& script_code, const std::string& language = "lua");
    ScriptResult ExecuteFile(const std::string& file_path);
    
    // Environment setup
    void RegisterFunction(const std::string& name, std::function<void()> func);
    void RegisterVariable(const std::string& name, void* ptr, const std::string& type);
    
    // Supported languages
    bool IsLanguageSupported(const std::string& language) const;
    std::vector<std::string> GetSupportedLanguages() const;

private:
    Console& m_console;
    
    // Language engines
    std::unique_ptr<class LuaEngine> m_lua_engine;
    std::unique_ptr<class PythonEngine> m_python_engine;
    
    void InitializeLua();
    void InitializePython();
    void ExposeConsoleAPI();
};

/**
 * @brief Crash dump analyzer for post-mortem debugging
 */
class CrashAnalyzer {
public:
    struct CrashInfo {
        std::string crash_type;
        std::string signal_name;
        int signal_code = 0;
        void* crash_address = nullptr;
        
        std::vector<std::string> callstack;
        std::unordered_map<std::string, std::string> registers;
        std::vector<std::pair<std::string, std::string>> memory_regions;
        
        std::chrono::system_clock::time_point timestamp;
        std::string build_info;
        std::string platform_info;
    };

    CrashAnalyzer();
    ~CrashAnalyzer();

    // Crash handling setup
    void InstallCrashHandlers();
    void UninstallCrashHandlers();
    
    // Dump analysis
    CrashInfo AnalyzeDumpFile(const std::string& dump_path);
    void GenerateCrashReport(const CrashInfo& info, const std::string& output_path);
    
    // Minidump integration (Windows)
    void EnableMinidumps(const std::string& dump_directory);
    void DisableMinidumps();

private:
    bool m_handlers_installed = false;
    std::string m_dump_directory;
    
    static void CrashHandler(int signal);
    static void* GetStackTrace(size_t max_frames);
    static std::string ResolveSymbol(void* address);
    
    void WriteCrashDump(const CrashInfo& info);
    std::string FormatCallstack(const std::vector<std::string>& callstack);
};

// Template implementation
template<typename T>
void Console::RegisterVariable(const std::string& name, T* variable, const std::string& description) {
    VariableInfo info;
    info.ptr = variable;
    info.type = std::type_index(typeid(T));
    info.description = description;
    
    info.getter = [variable]() {
        if constexpr (std::is_arithmetic_v<T>) {
            return std::to_string(*variable);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return *variable;
        } else if constexpr (std::is_same_v<T, bool>) {
            return *variable ? "true" : "false";
        } else {
            return std::string("unsupported_type");
        }
    };
    
    info.setter = [variable](const std::string& value) {
        if constexpr (std::is_arithmetic_v<T>) {
            if constexpr (std::is_integral_v<T>) {
                *variable = static_cast<T>(std::stoll(value));
            } else {
                *variable = static_cast<T>(std::stod(value));
            }
        } else if constexpr (std::is_same_v<T, std::string>) {
            *variable = value;
        } else if constexpr (std::is_same_v<T, bool>) {
            *variable = (value == "true" || value == "1" || value == "yes");
        }
    };
    
    m_variables[name] = info;
}

// Logging macros
#define ECSCOPE_LOG_TRACE(console, message, ...) \
    console.Trace(fmt::format(message, ##__VA_ARGS__))

#define ECSCOPE_LOG_DEBUG(console, message, ...) \
    console.Debug(fmt::format(message, ##__VA_ARGS__))

#define ECSCOPE_LOG_INFO(console, message, ...) \
    console.Info(fmt::format(message, ##__VA_ARGS__))

#define ECSCOPE_LOG_WARNING(console, message, ...) \
    console.Warning(fmt::format(message, ##__VA_ARGS__))

#define ECSCOPE_LOG_ERROR(console, message, ...) \
    console.Error(fmt::format(message, ##__VA_ARGS__))

#define ECSCOPE_LOG_CRITICAL(console, message, ...) \
    console.Critical(fmt::format(message, ##__VA_ARGS__))

} // namespace ECScope::Debug