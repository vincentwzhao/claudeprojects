const pool = require("../db/connection");
const User = require("../models/User");

class UserRepository {
  async findByEmail(email) {
    const result = await pool.query("SELECT * FROM users WHERE email = $1", [email]);
    if (result.rows.length === 0) return null;
    return new User(result.rows[0]);
  }

  async create({ email, passwordHash }) {
    const result = await pool.query(
      "INSERT INTO users (email, password_hash) VALUES ($1, $2) RETURNING *",
      [email, passwordHash]
    );
    return new User(result.rows[0]);
  }
}

module.exports = new UserRepository();
