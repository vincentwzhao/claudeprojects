const authService = require("../src/services/authService");

test("login rejects an unknown user", async () => {
  await expect(authService.login("nobody@example.com", "wrong")).rejects.toThrow(
    "Invalid email or password"
  );
});
