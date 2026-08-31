// Built-in food database.
// unit "g"    -> kcal is per 100 g; user enters a gram quantity
// unit "each" -> kcal is per one typical unit (egg, slice, cup, tbsp, etc.);
//                user enters a count
const FOOD_DB = [
  // Proteins
  { name: "Chicken breast, cooked", kcal: 165, unit: "g" },
  { name: "Chicken thigh, cooked", kcal: 209, unit: "g" },
  { name: "Ground beef, cooked (85% lean)", kcal: 250, unit: "g" },
  { name: "Steak, cooked", kcal: 271, unit: "g" },
  { name: "Pork chop, cooked", kcal: 231, unit: "g" },
  { name: "Bacon", kcal: 541, unit: "g" },
  { name: "Sausage", kcal: 300, unit: "g" },
  { name: "Ham", kcal: 145, unit: "g" },
  { name: "Turkey breast, cooked", kcal: 135, unit: "g" },
  { name: "Salmon, cooked", kcal: 208, unit: "g" },
  { name: "Tuna, canned in water", kcal: 116, unit: "g" },
  { name: "Shrimp, cooked", kcal: 99, unit: "g" },
  { name: "Egg, large", kcal: 72, unit: "each" },
  { name: "Egg white", kcal: 17, unit: "each" },
  { name: "Tofu, firm", kcal: 144, unit: "g" },
  { name: "Black beans, cooked", kcal: 132, unit: "g" },
  { name: "Lentils, cooked", kcal: 116, unit: "g" },
  { name: "Chickpeas, cooked", kcal: 164, unit: "g" },
  { name: "Hummus", kcal: 166, unit: "g" },

  // Grains & starches
  { name: "White rice, cooked", kcal: 130, unit: "g" },
  { name: "Brown rice, cooked", kcal: 123, unit: "g" },
  { name: "Quinoa, cooked", kcal: 120, unit: "g" },
  { name: "Pasta, cooked", kcal: 158, unit: "g" },
  { name: "Ramen noodles, cooked", kcal: 188, unit: "g" },
  { name: "Oats, cooked (oatmeal)", kcal: 71, unit: "g" },
  { name: "Bread, white (1 slice)", kcal: 79, unit: "each" },
  { name: "Bread, whole wheat (1 slice)", kcal: 69, unit: "each" },
  { name: "Bagel", kcal: 245, unit: "each" },
  { name: "Tortilla, flour (1)", kcal: 146, unit: "each" },
  { name: "Pita bread (1)", kcal: 165, unit: "each" },
  { name: "Potato, baked", kcal: 93, unit: "g" },
  { name: "Sweet potato, baked", kcal: 90, unit: "g" },
  { name: "French fries", kcal: 312, unit: "g" },
  { name: "Cereal, breakfast (dry)", kcal: 378, unit: "g" },
  { name: "Granola", kcal: 471, unit: "g" },

  // Fruits
  { name: "Apple", kcal: 95, unit: "each" },
  { name: "Banana", kcal: 105, unit: "each" },
  { name: "Orange", kcal: 62, unit: "each" },
  { name: "Grapes", kcal: 69, unit: "g" },
  { name: "Strawberries", kcal: 32, unit: "g" },
  { name: "Blueberries", kcal: 57, unit: "g" },
  { name: "Watermelon", kcal: 30, unit: "g" },
  { name: "Avocado", kcal: 240, unit: "each" },
  { name: "Orange juice", kcal: 45, unit: "g" },

  // Vegetables
  { name: "Broccoli, cooked", kcal: 35, unit: "g" },
  { name: "Spinach, raw", kcal: 23, unit: "g" },
  { name: "Carrot", kcal: 41, unit: "g" },
  { name: "Tomato", kcal: 18, unit: "g" },
  { name: "Cucumber", kcal: 15, unit: "g" },
  { name: "Bell pepper", kcal: 31, unit: "g" },
  { name: "Onion", kcal: 40, unit: "g" },
  { name: "Garlic", kcal: 149, unit: "g" },
  { name: "Mushroom", kcal: 22, unit: "g" },
  { name: "Corn", kcal: 86, unit: "g" },

  // Dairy
  { name: "Milk, whole", kcal: 61, unit: "g" },
  { name: "Milk, skim", kcal: 34, unit: "g" },
  { name: "Cheese, cheddar", kcal: 403, unit: "g" },
  { name: "Cheese, mozzarella", kcal: 280, unit: "g" },
  { name: "Cream cheese", kcal: 342, unit: "g" },
  { name: "Yogurt, plain", kcal: 61, unit: "g" },
  { name: "Greek yogurt, plain", kcal: 59, unit: "g" },
  { name: "Butter", kcal: 717, unit: "g" },

  // Fats & oils
  { name: "Olive oil", kcal: 119, unit: "each" }, // per tbsp
  { name: "Peanut butter", kcal: 94, unit: "each" }, // per tbsp
  { name: "Almonds", kcal: 579, unit: "g" },
  { name: "Mixed nuts", kcal: 607, unit: "g" },

  // Snacks & sweets
  { name: "Potato chips", kcal: 536, unit: "g" },
  { name: "Tortilla chips", kcal: 489, unit: "g" },
  { name: "Popcorn", kcal: 387, unit: "g" },
  { name: "Pretzels", kcal: 380, unit: "g" },
  { name: "Crackers", kcal: 421, unit: "g" },
  { name: "Chocolate bar", kcal: 546, unit: "g" },
  { name: "Cookie", kcal: 148, unit: "each" },
  { name: "Donut", kcal: 253, unit: "each" },
  { name: "Muffin", kcal: 340, unit: "each" },
  { name: "Ice cream", kcal: 207, unit: "g" },
  { name: "Granola bar", kcal: 118, unit: "each" },
  { name: "Protein bar", kcal: 200, unit: "each" },
  { name: "Honey", kcal: 64, unit: "each" }, // per tbsp
  { name: "Jam", kcal: 56, unit: "each" }, // per tbsp
  { name: "Sugar", kcal: 49, unit: "each" }, // per tbsp

  // Fast food & prepared
  { name: "Pizza slice (cheese)", kcal: 285, unit: "each" },
  { name: "Burger (fast food, plain)", kcal: 354, unit: "each" },
  { name: "Hot dog", kcal: 290, unit: "each" },
  { name: "Sushi roll, salmon avocado (6 pcs)", kcal: 255, unit: "each" },
  { name: "Burrito", kcal: 445, unit: "each" },
  { name: "Salad, mixed greens with dressing", kcal: 150, unit: "each" },

  // Beverages
  { name: "Soda (cola)", kcal: 42, unit: "g" },
  { name: "Coffee, black", kcal: 2, unit: "g" },
  { name: "Tea, unsweetened", kcal: 1, unit: "g" },
  { name: "Beer", kcal: 43, unit: "g" },
  { name: "Wine", kcal: 83, unit: "g" },
];
