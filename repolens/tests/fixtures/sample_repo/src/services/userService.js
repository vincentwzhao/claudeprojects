const userRepository = require("../repositories/UserRepository");

async function getProfile(userId, requestingUser) {
  if (requestingUser.sub !== userId) {
    throw new Error("Forbidden");
  }
  return userRepository.findByEmail(requestingUser.email);
}

module.exports = { getProfile };
