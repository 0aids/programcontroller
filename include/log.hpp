// Just a basic logging system.
#pragma once
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream> // Required for stringstream
#include <string>

// --- Color Codes ---
const char* const RESET_COLOR  = "\033[0m";
const char* const RED_COLOR    = "\033[31m";
const char* const YELLOW_COLOR = "\033[33m";
const char* const BLUE_COLOR   = "\033[34m";
const char* const GREY_COLOR   = "\033[90m";

// --- Log Levels ---
enum LogLevel {
    Error,   // Critical errors
    Warning, // Potential issues
    Message, // Key application events
    Verbose, // Detailed state information
    Debug    // Granular debugging information
};

// --- Null Stream for Discarding Logs ---
class NullBuffer : public std::streambuf {
  public:
    int overflow(int c) override {
        return c;
    }
};
static NullBuffer   null_buffer;
static std::ostream cnull(&null_buffer);

// --- Logger Class ---
class LogStreamWrapper; // Forward declaration

class Logger {
  public:
    friend class LogStreamWrapper; // Allow wrapper to access private members

    // Init
    static void initlogger(LogLevel level);

    static void setLevel(LogLevel newLevel) {
        flush(); // Finalize any pending repeated log before changing level
        currentLevel = newLevel;
    }

    static void setMode(std::ios_base::fmtflags flags) {
        s_nextLogFlags = flags;
        s_modeIsSet    = true;
    }

    // Call this before your program exits to finalize any repeated log lines.
    static void flush() {
        if (s_repeatCount > 1) {
            std::cout << std::endl;
        }
        s_lastSignature = "";
        s_repeatCount   = 0;
    }

    static LogStreamWrapper log(LogLevel level, const char* file, const char* func);

  private:
    // This function contains the core logic for coalescing logs
    static void                    commit(const std::string& prefix, const std::string& content);

    static LogLevel                currentLevel;
    static std::ios_base::fmtflags s_nextLogFlags;
    static bool                    s_modeIsSet;

    // Static members for tracking repeated logs
    static std::string s_lastSignature;
    static size_t      s_repeatCount;
};

class LogStreamWrapper {
  public:
    LogStreamWrapper(std::ostream& stream, bool applyFormatting, std::ios_base::fmtflags flags, const std::string& prefix) :
        m_stream(stream), m_applyFormatting(applyFormatting), m_prefix(prefix) {
        if (m_applyFormatting) {
            m_originalFlags = m_stream.flags();
            m_stream.flags(flags);
        }
    }

    ~LogStreamWrapper() {
        // On destruction, commit the buffered content to the Logger
        if (&m_stream != &cnull) {
            Logger::commit(m_prefix, m_buffer.str());
        }
        // Restore stream flags if they were changed
        if (m_applyFormatting) {
            m_stream.flags(m_originalFlags);
        }
    }

    template <typename T>
    LogStreamWrapper& operator<<(const T& msg) {
        m_buffer << msg; // Buffer the message instead of writing directly
        return *this;
    }

  private:
    std::ostream&           m_stream;
    std::ios_base::fmtflags m_originalFlags;
    bool                    m_applyFormatting;
    std::string             m_prefix;
    std::stringstream       m_buffer; // Internal buffer for the message
};

// --- Logger Method Implementations ---
inline LogStreamWrapper Logger::log(LogLevel level, const char* file, const char* func) {
    if (level > currentLevel) {
        return LogStreamWrapper(cnull, false, {}, "");
    }

    std::ostream& stream = (level == LogLevel::Error) ? std::cerr : std::cout;

    // Build the prefix string
    std::string levelStr;
    const char* colorCode = "";
    switch (level) {
        case LogLevel::Error:
            levelStr  = "ERROR";
            colorCode = RED_COLOR;
            break;
        case LogLevel::Warning:
            levelStr  = "WARNING";
            colorCode = YELLOW_COLOR;
            break;
        case LogLevel::Message:
            levelStr  = "MESSAGE";
            colorCode = BLUE_COLOR;
            break;
        case LogLevel::Verbose:
            levelStr  = "VERBOSE";
            colorCode = "";
            break;
        case LogLevel::Debug:
            levelStr  = "DEBUG";
            colorCode = GREY_COLOR;
            break;
    }

    std::string filename(file);
    size_t      last_slash = filename.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        filename = filename.substr(last_slash + 1);
    }

    std::stringstream prefix_ss;
    prefix_ss << colorCode << "[" << std::setw(7) << std::left << levelStr << "] "
              << "[" << filename << ":" << func << "] ";

    if (s_modeIsSet) {
        s_modeIsSet = false;
        return LogStreamWrapper(stream, true, s_nextLogFlags, prefix_ss.str());
    }
    return LogStreamWrapper(stream, false, {}, prefix_ss.str());
}

inline void Logger::commit(const std::string& prefix, const std::string& content) {
    std::string currentSignature = prefix + content;

    if (s_lastSignature == currentSignature) {
        // Repeated log: Overwrite the previous line with the new count.
        s_repeatCount++;
        std::cout << "\r" << prefix << content << " (count: " << s_repeatCount << ")" << RESET_COLOR << std::flush;
    } else {
        // New log: Finalize the previous line (if there was one) and print the new
        // one.
        if (!s_lastSignature.empty()) {
            std::cout << std::endl;
        }
        std::cout << prefix << content << RESET_COLOR << std::flush;
        s_lastSignature = currentSignature;
        s_repeatCount   = 1;
    }
}

// --- Macros ---
#define Log(level, msg) (Logger::log)(level, __FILE__, __func__) << msg

#define Log_var(level, var) Log(level, #var << " = " << var)

#define Log_mode(level, mode, msg)                                                                                                                                                 \
    {                                                                                                                                                                              \
        Logger::setMode(mode);                                                                                                                                                     \
        Log(level, msg);                                                                                                                                                           \
    }

#define Log_modevar(level, mode, var)                                                                                                                                              \
    {                                                                                                                                                                              \
        Logger::setMode(mode);                                                                                                                                                     \
        Log_var(level, var);                                                                                                                                                       \
    }

#define HEX_AND_SHOWBASE std::ios_base::hex | std::ios_base::showbase
