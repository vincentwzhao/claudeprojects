const express = require("express");
const authRoutes = require("./routes/auth");
const userRoutes = require("./routes/users");
const config = require("./config/config");

const app = express();
app.use(express.json());

app.use("/auth", authRoutes);
app.use("/users", userRoutes);

app.listen(config.port, () => {
  console.log(`Server listening on port ${config.port}`);
});

module.exports = app;
