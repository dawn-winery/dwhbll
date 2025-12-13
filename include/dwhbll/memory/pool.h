#pragma once

#include <cmath>
#include <list>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

#include <dwhbll/concurrency/spinlock.h>

namespace dwhbll::memory {
	/**
	 * Thread safe memory pool
	 * @tparam T the type of the object in the pool
	 */
	template <typename T, size_t BlockSize = std::max(1024/sizeof(T), 1ul)>
	requires std::default_initializable<T>
	class Pool {
// #pragma pack(push, 1)
		struct Obj {
			std::size_t blockAvailable = BlockSize;
			bool used[BlockSize] = {};
			T object[BlockSize];
			Obj* next{nullptr};
		};
// #pragma pack(pop)

		Obj* objects, *objectsBack;
		std::size_t available = 0, size = 0;
		concurrency::spinlock _lock;
		std::unordered_map<T*, std::pair<Obj*, std::size_t>> returning;

		Obj* makeNew() {
			Obj* obj = new Obj;
			objectsBack->next = obj;
			objectsBack = obj;
			size += BlockSize;
			return obj;
		}

	public:
		~Pool() {
			Obj* current = objects;
			while (current != nullptr) {
				for (int i = 0; i < BlockSize; i++) {
					if (current->used[i])
						current->object[i].~T();
				}
				Obj* last = current;
				current = current->next;
				delete last;
			}
		}

		// TODO: switch to a unique ptr.
		class ObjectWrapper {
			T* object;
			Pool* parent;

			friend class Pool;

		public:
			ObjectWrapper(Pool* parent, T* inner) : object(inner), parent(parent) {}

			// no copying, allow moving
			ObjectWrapper(const ObjectWrapper &other) = delete;

			ObjectWrapper(ObjectWrapper &&other) noexcept
				: object(other.object),
			      parent(other.parent) {
				other.object = nullptr;
				other.parent = nullptr;
			}

			ObjectWrapper & operator=(const ObjectWrapper &other) = delete;

			ObjectWrapper & operator=(ObjectWrapper &&other) noexcept {
				if (this == &other)
					return *this;
				object = other.object;
				parent = other.parent;

				other.object = nullptr;
				other.parent = nullptr;

				return *this;
			}

			/**
			 * Free the objects from the ObjectWrapper
			 * @note must return the object manually to the pool.
			 */
			T* disown() {
				T* obj = object;
				object = nullptr;
				return obj;
			}

			~ObjectWrapper();

			T& operator* ()
			{
				return *object;
			}

			T* operator-> ()
			{
				return object;
			}
		};

		explicit Pool(const std::size_t default_size = 16) {
			available = default_size * BlockSize;
			objects = objectsBack = new Obj;
			for (int i = 1; size < available; i++) {
				makeNew();
			}
			available = size;
		}

		/**
		 *
		 * @tparam Args The arguments to construct with.
		 */
		template <typename... Args>
		ObjectWrapper acquire(Args&&... args) {
			auto _dfd = _lock.lock();
			std::size_t index;
			Obj* obj = nullptr;
			if (available == 0) {
				// nothing left, make another object
				obj = makeNew();
				index = 0; // first one in the block
			} else {
				Obj* current = objects;
				while (current != nullptr) {
					if (current->blockAvailable == 0) {
						current = current->next;
						continue;
					}
					for (int i = 0; i < BlockSize; i++) {
						if (!current->used[i]) {
							obj = current;
							index = i;
							available--;
							goto found;
						}
					}
					current = current->next;
				}
			found:
				if (obj == nullptr)
					throw std::runtime_error("available != 0 and no available objects!");
			}
			--obj->blockAvailable;
			obj->used[index] = true;
			auto* f = obj->object;
			auto* newobj = new(f + index)T(args...);
			returning[f + index] = {obj, index};
			return ObjectWrapper(this, f + index);
		}

		T* find(const T& value) {
			auto _dfd = _lock.lock();
			Obj* current = objects;
			while (current != nullptr) {
				for (int i = 0; i < BlockSize; i++) {
					if (!current->used[i])
						continue;
					if (current->object[i] == value) {
						return current->object + i;
					}
				}
			}
			return nullptr;
		}

		void offer(T* object) {
			if (object != nullptr) {
				auto _dfd = _lock.lock();
				if (returning.contains(object)) {
					auto& [obj, index] = returning[object];
					obj->used[index] = false;
					obj->object[index].~T();
					++obj->blockAvailable;
					returning.erase(object);
					available++;
				} else {
					throw std::runtime_error("Object was not allocated in the pool!");
				}
			}
		}

		[[nodiscard]] std::size_t allocated_size() const {
			return sizeof(Obj) * (size / BlockSize);
		}

		[[nodiscard]] std::size_t used_size() const {
			return size - available;
		}

		[[nodiscard]] static constexpr std::size_t block_size() {
			return BlockSize;
		}
	};

	template<typename T, size_t BlockSize> requires std::default_initializable<T>
	Pool<T, BlockSize>::ObjectWrapper::~ObjectWrapper() {
		if (object != nullptr) {
			parent->offer(object);
		}
	}
}
