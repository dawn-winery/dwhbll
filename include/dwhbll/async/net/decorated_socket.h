#pragma once

#include <memory>

#include <dwhbll/async/net/isocket.h>

namespace dwhbll::async::net {
    class decorated_socket : public isocket {
        std::unique_ptr<isocket> super_;

    public:
        decorated_socket(std::unique_ptr<isocket>&& super) : super_(std::move(super)) { }

        ~decorated_socket() override = default;

        [[nodiscard]] bool is_shutdown() const noexcept override {
            return super_->is_shutdown();
        }

        [[nodiscard]] bool has_socket() const noexcept override {
            return super_->has_socket();
        }

        void set_nodelay(bool state) noexcept override {
            super_->set_nodelay(state);
        }

        [[nodiscard]] bool get_nodelay() const noexcept override {
            return super_->get_nodelay();
        }

        void close() noexcept override {
            super_->close();
        }

        [[nodiscard]] const network::address &get_address() const noexcept override {
            return super_->get_address();
        }

        [[nodiscard]] concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> read(std::span<std::uint8_t> buffer) override {
            return super_->read(buffer);
        }

        [[nodiscard]] concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> write(std::span<const std::uint8_t> buffer) override {
            return super_->write(buffer);
        }

        [[nodiscard]] concurrency::coroutine::task<stl_ext::Result<ssize_t, int>> read_some(std::span<std::uint8_t> buffer) override {
            return super_->read_some(buffer);
        }

        [[nodiscard]] concurrency::coroutine::task<stl_ext::Result<ssize_t, int>> write_some(std::span<const std::uint8_t> buffer) override {
            return super_->write_some(buffer);
        }

        [[nodiscard]] concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> flush() override {
            return super_->flush();
        }

        // TODO: there are better ways :xdd:
        [[nodiscard]] concurrency::coroutine::task<stl_ext::Result<stl_ext::UNIT, int>> write_str(const std::string &str) {
            return write(std::span{reinterpret_cast<const std::uint8_t*>(str.data()), str.size()});
        }
    };
}
