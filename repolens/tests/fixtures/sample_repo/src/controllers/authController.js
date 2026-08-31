const authService = require("../services/authService");

async function register(req, res) {
  try {
    const { email, password } = req.body;
    const user = await authService.register(email, password);
    res.status(201).json({ id: user.id, email: user.email });
  } catch (err) {
    res.status(400).json({ error: err.message });
  }
}

async function login(req, res) {
  try {
    const { email, password } = req.body;
    const { token, user } = await authService.login(email, password);
    res.status(200).json({ token, user: { id: user.id, email: user.email } });
  } catch (err) {
    res.status(401).json({ error: err.message });
  }
}

module.exports = { register, login };
