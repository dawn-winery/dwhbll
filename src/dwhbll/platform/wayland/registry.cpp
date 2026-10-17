#include <dwhbll/platform/wayland/registry.h>

#include <dwhbll/platform/wayland/display.h>

using namespace dwhbll::stl_ext;

namespace dwhbll::platform::wayland {

const wl_registry_listener registry::listener_ = {
    .global = &registry::handle_global,
    .global_remove = &registry::handle_global_remove
};

registry::registry(display& disp) : display_(&disp) {
    registry_ = wl_display_get_registry(display_->wl_display());
    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(registry_), display_->event_queue());
    wl_registry_add_listener(registry_, &listener_, this);
}

registry::~registry() {
    if (registry_)
        wl_registry_destroy(registry_);
}


Result<void*, std::string> registry::bind_raw(const wl_interface* iface, uint32_t version) {
    auto it = globals_by_iface_.find(iface->name);
    const global* entry = nullptr;

    if (it != globals_by_iface_.end() && !it->second.empty()) {
        auto g = globals_by_name_.find(it->second.front());
        if (g != globals_by_name_.end()) {
            entry = &g->second;
        }
    }

    if (!entry)
        return std::format("Compositor does not have global '{}'", iface->name);

    uint32_t target_version = std::min(version, entry->version);
    void* proxy = wl_registry_bind(registry_, entry->name, iface, target_version);
    if (!proxy)
        return std::format("Failed to bind global '{}'", iface->name);

    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(proxy), display_->event_queue());
    return proxy;
}

bool registry::has(std::string_view iface_name) const noexcept {
    auto it = globals_by_iface_.find(std::string(iface_name));
    return it != globals_by_iface_.end() && !it->second.empty();
}

std::uint64_t registry::add_observer(std::function<void(const global&)> on_add,
                                     std::function<void(uint32_t)> on_remove) {
    static uint64_t next_id = 0;
    uint64_t id = next_id++;
    observers_.emplace(id, observer { std::move(on_add), std::move(on_remove) });

    if (observers_[id].on_add) {
        for (const auto& [name, entry] : globals_by_name_) {
            observers_[id].on_add(entry);
        }
    }
    return id;
}

void registry::remove_observer(uint64_t id) {
    observers_.erase(id);
}

void registry::handle_global(void* data, wl_registry*, uint32_t name, 
                             const char* iface, uint32_t ver) {
    auto* self = static_cast<registry*>(data);
    global entry {name, iface, ver};

    self->globals_by_name_[name] = entry;
    self->globals_by_iface_[entry.interface].push_back(name);

    // This is kind of an hack in case the entries are modified while iterating
    std::vector<uint64_t> ids;
    ids.reserve(self->observers_.size());
    for (auto& [id, obs] : self->observers_) {
        ids.push_back(id);
    }

    for (uint64_t id : ids) {
        auto it = self->observers_.find(id);
        if (it != self->observers_.end() && it->second.on_add)
            it->second.on_add(entry);
    }
}

void registry::handle_global_remove(void* data, wl_registry*, uint32_t name) {
    auto* self = static_cast<registry*>(data);
    auto it = self->globals_by_name_.find(name);
    if (it != self->globals_by_name_.end()) {
        std::string iface = it->second.interface;
        self->globals_by_name_.erase(it);

        auto& vec = self->globals_by_iface_[iface];
        std::erase(vec, name);
    }

    std::vector<uint64_t> ids;
    ids.reserve(self->observers_.size());
    for (auto& [id, obs] : self->observers_) {
        ids.push_back(id);
    }

    for (uint64_t id : ids) {
        auto it = self->observers_.find(id);
        if (it != self->observers_.end() && it->second.on_remove)
            it->second.on_remove(name);
    }
}

} // namespace dwhbll::platform::wayland
