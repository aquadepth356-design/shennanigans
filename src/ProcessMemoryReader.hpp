#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <glm/glm.hpp>

// CS2 bone matrices are 3x4 row-major (48 bytes each), NOT 4x4 (64 bytes).
struct BoneMatrix3x4 {
    float m[3][4];
};

class ProcessMemoryReader {
public:
    explicit ProcessMemoryReader(const std::wstring& processName)
        : m_handle(nullptr), m_pid(0), m_clientBase(0)
    {
        m_pid = findPID(processName);
        if (m_pid == 0)
            throw std::runtime_error("Process not found. Is cs2.exe running?");

        m_handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, m_pid);
        if (!m_handle)
            throw std::runtime_error("OpenProcess failed. Try running as Administrator.");

        m_clientBase = findModuleBase(L"client.dll");
        if (m_clientBase == 0)
            throw std::runtime_error("Could not find client.dll base address.");
    }

    ~ProcessMemoryReader() {
        if (m_handle) { CloseHandle(m_handle); m_handle = nullptr; }
    }

    ProcessMemoryReader(const ProcessMemoryReader&)            = delete;
    ProcessMemoryReader& operator=(const ProcessMemoryReader&) = delete;

    template<typename T>
    bool tryRead(uintptr_t address, T& out) const {
        if (!address) return false;
        SIZE_T bytesRead = 0;
        BOOL ok = ReadProcessMemory(m_handle,
            reinterpret_cast<LPCVOID>(address), &out, sizeof(T), &bytesRead);
        return ok && bytesRead == sizeof(T);
    }

    template<typename T>
    T readAbsolute(uintptr_t address) const {
        T value{};
        tryRead(address, value);
        return value;
    }

    template<typename T>
    T readClient(uintptr_t offset) const {
        return readAbsolute<T>(m_clientBase + offset);
    }

    glm::mat4 readMatrix4x4(uintptr_t address) const {
        return readAbsolute<glm::mat4>(address);
    }

    // Read CS2 bone matrices - 3x4 row-major (48 bytes each).
    // Converts to glm::mat4 so translation = mat[0][3], mat[1][3], mat[2][3]
    // FIX: Accept partial reads - ReadProcessMemory returns FALSE with ERROR_PARTIAL_COPY
    // when the read spans a page boundary, but bytesRead will still be > 0.
    std::vector<glm::mat4> readBoneMatrices(uintptr_t address, size_t count) const {
        if (!address) return {};

        std::vector<BoneMatrix3x4> raw(count);
        SIZE_T bytesRead = 0;
        ReadProcessMemory(m_handle,
            reinterpret_cast<LPCVOID>(address),
            raw.data(), sizeof(BoneMatrix3x4) * count, &bytesRead);

        // FIX: Only bail if ZERO bytes came back - partial reads are fine and expected
        if (bytesRead == 0) return {};

        size_t gotBones = bytesRead / sizeof(BoneMatrix3x4);
        std::vector<glm::mat4> matrices(gotBones);

        for (size_t i = 0; i < gotBones; ++i) {
            const auto& r = raw[i];
            matrices[i][0] = glm::vec4(r.m[0][0], r.m[0][1], r.m[0][2], r.m[0][3]);
            matrices[i][1] = glm::vec4(r.m[1][0], r.m[1][1], r.m[1][2], r.m[1][3]);
            matrices[i][2] = glm::vec4(r.m[2][0], r.m[2][1], r.m[2][2], r.m[2][3]);
            matrices[i][3] = glm::vec4(0.f, 0.f, 0.f, 1.f);
        }
        return matrices;
    }

    size_t readRaw(uintptr_t address, void* outBuf, size_t size) const {
        if (!address || !outBuf) return 0;
        SIZE_T bytesRead = 0;
        ReadProcessMemory(m_handle,
            reinterpret_cast<LPCVOID>(address), outBuf, size, &bytesRead);
        return static_cast<size_t>(bytesRead);
    }

    uintptr_t clientBase() const { return m_clientBase; }
    DWORD     pid()        const { return m_pid; }

private:
    HANDLE    m_handle;
    DWORD     m_pid;
    uintptr_t m_clientBase;

    static DWORD findPID(const std::wstring& processName) {
        DWORD pid = 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        PROCESSENTRY32W entry{}; entry.dwSize = sizeof(entry);
        if (Process32FirstW(snap, &entry)) {
            do {
                if (processName == entry.szExeFile) { pid = entry.th32ProcessID; break; }
            } while (Process32NextW(snap, &entry));
        }
        CloseHandle(snap);
        return pid;
    }

    uintptr_t findModuleBase(const std::wstring& moduleName) const {
        uintptr_t base = 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_pid);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        MODULEENTRY32W entry{}; entry.dwSize = sizeof(entry);
        if (Module32FirstW(snap, &entry)) {
            do {
                if (moduleName == entry.szModule) {
                    base = reinterpret_cast<uintptr_t>(entry.modBaseAddr); break;
                }
            } while (Module32NextW(snap, &entry));
        }
        CloseHandle(snap);
        return base;
    }
};
