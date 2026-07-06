#include <memory>
#include <string>
#include <vector>

#include <dwhbll/utils/clone.hpp>
#include <dwhbll/console/Logging.h>

#include <dwhbll/utils/format.hpp>

struct InventoryItem {
  std::string name;
  int quantity;
};

struct RandomUserStruct {
  int id;
  std::string name;
  std::vector<int> numbers;
  std::unique_ptr<std::string> message;
  std::vector<std::unique_ptr<InventoryItem>> inventory;
};

int main() {
  RandomUserStruct user{
      .id = 69,
      .name = "nat",
      .numbers = {1, 2, 3},
      .message = std::make_unique<std::string>("hello"),
      .inventory = {},
  };

  user.inventory.push_back(
      std::make_unique<InventoryItem>(
          InventoryItem{
              .name = "apple",
              .quantity = 3,
          }
      )
  );

  user.inventory.push_back(
      std::make_unique<InventoryItem>(
          InventoryItem{
              .name = "banana",
              .quantity = 5,
          }
      )
  );

  auto cloned = dwhbll::utils::clone(user);

  cloned.name = "clone";
  cloned.numbers[0] = 999;
  *cloned.message = "different";
  cloned.inventory[0]->quantity = 999;

  if (user.name != "nat") return 1;
  if (user.numbers[0] != 1) return 1;
  if (*user.message != "hello") return 1;
  if (user.inventory[0]->quantity != 3) return 1;

  if (cloned.message.get() == user.message.get()) return 1;
  if (cloned.inventory[0].get() == user.inventory[0].get()) return 1;


  dwhbll::console::info("Original Struct: {}",::dwhbll::debug::dbg(user));
  dwhbll::console::info("Cloned Struct: {}",::dwhbll::debug::dbg(cloned));
  

  return 0;
}
