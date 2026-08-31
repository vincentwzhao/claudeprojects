const bcrypt = require("bcryptjs");
const jwt = require("jsonwebtoken");
const userRepository = require("../repositories/UserRepository");
const config = require("../config/config");

async function register(email, password) {
  const existing = await userRepository.findByEmail(email);
  if (existing) {
    throw new Error("A user with that email already exists");
  }
  const passwordHash = await bcrypt.hash(password, 10);
  return userRepository.create({ email, passwordHash });
}

async function login(email, password) {
  const user = await userRepository.findByEmail(email);
  if (!user) {
    throw new Error("Invalid email or password");
  }
  const passwordMatches = await bcrypt.compare(password, user.passwordHash);
  if (!passwordMatches) {
    throw new Error("Invalid email or password");
  }
  const token = jwt.sign({ sub: user.id, email: user.email }, config.jwtSecret, {
    expiresIn: config.jwtExpiresIn,
  });
  return { token, user };
}

function verifyToken(token) {
  return jwt.verify(token, config.jwtSecret);
}

module.exports = { register, login, verifyToken };
