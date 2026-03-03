#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <mutex>
#include <Windows.h>
#include <wininet.h>
#include "../../../sdk/sdk.h"

#pragma comment(lib, "wininet.lib")

namespace mesh_utils {
    struct vertex {
        rbx::vector3_t pos;
        rbx::vector3_t normal;
        rbx::vector2_t uv;
    };

    struct mesh_data {
        std::vector<vertex> vertices;
        std::vector<uint32_t> indices;
        bool loaded = false;
        bool failed = false;
    };

    inline std::map<std::string, mesh_data> mesh_cache;
    inline std::vector<std::string> pending_downloads;
    inline std::mutex cache_mutex;
    inline std::mutex download_mutex;
    inline bool worker_running = false;

    inline std::string download_asset(const std::string& asset_id) {
        std::string id = asset_id;
        if (id.find("rbxassetid://") != std::string::npos) {
            id = id.substr(13);
        } else if (id.find("http") != std::string::npos) {
            size_t last_slash = id.find_last_of('=');
            if (last_slash != std::string::npos) {
                id = id.substr(last_slash + 1);
            } else {
                size_t last_s = id.find_last_of('/');
                if (last_s != std::string::npos) id = id.substr(last_s + 1);
            }
        }

        std::string url = "https://assetdelivery.roblox.com/v1/asset/?id=" + id;

        HINTERNET hInternet = InternetOpenA("MeshDownloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) return "";

        HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hUrl) {
            InternetCloseHandle(hInternet);
            return "";
        }

        std::string content;
        char buffer[4096];
        DWORD bytesRead;
        while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            content.append(buffer, bytesRead);
        }

        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return content;
    }

    inline bool parse_mesh_v1(const std::string& data, mesh_data& mesh);
    inline bool parse_mesh_v2(const std::string& data, mesh_data& mesh);
    inline bool parse_mesh_v3(const std::string& data, mesh_data& mesh);
    inline bool parse_mesh_v4_plus(const std::string& data, mesh_data& mesh);

    inline void download_worker() {
        while (worker_running) {
            std::string id;
            {
                std::lock_guard<std::mutex> lock(download_mutex);
                if (!pending_downloads.empty()) {
                    id = pending_downloads.back();
                    pending_downloads.pop_back();
                }
            }

            if (!id.empty()) {
                std::string data = download_asset(id);
                mesh_data mesh;
                bool success = false;

                if (!data.empty()) {
                    if (data.find("version 1.00") != std::string::npos) success = parse_mesh_v1(data, mesh);
                    else if (data.find("version 2.00") != std::string::npos) success = parse_mesh_v2(data, mesh);
                    else if (data.find("version 3.00") != std::string::npos || data.find("version 3.01") != std::string::npos) success = parse_mesh_v3(data, mesh);
                    else if (data.size() > 12 && data.substr(0, 8) == "version ") success = parse_mesh_v4_plus(data, mesh);
                }

                if (success && mesh.loaded) {
                    std::lock_guard<std::mutex> lock(cache_mutex);
                    mesh_cache[id] = mesh;
                } else {
                    mesh.failed = true;
                    std::lock_guard<std::mutex> lock(cache_mutex);
                    mesh_cache[id] = mesh;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    inline void init() {
        if (!worker_running) {
            worker_running = true;
            std::thread(download_worker).detach();
        }
    }

    inline mesh_data* get_mesh(const std::string& mesh_id) {
        if (mesh_id.empty()) return nullptr;

        init();

        std::lock_guard<std::mutex> lock(cache_mutex);
        if (mesh_cache.count(mesh_id)) {
            mesh_data* mesh = &mesh_cache[mesh_id];
            if (mesh->failed) return nullptr;
            return mesh;
        }

        {
            std::lock_guard<std::mutex> dlock(download_mutex);
            if (std::find(pending_downloads.begin(), pending_downloads.end(), mesh_id) == pending_downloads.end()) {
                pending_downloads.push_back(mesh_id);
            }
        }

        return nullptr;
    }

    inline bool parse_mesh_v1(const std::string& data, mesh_data& mesh) {
        std::stringstream ss(data);
        std::string version;
        std::getline(ss, version);
        if (version.find("version 1.00") == std::string::npos) return false;

        int num_faces;
        ss >> num_faces;

        for (int i = 0; i < num_faces * 3; ++i) {
            vertex v;
            char dummy;
            ss >> dummy;
            ss >> v.pos.x >> dummy >> v.pos.y >> dummy >> v.pos.z >> dummy;
            ss >> dummy;
            ss >> v.normal.x >> dummy >> v.normal.y >> dummy >> v.normal.z >> dummy;
            ss >> dummy;
            ss >> v.uv.x >> dummy >> v.uv.y >> dummy;

            mesh.vertices.push_back(v);
            mesh.indices.push_back(i);
        }
        mesh.loaded = true;
        return true;
    }

    inline bool parse_mesh_v2(const std::string& data, mesh_data& mesh) {
        size_t nl = data.find('\n');
        if (nl == std::string::npos) return false;

        const char* start_ptr = data.data() + nl + 1;
        if (data.size() < (size_t)(start_ptr - data.data() + 12)) return false;

        uint16_t headerSize = *reinterpret_cast<const uint16_t*>(start_ptr);
        uint8_t vertexSize = *reinterpret_cast<const uint8_t*>(start_ptr + 2);
        uint8_t faceSize = *reinterpret_cast<const uint8_t*>(start_ptr + 3);
        uint32_t vertexCount = *reinterpret_cast<const uint32_t*>(start_ptr + 4);
        uint32_t faceCount = *reinterpret_cast<const uint32_t*>(start_ptr + 8);

        if (vertexCount > 100000 || faceCount > 200000) return false;

        const char* ptr = start_ptr + headerSize;
        if (data.size() < (size_t)((ptr - data.data()) + (vertexCount * vertexSize) + (faceCount * faceSize))) return false;

        for (uint32_t i = 0; i < vertexCount; ++i) {
            const char* vertex_ptr = ptr + (i * vertexSize);
            vertex v;
            v.pos = *reinterpret_cast<const rbx::vector3_t*>(vertex_ptr);
            v.normal = *reinterpret_cast<const rbx::vector3_t*>(vertex_ptr + 12);
            v.uv = *reinterpret_cast<const rbx::vector2_t*>(vertex_ptr + 24);
            mesh.vertices.push_back(v);
        }

        ptr += (vertexCount * vertexSize);

        for (uint32_t i = 0; i < faceCount; ++i) {
            const char* face_ptr = ptr + (i * faceSize);
            if (faceSize == 6) {
                mesh.indices.push_back(*reinterpret_cast<const uint16_t*>(face_ptr));
                mesh.indices.push_back(*reinterpret_cast<const uint16_t*>(face_ptr + 2));
                mesh.indices.push_back(*reinterpret_cast<const uint16_t*>(face_ptr + 4));
            } else {
                mesh.indices.push_back(*reinterpret_cast<const uint32_t*>(face_ptr));
                mesh.indices.push_back(*reinterpret_cast<const uint32_t*>(face_ptr + 4));
                mesh.indices.push_back(*reinterpret_cast<const uint32_t*>(face_ptr + 8));
            }
        }

        mesh.loaded = true;
        return true;
    }

    inline bool parse_mesh_v3(const std::string& data, mesh_data& mesh) {
        size_t nl = data.find('\n');
        if (nl == std::string::npos) return false;

        const char* start_ptr = data.data() + nl + 1;
        if (data.size() < (size_t)(start_ptr - data.data() + 12)) return false;

        uint16_t headerSize = *reinterpret_cast<const uint16_t*>(start_ptr);
        uint8_t vertexSize = *reinterpret_cast<const uint8_t*>(start_ptr + 2);
        uint8_t faceSize = *reinterpret_cast<const uint8_t*>(start_ptr + 3);

        uint32_t vertexCount, faceCount;
        if (headerSize >= 16) {
            vertexCount = *reinterpret_cast<const uint32_t*>(start_ptr + 8);
            faceCount = *reinterpret_cast<const uint32_t*>(start_ptr + 12);
        } else {
            vertexCount = *reinterpret_cast<const uint32_t*>(start_ptr + 6);
            faceCount = *reinterpret_cast<const uint32_t*>(start_ptr + 10);
        }

        if (vertexCount > 100000 || faceCount > 200000) return false;

        const char* ptr = start_ptr + headerSize;
        if (data.size() < (size_t)((ptr - data.data()) + (vertexCount * vertexSize) + (faceCount * faceSize))) return false;

        for (uint32_t i = 0; i < vertexCount; ++i) {
            const char* vertex_ptr = ptr + (i * vertexSize);
            vertex v;
            v.pos = *reinterpret_cast<const rbx::vector3_t*>(vertex_ptr);
            v.normal = *reinterpret_cast<const rbx::vector3_t*>(vertex_ptr + 12);
            v.uv = *reinterpret_cast<const rbx::vector2_t*>(vertex_ptr + 24);
            mesh.vertices.push_back(v);
        }

        ptr += (vertexCount * vertexSize);

        for (uint32_t i = 0; i < faceCount; ++i) {
            const char* face_ptr = ptr + (i * faceSize);
            if (faceSize == 6) {
                mesh.indices.push_back(*reinterpret_cast<const uint16_t*>(face_ptr));
                mesh.indices.push_back(*reinterpret_cast<const uint16_t*>(face_ptr + 2));
                mesh.indices.push_back(*reinterpret_cast<const uint16_t*>(face_ptr + 4));
            } else {
                mesh.indices.push_back(*reinterpret_cast<const uint32_t*>(face_ptr));
                mesh.indices.push_back(*reinterpret_cast<const uint32_t*>(face_ptr + 4));
                mesh.indices.push_back(*reinterpret_cast<const uint32_t*>(face_ptr + 8));
            }
        }

        mesh.loaded = true;
        return true;
    }

    inline bool parse_mesh_v4_plus(const std::string& data, mesh_data& mesh) {
        size_t nl = data.find('\n');
        if (nl == std::string::npos) return false;

        const char* start_ptr = data.data() + nl + 1;
        if (*start_ptr == '\r') start_ptr++;
        if (*start_ptr == '\n') start_ptr++;

        if (data.size() < (size_t)(start_ptr - data.data() + 12)) return false;

        uint16_t headerSize = *reinterpret_cast<const uint16_t*>(start_ptr);
        uint8_t vertexSize = *reinterpret_cast<const uint8_t*>(start_ptr + 2);
        uint8_t faceSize = *reinterpret_cast<const uint8_t*>(start_ptr + 3);

        uint32_t vertexCount, faceCount;
        if (headerSize >= 16) {
            vertexCount = *reinterpret_cast<const uint32_t*>(start_ptr + 8);
            faceCount = *reinterpret_cast<const uint32_t*>(start_ptr + 12);
        } else {
            vertexCount = *reinterpret_cast<const uint32_t*>(start_ptr + 6);
            faceCount = *reinterpret_cast<const uint32_t*>(start_ptr + 10);
        }

        if (vertexCount > 100000 || faceCount > 200000) return false;

        const char* ptr = start_ptr + headerSize;
        if (data.size() < (size_t)((ptr - data.data()) + (vertexCount * vertexSize) + (faceCount * faceSize))) return false;

        for (uint32_t i = 0; i < vertexCount; ++i) {
            const char* vertex_ptr = ptr + (i * vertexSize);
            vertex v;
            v.pos = *reinterpret_cast<const rbx::vector3_t*>(vertex_ptr);
            mesh.vertices.push_back(v);
        }

        ptr += (vertexCount * vertexSize);

        for (uint32_t i = 0; i < faceCount; ++i) {
            const char* face_ptr = ptr + (i * faceSize);
            if (faceSize == 6) {
                mesh.indices.push_back(*reinterpret_cast<const uint16_t*>(face_ptr));
                mesh.indices.push_back(*reinterpret_cast<const uint16_t*>(face_ptr + 2));
                mesh.indices.push_back(*reinterpret_cast<const uint16_t*>(face_ptr + 4));
            } else {
                mesh.indices.push_back(*reinterpret_cast<const uint32_t*>(face_ptr));
                mesh.indices.push_back(*reinterpret_cast<const uint32_t*>(face_ptr + 4));
                mesh.indices.push_back(*reinterpret_cast<const uint32_t*>(face_ptr + 8));
            }
        }

        mesh.loaded = true;
        return true;
    }
}

